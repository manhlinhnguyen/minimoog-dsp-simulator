// ─────────────────────────────────────────────────────────
// FILE: core/effects/eq_effect.cpp
// BRIEF: 5-band parametric equalizer — biquad implementation
//        based on Audio EQ Cookbook (R. Bristow-Johnson)
// ─────────────────────────────────────────────────────────
#include "eq_effect.h"
#include <cmath>
#include <algorithm>

static constexpr float PI = 3.14159265358979323846f;

// ─────────────────────────────────────────────────────────
// Coefficient computation
// Writes result into `out`; safe to call with pendingCoef_[b]
// from UI thread, or coef_[b] from audio thread during init.
// ─────────────────────────────────────────────────────────
void EqEffect::computeCoef(int band, BiquadCoef& out) const noexcept {
    const float dB    = gainDb_[band];
    const float A     = std::pow(10.f, dB / 40.f);
    const float w0    = 2.f * PI * FREQ[band] / sampleRate_;
    const float cosw  = std::cos(w0);
    const float sinw  = std::sin(w0);

    float b0, b1, b2, a0, a1, a2;

    if (band == 0) {
        // ── Low Shelf ──────────────────────────────────────────
        const float alpha = (sinw / 2.f) *
                            std::sqrt((A + 1.f/A) * (1.f/SHELF_S - 1.f) + 2.f);
        const float sqA2  = 2.f * std::sqrt(A) * alpha;
        b0 =  A * ((A+1.f) - (A-1.f)*cosw + sqA2);
        b1 =  2.f * A * ((A-1.f) - (A+1.f)*cosw);
        b2 =  A * ((A+1.f) - (A-1.f)*cosw - sqA2);
        a0 =       (A+1.f) + (A-1.f)*cosw + sqA2;
        a1 = -2.f * ((A-1.f) + (A+1.f)*cosw);
        a2 =       (A+1.f) + (A-1.f)*cosw - sqA2;

    } else if (band == BANDS - 1) {
        // ── High Shelf ─────────────────────────────────────────
        const float alpha = (sinw / 2.f) *
                            std::sqrt((A + 1.f/A) * (1.f/SHELF_S - 1.f) + 2.f);
        const float sqA2  = 2.f * std::sqrt(A) * alpha;
        b0 =  A * ((A+1.f) + (A-1.f)*cosw + sqA2);
        b1 = -2.f * A * ((A-1.f) + (A+1.f)*cosw);
        b2 =  A * ((A+1.f) + (A-1.f)*cosw - sqA2);
        a0 =       (A+1.f) - (A-1.f)*cosw + sqA2;
        a1 =  2.f * ((A-1.f) - (A+1.f)*cosw);
        a2 =       (A+1.f) - (A-1.f)*cosw - sqA2;

    } else {
        // ── Peak EQ ────────────────────────────────────────────
        const float alpha = sinw / (2.f * PEAK_Q);
        b0 =  1.f + alpha * A;
        b1 = -2.f * cosw;
        b2 =  1.f - alpha * A;
        a0 =  1.f + alpha / A;
        a1 = -2.f * cosw;
        a2 =  1.f - alpha / A;
    }

    const float inv_a0 = 1.f / a0;
    out = { b0*inv_a0, b1*inv_a0, b2*inv_a0, a1*inv_a0, a2*inv_a0 };
}

// ─────────────────────────────────────────────────────────
// IEffect interface
// ─────────────────────────────────────────────────────────

void EqEffect::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    // init() is [RT-UNSAFE] — called before audio starts, no race possible.
    // Write both buffers directly so processBlock sees valid coefs immediately.
    for (int b = 0; b < BANDS; ++b) {
        computeCoef(b, coef_[b]);
        pendingCoef_[b] = coef_[b];
        coefDirty_[b].store(false, std::memory_order_relaxed);
    }
    reset();
}

void EqEffect::reset() noexcept {
    for (int b = 0; b < BANDS; ++b)
        stateL_[b] = stateR_[b] = BiquadState{0.f, 0.f, 0.f, 0.f};
}

// [RT-UNSAFE] — called from UI thread via EffectChain::setSlotParam
void EqEffect::setParam(int id, float v) noexcept {
    if (id >= 0 && id < BANDS) {
        gainDb_[id] = std::clamp(v, -12.f, 12.f);
        // Write to pendingCoef_ only; audio thread flushes to coef_ in processBlock.
        // acquire/release on coefDirty_ ensures pendingCoef_ write is visible
        // to the audio thread before it copies to coef_.
        computeCoef(id, pendingCoef_[id]);
        coefDirty_[id].store(true, std::memory_order_release);
    } else if (id == BANDS) {
        level_ = std::clamp(v, 0.f, 1.f);
    }
}

float EqEffect::getParam(int id) const noexcept {
    if (id >= 0 && id < BANDS) return gainDb_[id];
    if (id == BANDS)            return level_;
    return 0.f;
}

const char* EqEffect::paramName(int id) const noexcept {
    static const char* names[] = {
        "Low", "LowMid", "Mid", "HiMid", "High", "Level"
    };
    return (id >= 0 && id < 6) ? names[id] : "";
}

float EqEffect::paramMin(int id) const noexcept {
    return (id < BANDS) ? -12.f : 0.f;
}

float EqEffect::paramMax(int id) const noexcept {
    return (id < BANDS) ? 12.f : 1.f;
}

float EqEffect::paramDefault(int id) const noexcept {
    return (id < BANDS) ? 0.f : 1.f;
}

// ─────────────────────────────────────────────────────────
// DSP  [RT-SAFE]
// ─────────────────────────────────────────────────────────

float EqEffect::processSample(float x,
                               BiquadState&      s,
                               const BiquadCoef& c) const noexcept {
    const float y = c.b0*x + c.b1*s.x1 + c.b2*s.x2
                            - c.a1*s.y1 - c.a2*s.y2;
    s.x2 = s.x1;  s.x1 = x;
    s.y2 = s.y1;  s.y1 = y;
    return y;
}

// [RT-SAFE]
void EqEffect::processBlock(float* L, float* R, int n) noexcept {
    // Flush pending coefficient updates from UI thread.
    // Acquire load pairs with release store in setParam, ensuring
    // pendingCoef_ writes are visible before we copy to coef_.
    for (int b = 0; b < BANDS; ++b) {
        if (coefDirty_[b].load(std::memory_order_acquire)) {
            coef_[b] = pendingCoef_[b];
            coefDirty_[b].store(false, std::memory_order_relaxed);
        }
    }

    for (int i = 0; i < n; ++i) {
        float l = L[i];
        float r = R[i];
        for (int b = 0; b < BANDS; ++b) {
            l = processSample(l, stateL_[b], coef_[b]);
            r = processSample(r, stateR_[b], coef_[b]);
        }
        L[i] = l * level_;
        R[i] = r * level_;
    }
}
