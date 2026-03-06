// ─────────────────────────────────────────────────────────
// FILE: core/dsp/param_smoother.h
// BRIEF: One-pole parameter smoother — eliminates zipper noise
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include <cmath>

// ════════════════════════════════════════════════════════
// ONE-POLE PARAMETER SMOOTHER
// Apply to: cutoff, resonance, volume, any continuous param.
// NOT needed for: switches, discrete selectors.
// Default smooth time: 5ms
// ════════════════════════════════════════════════════════

class ParamSmoother {
public:
    void init(float sampleRate, ms_t smoothMs = 5.0f) noexcept {
        sampleRate_ = sampleRate;
        setSmoothTime(smoothMs);
    }

    void setSmoothTime(ms_t ms) noexcept {
        ms = ms < 0.01f ? 0.01f : ms;
        coeff_ = std::exp(-1.0f / (ms * 0.001f * sampleRate_));
    }

    void setTarget(float val) noexcept { target_ = val; }

    // Instant jump — use on preset load
    void snapTo(float val) noexcept { current_ = target_ = val; }

    // [RT-SAFE] — returns smoothed value, advances state
    float tick() noexcept {
        current_ += (target_ - current_) * (1.0f - coeff_);
        return current_;
    }

    float getCurrent() const noexcept { return current_; }
    float getTarget()  const noexcept { return target_; }

    bool isSettled() const noexcept {
        return std::abs(target_ - current_) < 1e-5f;
    }

private:
    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    float coeff_      = 0.0f;
    float current_    = 0.0f;
    float target_     = 0.0f;
};
