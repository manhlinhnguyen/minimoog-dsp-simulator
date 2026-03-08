// ─────────────────────────────────────────────────────────
// FILE: core/music/midi_file_player.cpp
// BRIEF: SMF parser + real-time playback implementation
// ─────────────────────────────────────────────────────────
#include "midi_file_player.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// ── SMF parse helpers ─────────────────────────────────────

static inline uint16_t read16be(const uint8_t* p) noexcept {
    return static_cast<uint16_t>((p[0] << 8) | p[1]);
}
static inline uint32_t read32be(const uint8_t* p) noexcept {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16)
         | (uint32_t(p[2]) <<  8) |  uint32_t(p[3]);
}

// Decode variable-length quantity; advances p.
static uint32_t readVLQ(const uint8_t*& p, const uint8_t* end) noexcept {
    uint32_t val = 0;
    while (p < end) {
        const uint8_t b = *p++;
        val = (val << 7) | (b & 0x7Fu);
        if (!(b & 0x80u)) break;
    }
    return val;
}

// ── MidiFilePlayer::load ──────────────────────────────────

bool MidiFilePlayer::load(const uint8_t* data, size_t len,
                           const char* name) noexcept {
    // Must be stopped; caller is responsible for ensuring no race.
    state_.store(static_cast<int>(State::Stopped), std::memory_order_relaxed);
    nextIdx_ = 0;
    curTick_ = 0.0;
    posNorm_.store(0.0f, std::memory_order_relaxed);
    seekReq_.store(-1.0f, std::memory_order_relaxed);

    events_.clear();
    tempoMap_.clear();
    totalTicks_ = 0;
    ppqn_       = 480;
    hasFile_    = false;

    if (!data || len < 14) return false;

    const bool ok = parseFile(data, len);
    if (!ok) {
        events_.clear();
        tempoMap_.clear();
        return false;
    }

    // Ensure there is at least a default tempo entry
    if (tempoMap_.empty())
        tempoMap_.push_back({0u, 500000u}); // 120 BPM default

    // Sort events by tick (stable to preserve same-tick order)
    std::stable_sort(events_.begin(), events_.end(),
        [](const Event& a, const Event& b) { return a.tick < b.tick; });

    if (name && name[0]) {
        std::strncpy(fileName_, name, sizeof(fileName_) - 1);
        fileName_[sizeof(fileName_) - 1] = '\0';
    } else {
        fileName_[0] = '\0';
    }

    hasFile_ = true;
    return true;
}

void MidiFilePlayer::unload() noexcept {
    state_.store(static_cast<int>(State::Stopped), std::memory_order_relaxed);
    hasFile_    = false;
    fileName_[0] = '\0';
    events_.clear();
    tempoMap_.clear();
    totalTicks_ = 0;
    nextIdx_    = 0;
    curTick_    = 0.0;
    posNorm_.store(0.0f, std::memory_order_relaxed);
}

// ── SMF parser ────────────────────────────────────────────

