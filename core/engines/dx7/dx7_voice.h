// ─────────────────────────────────────────────────────────
// FILE: core/engines/dx7/dx7_voice.h
// BRIEF: DX7 6-operator FM voice
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include "dx7_algorithms.h"
#include <array>
#include <cmath>

struct DX7Operator {
    float phase    = 0.0f;
    float phaseInc = 0.0f;

    // ADSR envelope (simplified linear stages)
    enum class Stage { Attack, Decay, Sustain, Release, Off };
    Stage stage    = Stage::Off;
    float envVal   = 0.0f;   // current envelope value 0..1
    float attackRate  = 0.0f;
    float decayRate   = 0.0f;
    float sustainLvl  = 0.0f;
    float releaseRate = 0.0f;

    float level    = 1.0f;   // operator output level 0..1
    float velSens  = 0.5f;   // velocity sensitivity
    float ratio    = 1.0f;   // freq ratio (relative to note)
    float fixedHz  = 440.0f; // fixed frequency mode in Hz
    bool  fixedMode = false; // true: fixedHz, false: baseFreq*ratio
    float output   = 0.0f;   // last tick output (for feedback)

    void trigger(float baseFreq, float sampleRate,
                 float ratio, float level, float velSens,
                 bool  fixedMode, float fixedHz,
                 float attack, float decay, float sustain, float release,
                 int   velocity, float kbdRateScale = 0.0f,
                 int   midiNote = 60) noexcept;

    void release() noexcept;

    // [RT-SAFE] tick — returns output sample; pitchFactor = pow(2, bendSemis/12)
    float tick(float pitchFactor = 1.0f) noexcept;
};

// ════════════════════════════════════════════════════════
// DX7Voice — 6 operators + algorithm routing
// ════════════════════════════════════════════════════════

struct DX7Voice {
    static constexpr int NUM_OPS = 6;

    bool  active   = false;
    int   note     = 60;
    int   velocity = 100;

    std::array<DX7Operator, NUM_OPS> ops;

    float feedback     = 0.0f;   // feedback from op0 (0..1 → 0..4 radians)
    float feedbackBuf  = 0.0f;   // previous op0 output

    void trigger(int midiNote, int vel,
                 float sampleRate,
                 int   algorithm,      // 0..31
                 const float ratios[NUM_OPS],
                 const bool  fixedMode[NUM_OPS],
                 const float fixedHz[NUM_OPS],
                 const float levels[NUM_OPS],
                 const float velSens[NUM_OPS],
                 const float attacks[NUM_OPS],
                 const float decays[NUM_OPS],
                 const float sustains[NUM_OPS],
                 const float releases[NUM_OPS],
                 float feedbackAmount,
                 const float kbdRate[NUM_OPS] = nullptr) noexcept;

    void release() noexcept;

    // [RT-SAFE]
    void tick(int algorithm, float pitchFactor, float& outL, float& outR) noexcept;
};
