// ─────────────────────────────────────────────────────────
// FILE: core/effects/flanger_effect.cpp
// BRIEF: Stereo flanger effect implementation
// ─────────────────────────────────────────────────────────
#include "flanger_effect.h"
#include <cmath>
#include <algorithm>

void FlangerEffect::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    reset();
}

void FlangerEffect::reset() noexcept {
    delayL_.fill(0.f);
    delayR_.fill(0.f);
    writePos_ = 0;
    lfoPhase_ = 0.f;
}

void FlangerEffect::setParam(int id, float v) noexcept {
    switch (id) {
        case 0: depth_    = std::clamp(v, 0.f, 1.f);   break;
        case 1: rate_     = std::clamp(v, 0.05f, 4.f); break;
        case 2: feedback_ = std::clamp(v, -0.95f, 0.95f); break;
        case 3: mix_      = std::clamp(v, 0.f, 1.f);   break;
        default: break;
    }
}

float FlangerEffect::getParam(int id) const noexcept {
    switch (id) {
        case 0: return depth_;
        case 1: return rate_;
        case 2: return feedback_;
        case 3: return mix_;
        default: return 0.f;
    }
}

const char* FlangerEffect::paramName(int id) const noexcept {
    static const char* names[] = {"Depth", "Rate", "Feedback", "Mix"};
    return (id >= 0 && id < 4) ? names[id] : "";
}

float FlangerEffect::paramMin(int id) const noexcept {
    switch (id) {
        case 1: return 0.05f;
        case 2: return -0.95f;
        default: return 0.f;
    }
}

float FlangerEffect::paramMax(int id) const noexcept {
    switch (id) {
        case 1: return 4.f;
        case 2: return 0.95f;
        default: return 1.f;
    }
}

float FlangerEffect::paramDefault(int id) const noexcept {
    switch (id) {
        case 0: return 0.7f;
        case 1: return 0.3f;
        case 2: return 0.5f;
        case 3: return 0.5f;
        default: return 0.f;
    }
}

float FlangerEffect::readInterp(
        const std::array<float, MAX_DELAY_SAMPLES>& buf,
        float delaySamples) const noexcept {
    const float rpos = static_cast<float>(writePos_) - delaySamples;
    int i0 = static_cast<int>(rpos);
    const float frac = rpos - static_cast<float>(i0);
    i0 = ((i0 % MAX_DELAY_SAMPLES) + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
    const int i1 = (i0 + 1) % MAX_DELAY_SAMPLES;
    return buf[i0] * (1.f - frac) + buf[i1] * frac;
}

// [RT-SAFE]
void FlangerEffect::processBlock(float* L, float* R, int n,
                                 const EffectContext& ctx) noexcept {
    (void)ctx;
    const float lfoInc    = rate_ / sampleRate_;
    const float maxDelay  = depth_ * 0.010f * sampleRate_;  // 0..10ms
    const float dry = 1.f - mix_;
    const float wet = mix_;

    for (int i = 0; i < n; ++i) {
        const float inL = L[i];
        const float inR = R[i];

        const float lfo  = std::sin(lfoPhase_ * 6.28318530718f);
        const float dlyL = maxDelay * (0.5f + 0.5f * lfo) + 1.f;
        const float dlyR = maxDelay * (0.5f - 0.5f * lfo) + 1.f;  // opposite phase → stereo

        const float outL = readInterp(delayL_, dlyL);
        const float outR = readInterp(delayR_, dlyR);

        delayL_[writePos_] = inL + outL * feedback_;
        delayR_[writePos_] = inR + outR * feedback_;

        L[i] = dry * inL + wet * outL;
        R[i] = dry * inR + wet * outR;

        writePos_ = (writePos_ + 1) % MAX_DELAY_SAMPLES;

        lfoPhase_ += lfoInc;
        if (lfoPhase_ >= 1.f) lfoPhase_ -= 1.f;
    }
}
