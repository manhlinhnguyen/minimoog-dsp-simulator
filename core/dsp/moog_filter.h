// ─────────────────────────────────────────────────────────
// FILE: core/dsp/moog_filter.h
// BRIEF: 4-pole Moog ladder filter with tanh saturation
// Reference: Huovilainen (2004) nonlinear transistor ladder
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"

// ════════════════════════════════════════════════════════
// MOOG LADDER FILTER
// Characteristic: 4-pole lowpass, -24dB/oct
// Nonlinearity: tanh (models transistor I-V curve)
// Self-oscillation: yes, at resonance ≈ 1.0
// ════════════════════════════════════════════════════════

class MoogLadderFilter {
public:
    // ── Setup ────────────────────────────────────────────
    void setSampleRate(float sr) noexcept;
    void reset()                 noexcept;  // clear state vars

    // ── Parameters (can change every sample) ─────────────
    void setCutoff(hz_t hz)    noexcept;
    void setResonance(float r) noexcept;  // 0.0..1.0

    // ── Process ──────────────────────────────────────────
    // [RT-SAFE]
    sample_t process(sample_t input) noexcept;

    // ── Debug query ──────────────────────────────────────
    hz_t  getCutoff()    const noexcept { return cutoff_; }
    float getResonance() const noexcept { return resonance_; }

private:
    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    hz_t  cutoff_     = 1000.0f;
    float resonance_  = 0.0f;

    // Huovilainen state: 4 capacitor voltages
    float V_[4]  = {0.f, 0.f, 0.f, 0.f};
    float dV_[4] = {0.f, 0.f, 0.f, 0.f};
    float tV_[4] = {0.f, 0.f, 0.f, 0.f};

    // Precomputed on setCutoff() / setSampleRate()
    float VT2_  = 1.0f;       // normalized thermal voltage (unit-amplitude audio)
    float x_    = 0.0f;       // normalized freq
    float g_    = 0.0f;       // filter gain
    float res4_ = 0.0f;       // resonance × 4

    void updateCoeffs() noexcept;

    // Padé 3/3 tanh approximation — [RT-SAFE]
    static float tanh_approx(float x) noexcept {
        if (x >  3.0f) return  1.0f;
        if (x < -3.0f) return -1.0f;
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
};