bool MidiFilePlayer::parseFile(const uint8_t* data, size_t len) noexcept {
    const uint8_t* p   = data;
    const uint8_t* end = data + len;

    // ── Header chunk ──────────────────────────────────────
    if (end - p < 14) return false;
    if (std::memcmp(p, "MThd", 4) != 0) return false;
    const uint32_t hdrLen = read32be(p + 4);
    if (hdrLen < 6) return false;

    const uint16_t format  = read16be(p + 8);
    const uint16_t ntracks = read16be(p + 10);
    const uint16_t div     = read16be(p + 12);
    p += 8 + hdrLen;

    if (format > 1) return false;  // Format 2 not supported
    if (div & 0x8000u) return false;  // SMPTE not supported
    ppqn_ = div ? div : 480;

    // ── Track chunks ─────────────────────────────────────
    for (int tr = 0; tr < ntracks && p + 8 <= end; ++tr) {
        if (std::memcmp(p, "MTrk", 4) != 0) {
            // Skip unknown chunk
            const uint32_t skip = read32be(p + 4);
            p += 8 + skip;
            continue;
        }
        const uint32_t trkLen = read32be(p + 4);
        p += 8;
        const uint8_t* trkEnd = p + trkLen;
        if (trkEnd > end) trkEnd = end;

        uint32_t tick          = 0;
        uint8_t  runningStatus = 0;

        while (p < trkEnd) {
            // Delta time
            const uint32_t delta = readVLQ(p, trkEnd);
            tick += delta;
            if (p >= trkEnd) break;

            const uint8_t b = *p;

            // ── Meta event ────────────────────────────────
            if (b == 0xFF) {
                ++p;
                if (p >= trkEnd) break;
                const uint8_t mtype = *p++;
                const uint32_t mlen = readVLQ(p, trkEnd);
                if (p + mlen > trkEnd) break;

                if (mtype == 0x51 && mlen == 3) {
                    const uint32_t uspb =
                        (uint32_t(p[0]) << 16) | (uint32_t(p[1]) << 8) | p[2];
                    tempoMap_.push_back({tick, uspb});
                }
                // 0x2F = End of Track — stop parsing this track
                p += mlen;
                if (mtype == 0x2F) break;
                // Running status unchanged after meta
                continue;
            }

            // ── SysEx ─────────────────────────────────────
            if (b == 0xF0 || b == 0xF7) {
                ++p;
                const uint32_t slen = readVLQ(p, trkEnd);
                if (p + slen > trkEnd) break;
                p += slen;
                runningStatus = 0;
                continue;
            }

            // ── MIDI event ────────────────────────────────
            uint8_t status;
            if (b & 0x80u) {
                status = b;
                ++p;
                runningStatus = (b < 0xF0u) ? b : 0u;
            } else {
                status = runningStatus;
                // p NOT advanced — b is data1
            }
            if (status == 0) { ++p; continue; } // corrupt

            const uint8_t stype = status & 0xF0u;

            uint8_t d1 = 0, d2 = 0;
            if (p < trkEnd) d1 = *p++;
            // 1-data-byte messages: Program Change (Cx), Channel Pressure (Dx)
            const bool twoData = (stype != 0xC0u && stype != 0xD0u);
            if (twoData && p < trkEnd) d2 = *p++;

            // Store only messages we care about
            if (stype == 0x80u || stype == 0x90u ||
                stype == 0xB0u || stype == 0xE0u ||
                stype == 0xC0u) {
                events_.push_back({tick, status, d1, d2});
            }

            totalTicks_ = std::max(totalTicks_, tick);
        }
    }

    // Sort tempo map
    std::stable_sort(tempoMap_.begin(), tempoMap_.end(),
        [](const TempoChange& a, const TempoChange& b) {
            return a.tick < b.tick;
        });

    return !events_.empty();
}

// ── Transport ─────────────────────────────────────────────

void MidiFilePlayer::play() noexcept {
    if (!hasFile_) return;
    state_.store(static_cast<int>(State::Playing), std::memory_order_release);
}

void MidiFilePlayer::pause() noexcept {
    const State cur = getState();
    if (cur == State::Playing)
        state_.store(static_cast<int>(State::Paused), std::memory_order_relaxed);
    else if (cur == State::Paused)
        state_.store(static_cast<int>(State::Playing), std::memory_order_release);
}

void MidiFilePlayer::stop() noexcept {
    state_.store(static_cast<int>(State::Stopped), std::memory_order_relaxed);
    // Reset position — audio thread will pick it up via seekReq_
    seekReq_.store(0.0f, std::memory_order_relaxed);
}

void MidiFilePlayer::seekNorm(float norm) noexcept {
    norm = std::max(0.0f, std::min(1.0f, norm));
    seekReq_.store(norm, std::memory_order_relaxed);
}

void MidiFilePlayer::setTempoScale(float s) noexcept {
    s = std::max(0.25f, std::min(4.0f, s));
    tempoScale_.store(s, std::memory_order_relaxed);
}

// ── Tick (audio thread) ───────────────────────────────────

void MidiFilePlayer::tick(int nFrames, float sampleRate,
                           BlockEvents& out) noexcept {
    out.count = 0;
    if (!hasFile_) return;

    // Apply pending seek (any state)
    {
        const float req = seekReq_.exchange(-1.0f, std::memory_order_relaxed);
        if (req >= 0.0f) {
            curTick_  = req * static_cast<double>(totalTicks_);
            // Reposition event index
            nextIdx_ = 0;
            while (nextIdx_ < events_.size() &&
                   events_[nextIdx_].tick < static_cast<uint32_t>(curTick_))
                ++nextIdx_;
            posNorm_.store(req, std::memory_order_relaxed);
        }
    }

    if (static_cast<State>(state_.load(std::memory_order_acquire)) != State::Playing)
        return;
    if (totalTicks_ == 0) return;

    const float  tScale  = tempoScale_.load(std::memory_order_relaxed);
    const double tps     = ticksPerSample(sampleRate) * static_cast<double>(tScale);
    const double advance = tps * nFrames;
    const double endTick = curTick_ + advance;

    // Push all due events
    while (nextIdx_ < events_.size()) {
        const Event& ev = events_[nextIdx_];
        if (static_cast<double>(ev.tick) >= endTick) break;

        const uint8_t stype = ev.status & 0xF0u;
        const uint8_t ch    = ev.status & 0x0Fu;

        if (stype == 0x90u) {
            if (ev.d2 > 0) out.push(makeNoteOn(ch, ev.d1, ev.d2));
            else            out.push(makeNoteOff(ch, ev.d1));
        } else if (stype == 0x80u) {
            out.push(makeNoteOff(ch, ev.d1));
        } else if (stype == 0xB0u) {
            out.push(makeCC(ch, ev.d1, ev.d2));
        } else if (stype == 0xE0u) {
            out.push(makePB(ch, ev.d1, ev.d2));
        }
        ++nextIdx_;
    }

    curTick_ = endTick;

    // Update UI-readable position
    posNorm_.store(static_cast<float>(curTick_ / totalTicks_),
                   std::memory_order_relaxed);

    // End-of-file handling
    if (curTick_ >= static_cast<double>(totalTicks_)) {
        if (loop_.load(std::memory_order_relaxed)) {
            curTick_ = 0.0;
            nextIdx_ = 0;
            posNorm_.store(0.0f, std::memory_order_relaxed);
        } else {
            state_.store(static_cast<int>(State::Stopped),
                         std::memory_order_relaxed);
            curTick_ = 0.0;
            nextIdx_ = 0;
            posNorm_.store(0.0f, std::memory_order_relaxed);
        }
    }
}

