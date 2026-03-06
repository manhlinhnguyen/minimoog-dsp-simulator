// ─────────────────────────────────────────────────────────
// FILE: core/dsp/moog_filter.cpp
// BRIEF: Huovilainen nonlinear Moog ladder filter
// ─────────────────────────────────────────────────────────
#include "moog_filter.h"
#include "core/util/math_utils.h"
#include <cmath>

void MoogLadderFilter::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    updateCoeffs();
}

void MoogLadderFilter::setCutoff(hz_t hz) noexcept {
    cutoff_ = clamp(hz, 20.0f, sampleRate_ * 0.49f);
    updateCoeffs();
}

void MoogLadderFilter::setResonance(float r) noexcept {
    resonance_ = clamp(r, 0.0f, 1.0f);
    res4_      = resonance_ * 4.0f;  // feedback gain
}

void MoogLadderFilter::reset() noexcept {
    for (int i = 0; i < 4; ++i)
        V_[i] = dV_[i] = tV_[i] = 0.0f;
}

void MoogLadderFilter::updateCoeffs() noexcept {
    // Normalized angular frequency
    x_ = TWO_PI * cutoff_ / sampleRate_;
    // Huovilainen polynomial gain approximation
    g_ = 0.9892f * x_
       - 0.4324f * x_ * x_
       + 0.1381f * x_ * x_ * x_
       - 0.0202f * x_ * x_ * x_ * x_;
}

sample_t MoogLadderFilter::process(sample_t input) noexcept {  // [RT-SAFE]
    // Scale input by thermal voltage
    const float inp = input / VT2_;

    // Nonlinear feedback: use already-computed tV_[3] = tanh(V_[3]/VT2_)
    const float fb = tanh_approx(inp - res4_ * tV_[3]);

    // 4-stage transistor ladder
    dV_[0] = g_ * (fb    - tV_[0]);  V_[0] += dV_[0];  tV_[0] = tanh_approx(V_[0] / VT2_);
    dV_[1] = g_ * (tV_[0] - tV_[1]); V_[1] += dV_[1];  tV_[1] = tanh_approx(V_[1] / VT2_);
    dV_[2] = g_ * (tV_[1] - tV_[2]); V_[2] += dV_[2];  tV_[2] = tanh_approx(V_[2] / VT2_);
    dV_[3] = g_ * (tV_[2] - tV_[3]); V_[3] += dV_[3];  tV_[3] = tanh_approx(V_[3] / VT2_);

    // V[3] = -24 dB/oct lowpass output
    return V_[3];
}
