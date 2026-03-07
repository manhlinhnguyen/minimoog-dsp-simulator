// ─────────────────────────────────────────────────────────
// FILE: core/effects/gain_effect.cpp
// BRIEF: Gain/Overdrive/Distortion effect implementation
// ─────────────────────────────────────────────────────────
#include "gain_effect.h"
#include <cmath>
#include <algorithm>

void GainEffect::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    reset();
}

void GainEffect::reset() noexcept {
    lpStateL_ = lpStateR_ = 0.f;
}

void GainEffect::setParam(int id, float v) noexcept {
    switch (id) {
        case 0: mode_      = std::clamp(v, 0.f, 2.f); break;
        case 1: gain_      = std::clamp(v, 0.f, 4.f); break;
        case 2: asymmetry_ = std::clamp(v, -1.f, 1.f); break;
        case 3: level_     = std::clamp(v, 0.f, 1.f); break;
        case 4: tone_      = std::clamp(v, 0.f, 1.f); break;
        default: break;
    }
}

float GainEffect::getParam(int id) const noexcept {
    switch (id) {
        case 0: return mode_;
        case 1: return gain_;
        case 2: return asymmetry_;
        case 3: return level_;
        case 4: return tone_;
        default: return 0.f;
    }
}

const char* GainEffect::paramName(int id) const noexcept {
    static const char* names[] = {"Mode", "Gain", "Asymmetry", "Level", "Tone"};
    return (id >= 0 && id < 5) ? names[id] : "";
}

float GainEffect::paramMin(int id) const noexcept {
    switch (id) {
        case 0: return 0.f;
        case 1: return 0.f;
        case 2: return -1.f;
        case 3: return 0.f;
        case 4: return 0.f;
        default: return 0.f;
    }
}

float GainEffect::paramMax(int id) const noexcept {
    switch (id) {
        case 0: return 2.f;
        case 1: return 4.f;
        case 2: return 1.f;
        case 3: return 1.f;
        case 4: return 1.f;
        default: return 1.f;
    }
}

float GainEffect::paramDefault(int id) const noexcept {
    switch (id) {
        case 0: return 0.f;   // Boost
        case 1: return 1.f;   // Unity gain
        case 2: return 0.f;   // No asymmetry
        case 3: return 1.f;   // Full level
        case 4: return 1.f;   // Full tone (bright)
        default: return 0.f;
    }
}

// [RT-SAFE]
float GainEffect::saturate(float x) const noexcept {
    const int m = static_cast<int>(mode_ + 0.5f);
    if (m == 0) {
        // Boost — soft clip via tanh approximation
        if (x >  3.f) return  1.f;
        if (x < -3.f) return -1.f;
        const float x2 = x * x;
        return x * (27.f + x2) / (27.f + 9.f * x2);
    } else if (m == 1) {
        // Overdrive — asymmetric soft clip
        const float bias = x + asymmetry_ * 0.2f;
        if (bias >= 1.f)  return  1.f;
        if (bias <= -1.f) return -1.f;
        return 1.5f * bias - 0.5f * bias * bias * bias;
    } else {
        // Distortion — hard waveshaper
        const float bias = x + asymmetry_ * 0.3f;
        const float hard = std::clamp(bias * gain_, -1.f, 1.f);
        // Fold-back for extra harmonics
        if (hard >  0.6f) return  0.6f + (hard - 0.6f) * 0.3f;
        if (hard < -0.6f) return -0.6f + (hard + 0.6f) * 0.3f;
        return hard;
    }
}

// [RT-SAFE]
void GainEffect::processBlock(float* L, float* R, int n) noexcept {
    // Tone LP coefficient (simple 1-pole)
    const float lpCoef = 1.f - tone_ * 0.95f;  // 0=full LP, 1=full bypass
    for (int i = 0; i < n; ++i) {
        float l = L[i] * gain_;
        float r = R[i] * gain_;

        l = saturate(l);
        r = saturate(r);

        // Tone filter (blend LP)
        lpStateL_ += lpCoef * (l - lpStateL_);
        lpStateR_ += lpCoef * (r - lpStateR_);
        // tone_=1 → bypass LP (bright), tone_=0 → full LP (dark)
        l = l * tone_ + lpStateL_ * (1.f - tone_);
        r = r * tone_ + lpStateR_ * (1.f - tone_);

        L[i] = l * level_;
        R[i] = r * level_;
    }
}
