// ─────────────────────────────────────────────────────────
// FILE: core/effects/phaser_effect.cpp
// BRIEF: 4-stage all-pass phaser effect implementation
// ─────────────────────────────────────────────────────────
#include "phaser_effect.h"
#include <cmath>
#include <algorithm>

void PhaserEffect::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    reset();
}

void PhaserEffect::reset() noexcept {
    for (int s = 0; s < NUM_STAGES; ++s)
        apL_[s] = apR_[s] = 0.f;
    fbL_ = fbR_ = 0.f;
    lfoPhase_ = 0.f;
}

void PhaserEffect::setParam(int id, float v) noexcept {
    switch (id) {
        case 0: depth_    = std::clamp(v, 0.f, 1.f);     break;
        case 1: rate_     = std::clamp(v, 0.05f, 5.f);   break;
        case 2: feedback_ = std::clamp(v, -0.95f, 0.95f);break;
        case 3: mix_      = std::clamp(v, 0.f, 1.f);     break;
        default: break;
    }
}

float PhaserEffect::getParam(int id) const noexcept {
    switch (id) {
        case 0: return depth_;
        case 1: return rate_;
        case 2: return feedback_;
        case 3: return mix_;
        default: return 0.f;
    }
}

const char* PhaserEffect::paramName(int id) const noexcept {
    static const char* names[] = {"Depth", "Rate", "Feedback", "Mix"};
    return (id >= 0 && id < 4) ? names[id] : "";
}

float PhaserEffect::paramMin(int id) const noexcept {
    switch (id) {
        case 1: return 0.05f;
        case 2: return -0.95f;
        default: return 0.f;
    }
}

float PhaserEffect::paramMax(int id) const noexcept {
    switch (id) {
        case 1: return 5.f;
        case 2: return 0.95f;
        default: return 1.f;
    }
}

float PhaserEffect::paramDefault(int id) const noexcept {
    switch (id) {
        case 0: return 0.7f;
        case 1: return 0.4f;
        case 2: return 0.5f;
        case 3: return 0.5f;
        default: return 0.f;
    }
}

// 1-pole all-pass: y[n] = -a*x[n] + x[n-1] + a*y[n-1]
float PhaserEffect::allpass(float in, float coef, float& state) noexcept {
    const float out = -coef * in + state;
    state = in + coef * out;
    return out;
}

// [RT-SAFE]
void PhaserEffect::processBlock(float* L, float* R, int n,
                                const EffectContext& ctx) noexcept {
    (void)ctx;
    const float lfoInc = rate_ / sampleRate_;
    const float dry = 1.f - mix_;
    const float wet = mix_;

    for (int i = 0; i < n; ++i) {
        const float inL = L[i];
        const float inR = R[i];

        // LFO → sweep all-pass cutoff between 200..8000 Hz
        const float lfo = 0.5f + 0.5f * std::sin(lfoPhase_ * 6.28318530718f);
        const float fHz = 200.f + depth_ * lfo * 7800.f;
        // Bilinear all-pass coefficient
        const float wd = 3.14159265358979f * fHz / sampleRate_;
        const float ap = (wd - 1.f) / (wd + 1.f);

        // Apply 4 all-pass stages (with feedback)
        float sL = inL + fbL_ * feedback_;
        float sR = inR + fbR_ * feedback_;

        for (int s = 0; s < NUM_STAGES; ++s) {
            sL = allpass(sL, ap, apL_[s]);
            sR = allpass(sR, ap, apR_[s]);
        }

        fbL_ = sL;
        fbR_ = sR;

        L[i] = dry * inL + wet * sL;
        R[i] = dry * inR + wet * sR;

        lfoPhase_ += lfoInc;
        if (lfoPhase_ >= 1.f) lfoPhase_ -= 1.f;
    }
}
