// ─────────────────────────────────────────────────────────
// FILE: core/engines/moog/voice_pool.cpp
// BRIEF: Voice pool implementation
// ─────────────────────────────────────────────────────────
#include "voice_pool.h"
#include "core/util/math_utils.h"
#include <cmath>

// ─────────────────────────────────────────────────────────
// INIT
// ─────────────────────────────────────────────────────────

void VoicePool::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    for (auto& v : voices_) v.init(sampleRate);
    heldCount_ = 0;
}

// ─────────────────────────────────────────────────────────
// CONFIG — call once per block before tick loop
// ─────────────────────────────────────────────────────────

void VoicePool::applyConfig(const float p[]) noexcept {
    mode_         = static_cast<VoiceMode>(static_cast<int>(p[MP_VOICE_MODE]));
    stealMode_    = static_cast<StealMode>(static_cast<int>(p[MP_VOICE_STEAL]));
    maxVoices_    = clamp(static_cast<int>(p[MP_VOICE_COUNT]), 1, MAX_VOICES);
    unisonDetune_ = p[MP_UNISON_DETUNE] * 50.0f;  // 0..50 cents
    notePriority_ = clamp(static_cast<int>(p[MP_NOTE_PRIORITY]), 0, 2);

    // Compensate for loudness increase from multiple voices
    outputGain_ = (maxVoices_ > 1)
                ? (1.0f / std::sqrt(static_cast<float>(maxVoices_)))
                : 1.0f;
}

// ─────────────────────────────────────────────────────────
// TICK — [RT-SAFE]
// ─────────────────────────────────────────────────────────

void VoicePool::tick(const float params[],
                     sample_t& outL,
                     sample_t& outR) noexcept {
    sample_t sum = 0.0f;

    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voices_[i].isActive()) {
            voices_[i].tickAge();
            sum += voices_[i].tick(params);
        }
    }

    sum  *= outputGain_;
    outL  = sum;
    outR  = sum;  // mono sum → stereo (no panning in V1)
}

// ─────────────────────────────────────────────────────────
// NOTE ON
// ─────────────────────────────────────────────────────────

void VoicePool::noteOn(int note, int vel) noexcept {
    pushHeld(note, vel);
    switch (mode_) {
        case VoiceMode::Mono:   noteOnMono  (note, vel); break;
        case VoiceMode::Poly:   noteOnPoly  (note, vel); break;
        case VoiceMode::Unison: noteOnUnison(note, vel); break;
    }
}

void VoicePool::noteOnMono(int note, int vel) noexcept {
    Voice* v = nullptr;
    for (auto& voice : voices_)
        if (voice.isActive()) { v = &voice; break; }
    if (!v) v = findFreeVoice();
    if (!v) v = findVoiceToSteal();
    if (!v) return;
    triggerVoice(v, note, vel, 0.0f);
}

void VoicePool::noteOnPoly(int note, int vel) noexcept {
    Voice* v = findVoiceByNote(note);
    if (!v) v = findFreeVoice();
    if (!v) v = findVoiceToSteal();
    if (!v) return;
    triggerVoice(v, note, vel, 0.0f);
}

void VoicePool::noteOnUnison(int note, int vel) noexcept {
    // Silence all existing voices first
    for (auto& v : voices_)
        if (v.isActive()) v.forceOff();

    const int n = maxVoices_;
    // P3: Analog-style detune — linear spread + per-voice LCG random jitter
    // Reseed per-note so the same note always gets the same spread character
    unisonSeed_ = static_cast<uint32_t>(note) * 2654435761u + 12345u;
    for (int i = 0; i < n; ++i) {
        Voice* v = findFreeVoice();
        if (!v) break;
        float offset = 0.0f;
        if (n > 1) {
            // Base: linear equal-spacing across detune range
            const float base = (i - (n - 1) * 0.5f) * (unisonDetune_ / (n - 1));
            // Jitter: ±20% of total spread, per-voice random (LCG)
            unisonSeed_ = unisonSeed_ * 1664525u + 1013904223u;
            const float rnd = (unisonSeed_ >> 16) / 65535.0f - 0.5f;  // -0.5..+0.5
            const float jitter = rnd * unisonDetune_ * 0.2f;
            offset = base + jitter;
        }
        triggerVoice(v, note, vel, offset);
    }
}

// ─────────────────────────────────────────────────────────
// NOTE OFF
// ─────────────────────────────────────────────────────────

void VoicePool::noteOff(int note) noexcept {
    removeHeld(note);
    switch (mode_) {
        case VoiceMode::Mono:   noteOffMono  (note); break;
        case VoiceMode::Poly:   noteOffPoly  (note); break;
        case VoiceMode::Unison: noteOffUnison(note); break;
    }
}

