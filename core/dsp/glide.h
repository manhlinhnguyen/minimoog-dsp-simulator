// ─────────────────────────────────────────────────────────
// FILE: core/dsp/glide.h
// BRIEF: Portamento / glide — exponential interpolation in log domain
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include <cmath>

// ════════════════════════════════════════════════════════
// GLIDE (PORTAMENTO)
// Exponential interpolation in LOG domain.
// Log domain = equal semitone steps per time (perceptually correct).
// ════════════════════════════════════════════════════════

class GlideProcessor {
public:
    void setSampleRate(float sr) noexcept;
    void setGlideTime(ms_t ms)   noexcept;  // 0 = instant
    void setEnabled(bool on)     noexcept;

    // Triggers glide from current pitch toward hz
    void setTarget(hz_t hz) noexcept;

    // Jump without glide (first note, all-notes-off, etc.)
    void jumpTo(hz_t hz) noexcept;

    // [RT-SAFE] — returns current pitch in Hz, advances state
    hz_t tick() noexcept;

    hz_t getCurrent() const noexcept { return currentHz_; }
    hz_t getTarget()  const noexcept { return targetHz_; }
    bool isGliding()  const noexcept;

private:
    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    ms_t  glideMs_    = 100.0f;
    bool  enabled_    = false;
    hz_t  currentHz_  = 440.0f;
    hz_t  targetHz_   = 440.0f;

    // Pitch in log2 semitone space
    float logCurrent_ = 0.0f;
    float logTarget_  = 0.0f;

    // One-pole coeff: exp(-1 / (glideMs × 0.001 × sr))
    float coeff_      = 0.0f;

    void updateCoeff() noexcept;

    static float hzToLog(hz_t hz) noexcept {
        return std::log2(hz / A4_HZ);
    }
    static hz_t logToHz(float log2val) noexcept {
        return A4_HZ * std::exp2(log2val);
    }
};
