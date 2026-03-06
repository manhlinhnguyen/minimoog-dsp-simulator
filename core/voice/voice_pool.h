// ─────────────────────────────────────────────────────────
// FILE: core/voice/voice_pool.h
// BRIEF: Polyphony manager — owns all voices, handles stealing
// ─────────────────────────────────────────────────────────
#pragma once
#include "voice.h"
#include "shared/types.h"
#include "shared/params.h"
#include <array>

// ════════════════════════════════════════════════════════
// VOICE POOL — polyphony manager
// Owns MAX_VOICES Voice objects (pre-allocated).
// Handles: note-on/off, voice stealing, unison spread.
// Modes: Mono | Poly | Unison
// ════════════════════════════════════════════════════════

class VoicePool {
public:
    // ── Setup ────────────────────────────────────────────
    void init(float sampleRate) noexcept;

    // ── Note events (from SynthEngine) ───────────────────
    void noteOn    (int note, int vel) noexcept;
    void noteOff   (int note)          noexcept;
    void allNotesOff()                 noexcept;
    void allSoundOff()                 noexcept;  // immediate

    // ── Config (call once per block from params snapshot) ─
    void applyConfig(const float params[]) noexcept;

    // ── Per-sample render ────────────────────────────────
    // [RT-SAFE]
    void tick(const float params[],
              sample_t& outL,
              sample_t& outR) noexcept;

    // ── Query ────────────────────────────────────────────
    int       getActiveCount() const noexcept;
    int       getMaxVoices()   const noexcept { return maxVoices_; }
    VoiceMode getMode()        const noexcept { return mode_; }

private:
    std::array<Voice, MAX_VOICES> voices_;
    float     sampleRate_   = SAMPLE_RATE_DEFAULT;
    VoiceMode mode_         = VoiceMode::Mono;
    StealMode stealMode_    = StealMode::Oldest;
    int       maxVoices_    = 1;
    float     unisonDetune_ = 0.0f;  // total spread in cents
    float     outputGain_   = 1.0f;  // 1/sqrt(N) for headroom

    // Held notes ring (for mono legato retriggering)
    std::array<HeldNote, MAX_HELD_NOTES> heldNotes_;
    int heldCount_ = 0;

    // ── Voice management ─────────────────────────────────
    Voice* findFreeVoice()        noexcept;
    Voice* findVoiceToSteal()     noexcept;
    Voice* findVoiceByNote(int n) noexcept;

    void triggerVoice(Voice* v, int note, int vel,
                      float detuneCents = 0.0f) noexcept;

    // ── Mode-specific dispatch ────────────────────────────
    void noteOnMono  (int note, int vel) noexcept;
    void noteOnPoly  (int note, int vel) noexcept;
    void noteOnUnison(int note, int vel) noexcept;

    void noteOffMono  (int note) noexcept;
    void noteOffPoly  (int note) noexcept;
    void noteOffUnison(int note) noexcept;

    // ── Held notes stack ─────────────────────────────────
    void            pushHeld  (int note, int vel) noexcept;
    void            removeHeld(int note)          noexcept;
    const HeldNote* topHeld() const               noexcept;
};