// ── Helpers ───────────────────────────────────────────────

double MidiFilePlayer::ticksPerSample(float sampleRate) const noexcept {
    // Find tempo at curTick_ (last TempoChange at or before curTick_)
    uint32_t uspb = 500000u;  // default: 120 BPM
    for (const auto& tc : tempoMap_) {
        if (tc.tick <= static_cast<uint32_t>(curTick_))
            uspb = tc.microPerBeat;
        else
            break;
    }
    // ticks/sample = ppqn * 1e6 / (uspb * sampleRate)
    return static_cast<double>(ppqn_) * 1.0e6
           / (static_cast<double>(uspb) * static_cast<double>(sampleRate));
}

double MidiFilePlayer::ticksToMicros(double ticks) const noexcept {
    if (tempoMap_.empty() || ppqn_ == 0) return 0.0;
    double micros   = 0.0;
    double lastTick = 0.0;
    uint32_t uspb   = 500000u;
    for (const auto& tc : tempoMap_) {
        const double seg = std::min(ticks, static_cast<double>(tc.tick));
        micros += (seg - lastTick) * static_cast<double>(uspb) / ppqn_;
        if (seg >= ticks) break;
        lastTick = static_cast<double>(tc.tick);
        uspb     = tc.microPerBeat;
    }
    if (lastTick < ticks)
        micros += (ticks - lastTick) * static_cast<double>(uspb) / ppqn_;
    return micros;
}

float MidiFilePlayer::getPositionSec() const noexcept {
    if (!hasFile_) return 0.0f;
    const double pos = posNorm_.load(std::memory_order_relaxed)
                       * static_cast<double>(totalTicks_);
    return static_cast<float>(ticksToMicros(pos) * 1e-6);
}

float MidiFilePlayer::getDurationSec() const noexcept {
    if (!hasFile_) return 0.0f;
    return static_cast<float>(ticksToMicros(totalTicks_) * 1e-6);
}

int MidiFilePlayer::getFileBPM() const noexcept {
    if (tempoMap_.empty()) return 120;
    return static_cast<int>(60000000u / tempoMap_[0].microPerBeat);
}

// ── Event factories ───────────────────────────────────────

MidiEvent MidiFilePlayer::makeNoteOn(uint8_t ch, uint8_t note, uint8_t vel) noexcept {
    MidiEvent e;
    e.type    = MidiEvent::Type::NoteOn;
    e.channel = ch;
    e.data1   = note;
    e.data2   = vel;
    return e;
}

MidiEvent MidiFilePlayer::makeNoteOff(uint8_t ch, uint8_t note) noexcept {
    MidiEvent e;
    e.type    = MidiEvent::Type::NoteOff;
    e.channel = ch;
    e.data1   = note;
    e.data2   = 0;
    return e;
}

MidiEvent MidiFilePlayer::makeCC(uint8_t ch, uint8_t cc, uint8_t val) noexcept {
    MidiEvent e;
    e.type    = MidiEvent::Type::ControlChange;
    e.channel = ch;
    e.data1   = cc;
    e.data2   = val;
    return e;
}

MidiEvent MidiFilePlayer::makePB(uint8_t ch, uint8_t lsb, uint8_t msb) noexcept {
    MidiEvent e;
    e.type      = MidiEvent::Type::PitchBend;
    e.channel   = ch;
    e.pitchBend = static_cast<int16_t>(
        (static_cast<int>(msb) << 7 | lsb) - 8192);
    return e;
}
