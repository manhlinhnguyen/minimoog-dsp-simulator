// ─────────────────────────────────────────────────────────
// FILE: core/music/midi_file_player.h
// BRIEF: Standard MIDI File (SMF) parser + real-time playback engine
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include <atomic>
#include <vector>
#include <cstdint>
#include <cstring>

// ════════════════════════════════════════════════════════
// MidiFilePlayer
//
// Thread model:
//   load / unload / seekNorm  → UI/init thread [RT-UNSAFE]
//   play / pause / stop /
//   setLoop / setTempoScale   → UI thread (atomic write)
//   tick()                    → audio thread  [RT-SAFE]
//
// Safety: load() must only be called while state == Stopped.
// The audio thread checks state_ before touching events_[].
// ════════════════════════════════════════════════════════

class MidiFilePlayer {
public:
    enum class State : int { Stopped = 0, Playing = 1, Paused = 2 };

    // Small stack-allocated event batch produced by tick().
    // Avoids going through the SPSC MidiEventQueue (audio thread is
    // both producer and consumer there, which violates SPSC contract).
    struct BlockEvents {
        static constexpr int MAX = 64;
        MidiEvent events[MAX];
        int       count = 0;

        void push(const MidiEvent& e) noexcept {
            if (count < MAX) events[count++] = e;
        }
    };

    MidiFilePlayer()  = default;
    ~MidiFilePlayer() = default;

    // ── Load / Unload [RT-UNSAFE] ─────────────────────────
    // data: raw SMF bytes. fileName: display name (may be nullptr).
    // Returns false if the file is malformed.
    bool load(const uint8_t* data, size_t len,
              const char* fileName = "") noexcept;
    void unload() noexcept;

    bool        hasFile()  const noexcept { return hasFile_; }
    const char* fileName() const noexcept { return fileName_; }

    // ── Transport (UI thread, atomic) ─────────────────────
    void play()  noexcept;
    void pause() noexcept;
    void stop()  noexcept;
    void setLoop(bool on)       noexcept { loop_.store(on,  std::memory_order_relaxed); }
    void seekNorm(float norm)   noexcept;   // 0..1 — only while Paused/Stopped
    void setTempoScale(float s) noexcept;   // 0.25..4.0

    // ── Audio thread [RT-SAFE] ────────────────────────────
    // Call once per processBlock. Fills out with MIDI events due this block.
    void tick(int nFrames, float sampleRate, BlockEvents& out) noexcept;

    // ── UI queries (any thread, atomic) ──────────────────
    State getState()        const noexcept { return static_cast<State>(state_.load(std::memory_order_relaxed)); }
    bool  isPlaying()       const noexcept { return getState() == State::Playing; }
    bool  isPaused()        const noexcept { return getState() == State::Paused;  }
    bool  isStopped()       const noexcept { return getState() == State::Stopped; }
    bool  isLooping()       const noexcept { return loop_.load(std::memory_order_relaxed); }
    float getPositionNorm() const noexcept { return posNorm_.load(std::memory_order_relaxed); }
    float getTempoScale()   const noexcept { return tempoScale_.load(std::memory_order_relaxed); }
    float getPositionSec()  const noexcept;
    float getDurationSec()  const noexcept;
    int   getFileBPM()      const noexcept;   // first tempo event → BPM

private:
    // ── Parsed SMF data (immutable after load, guarded by state_) ──
    struct Event {
        uint32_t tick;
        uint8_t  status;   // 8x/9x/Ax/Bx/Cx/Dx/Ex
        uint8_t  d1, d2;
    };
    struct TempoChange {
        uint32_t tick;
        uint32_t microPerBeat;
    };

    std::vector<Event>       events_;
    std::vector<TempoChange> tempoMap_;
    uint32_t totalTicks_ = 0;
    uint16_t ppqn_       = 480;
    bool     hasFile_    = false;
    char     fileName_[256] = {};

    // ── Audio-thread-only state (no atomic needed) ────────
    size_t nextIdx_  = 0;
    double curTick_  = 0.0;

    // ── Atomics (UI ↔ audio) ──────────────────────────────
    std::atomic<int>   state_      {0};
    std::atomic<bool>  loop_       {false};
    std::atomic<float> tempoScale_ {1.0f};
    std::atomic<float> posNorm_    {0.0f};
    std::atomic<float> seekReq_    {-1.0f};  // >=0 = pending seek (norm)

    // ── Helpers ───────────────────────────────────────────
    bool   parseFile(const uint8_t* data, size_t len) noexcept;
    double ticksPerSample(float sampleRate) const noexcept;
    double ticksToMicros(double ticks)      const noexcept;

    static MidiEvent makeNoteOn (uint8_t ch, uint8_t note, uint8_t vel) noexcept;
    static MidiEvent makeNoteOff(uint8_t ch, uint8_t note)              noexcept;
    static MidiEvent makeCC     (uint8_t ch, uint8_t cc,  uint8_t val)  noexcept;
    static MidiEvent makePB     (uint8_t ch, uint8_t lsb, uint8_t msb)  noexcept;
};