void VoicePool::noteOffMono(int note) noexcept {
    // If another note is still held, retrigger based on priority
    const HeldNote* prev = topHeldByPriority();
    if (prev) {
        Voice* v = nullptr;
        for (auto& voice : voices_)
            if (voice.isActive()) { v = &voice; break; }
        if (v) triggerVoice(v, prev->midiNote, prev->velocity, 0.0f);
        return;
    }
    for (auto& v : voices_)
        if (v.isActive() && v.getNote() == note)
            v.noteOff();
}

void VoicePool::noteOffPoly(int note) noexcept {
    Voice* v = findVoiceByNote(note);
    if (v) v->noteOff();
}

void VoicePool::noteOffUnison(int note) noexcept {
    (void)note;
    for (auto& v : voices_)
        if (v.isActive()) v.noteOff();
}

void VoicePool::allNotesOff() noexcept {
    for (auto& v : voices_)
        if (v.isActive()) v.noteOff();
    heldCount_ = 0;
}

void VoicePool::allSoundOff() noexcept {
    for (auto& v : voices_) v.forceOff();
    heldCount_ = 0;
}

// ─────────────────────────────────────────────────────────
// VOICE SELECTION
// ─────────────────────────────────────────────────────────

Voice* VoicePool::findFreeVoice() noexcept {
    for (int i = 0; i < maxVoices_; ++i)
        if (!voices_[i].isActive()) return &voices_[i];
    return nullptr;
}

Voice* VoicePool::findVoiceToSteal() noexcept {
    switch (stealMode_) {
        case StealMode::Oldest: {
            Voice* best  = nullptr;
            int    maxAge = -1;
            for (int i = 0; i < maxVoices_; ++i) {
                if (voices_[i].getAge() > maxAge) {
                    maxAge = voices_[i].getAge();
                    best   = &voices_[i];
                }
            }
            return best;
        }
        case StealMode::Lowest: {
            Voice* best    = nullptr;
            int    minNote = 128;
            for (int i = 0; i < maxVoices_; ++i) {
                if (voices_[i].getNote() < minNote) {
                    minNote = voices_[i].getNote();
                    best    = &voices_[i];
                }
            }
            return best;
        }
        case StealMode::Quietest: {
            Voice* best     = nullptr;
            float  minLevel = 2.0f;
            for (int i = 0; i < maxVoices_; ++i) {
                if (voices_[i].getLevel() < minLevel) {
                    minLevel = voices_[i].getLevel();
                    best     = &voices_[i];
                }
            }
            return best;
        }
    }
    return &voices_[0];  // fallback
}

Voice* VoicePool::findVoiceByNote(int n) noexcept {
    for (int i = 0; i < maxVoices_; ++i)
        if (voices_[i].isActive() && voices_[i].getNote() == n)
            return &voices_[i];
    return nullptr;
}

void VoicePool::triggerVoice(Voice* v, int note, int vel,
                              float detuneCents) noexcept {
    v->noteOn(note, vel, detuneCents);
}

// ─────────────────────────────────────────────────────────
// HELD NOTES STACK
// ─────────────────────────────────────────────────────────

void VoicePool::pushHeld(int note, int vel) noexcept {
    // Update velocity if already held
    for (int i = 0; i < heldCount_; ++i) {
        if (heldNotes_[i].midiNote == note) {
            heldNotes_[i].velocity = vel;
            return;
        }
    }
    if (heldCount_ < MAX_HELD_NOTES)
        heldNotes_[heldCount_++] = { note, vel };
}

void VoicePool::removeHeld(int note) noexcept {
    for (int i = 0; i < heldCount_; ++i) {
        if (heldNotes_[i].midiNote == note) {
            for (int j = i; j < heldCount_ - 1; ++j)
                heldNotes_[j] = heldNotes_[j + 1];
            --heldCount_;
            return;
        }
    }
}

const HeldNote* VoicePool::topHeld() const noexcept {
    if (heldCount_ == 0) return nullptr;
    return &heldNotes_[heldCount_ - 1];  // most recent
}

const HeldNote* VoicePool::topHeldByPriority() const noexcept {
    if (heldCount_ == 0) return nullptr;
    switch (notePriority_) {
        default:  // 0 = Last note (LIFO)
            return &heldNotes_[heldCount_ - 1];
        case 1: { // Lowest note
            const HeldNote* best = &heldNotes_[0];
            for (int i = 1; i < heldCount_; ++i)
                if (heldNotes_[i].midiNote < best->midiNote)
                    best = &heldNotes_[i];
            return best;
        }
        case 2: { // Highest note
            const HeldNote* best = &heldNotes_[0];
            for (int i = 1; i < heldCount_; ++i)
                if (heldNotes_[i].midiNote > best->midiNote)
                    best = &heldNotes_[i];
            return best;
        }
    }
}

int VoicePool::getActiveCount() const noexcept {
    int count = 0;
    for (int i = 0; i < MAX_VOICES; ++i)
        if (voices_[i].isActive()) ++count;
    return count;
}
