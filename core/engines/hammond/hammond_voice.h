// ─────────────────────────────────────────────────────────
// FILE: core/engines/hammond/hammond_voice.h
// BRIEF: Single Hammond tonewheel voice (9 drawbars)
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include "core/dsp/noise.h"

// 9 drawbar harmonic ratios (relative to fundamental)
// 16', 5⅓', 8', 4', 2⅔', 2', 1⅗', 1⅓', 1'
constexpr float HAMMOND_RATIOS[9] = {
    0.5f, 0.75f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f
};

struct HammondVoice {
    bool  active    = false;
    int   note      = 60;
    float baseFreq  = 261.63f;
    float phases[9] = {};
    float phaseInc[9] = {};

    // Percussion
    float percPhase    = 0.0f;
    float percPhaseInc = 0.0f;
    float percEnv      = 0.0f;   // 0..1 decay envelope

    // Key-click transient
    float clickEnv = 0.0f;       // fast noise burst
    NoiseGenerator noise_;       // white noise for click burst

    void trigger(int midiNote, float sampleRate) noexcept;
    void release() noexcept;

    // [RT-SAFE]
    void tick(const float drawbars[9],
              bool  percOn,
              int   percHarm,      // 0=2nd, 1=3rd
              float percDecayRate, // per-sample multiplier
              float percLevel,
              float clickLevel,
              float pitchFactor,   // pow(2, pitchBendSemis/12) — real-time pitch bend
              float& outL, float& outR) noexcept;
};
