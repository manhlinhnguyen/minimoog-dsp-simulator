// ─────────────────────────────────────────────────────────
// FILE: core/effects/tremolo_effect.cpp
// BRIEF: Amplitude tremolo implementation
// ─────────────────────────────────────────────────────────
#include "tremolo_effect.h"
#include <cmath>
#include <algorithm>

void TremoloEffect::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    reset();
}

void TremoloEffect::reset() noexcept {
    lfoPhase_ = 0.f;
}

void TremoloEffect::setParam(int id, float v) noexcept {
    switch (id) {
        case 0: depth_    = std::clamp(v, 0.f, 1.f);   break;
        case 1: rate_     = std::clamp(v, 0.1f, 20.f); break;
        case 2: waveform_ = std::clamp(v, 0.f, 2.f);   break;
        default: break;
    }
}

float TremoloEffect::getParam(int id) const noexcept {
    switch (id) {
        case 0: return depth_;
        case 1: return rate_;
        case 2: return waveform_;
        default: return 0.f;
    }
}

const char* TremoloEffect::paramName(int id) const noexcept {
    static const char* names[] = {"Depth", "Rate", "Waveform"};
    return (id >= 0 && id < 3) ? names[id] : "";
}

float TremoloEffect::paramMin(int id) const noexcept {
    if (id == 1) return 0.1f;
    return 0.f;
}

float TremoloEffect::paramMax(int id) const noexcept {
    switch (id) {
        case 1: return 20.f;
        case 2: return 2.f;
        default: return 1.f;
    }
}

float TremoloEffect::paramDefault(int id) const noexcept {
    switch (id) {
        case 0: return 0.5f;
        case 1: return 4.f;
        case 2: return 0.f;
        default: return 0.f;
    }
}

float TremoloEffect::lfoValue() const noexcept {
    const int wave = static_cast<int>(waveform_ + 0.5f);
    if (wave == 0) {
        // Sine
        return std::sin(lfoPhase_ * 6.28318530718f);
    } else if (wave == 1) {
        // Triangle
        const float p = lfoPhase_;
        return (p < 0.5f) ? (4.f * p - 1.f) : (3.f - 4.f * p);
    } else {
        // Square
        return (lfoPhase_ < 0.5f) ? 1.f : -1.f;
    }
}

// [RT-SAFE]
void TremoloEffect::processBlock(float* L, float* R, int n) noexcept {
    const float lfoInc = rate_ / sampleRate_;
    for (int i = 0; i < n; ++i) {
        const float lfo = lfoValue();
        // depth_=0 → no modulation, depth_=1 → full amplitude swing
        const float gain = 1.f - depth_ * (0.5f + 0.5f * lfo);
        L[i] *= gain;
        R[i] *= gain;

        lfoPhase_ += lfoInc;
        if (lfoPhase_ >= 1.f) lfoPhase_ -= 1.f;
    }
}
