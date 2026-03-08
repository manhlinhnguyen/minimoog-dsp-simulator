// ─────────────────────────────────────────────────────────
// FILE: core/effects/chorus_effect.cpp
// BRIEF: Stereo chorus effect implementation
// ─────────────────────────────────────────────────────────
#include "chorus_effect.h"
#include <cmath>
#include <algorithm>

void ChorusEffect::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    reset();
}

void ChorusEffect::reset() noexcept {
    delayL_.fill(0.f);
    delayR_.fill(0.f);
    writePos_ = 0;
    for (int v = 0; v < MAX_VOICES; ++v)
        lfoPhase_[v] = static_cast<float>(v) / MAX_VOICES;
}

void ChorusEffect::setParam(int id, float v) noexcept {
    switch (id) {
        case 0: depth_  = std::clamp(v, 0.f, 1.f); break;
        case 1: rate_   = std::clamp(v, 0.1f, 5.f); break;
        case 2: voices_ = std::clamp(v, 1.f, 4.f); break;
        case 3: mix_    = std::clamp(v, 0.f, 1.f); break;
        default: break;
    }
}

float ChorusEffect::getParam(int id) const noexcept {
    switch (id) {
        case 0: return depth_;
        case 1: return rate_;
        case 2: return voices_;
        case 3: return mix_;
        default: return 0.f;
    }
}

const char* ChorusEffect::paramName(int id) const noexcept {
    static const char* names[] = {"Depth", "Rate", "Voices", "Mix"};
    return (id >= 0 && id < 4) ? names[id] : "";
}

float ChorusEffect::paramMin(int id) const noexcept {
    switch (id) {
        case 1: return 0.1f;
        case 2: return 1.f;
        default: return 0.f;
    }
}

float ChorusEffect::paramMax(int id) const noexcept {
    switch (id) {
        case 1: return 5.f;
        case 2: return 4.f;
        default: return 1.f;
    }
}

float ChorusEffect::paramDefault(int id) const noexcept {
    switch (id) {
        case 0: return 0.5f;
        case 1: return 0.5f;
        case 2: return 2.f;
        case 3: return 0.5f;
        default: return 0.f;
    }
}

float ChorusEffect::readDelayInterp(
        const std::array<float, MAX_DELAY_SAMPLES>& buf,
        float delaySamples) const noexcept {
    const float rpos = static_cast<float>(writePos_) - delaySamples;
    int i0 = static_cast<int>(rpos);
    const float frac = rpos - static_cast<float>(i0);

    // Wrap
    i0 = ((i0 % MAX_DELAY_SAMPLES) + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
    const int i1 = (i0 + 1) % MAX_DELAY_SAMPLES;

    return buf[i0] * (1.f - frac) + buf[i1] * frac;
}

// [RT-SAFE]
void ChorusEffect::processBlock(float* L, float* R, int n,
                                const EffectContext& ctx) noexcept {
    (void)ctx;
    const float lfoInc  = rate_ / sampleRate_;
    const float depthSamples = depth_ * 0.030f * sampleRate_;  // max 30ms depth
    const int   numVoices = static_cast<int>(voices_ + 0.5f);
    const float dry = 1.f - mix_;
    const float wet = mix_;

    for (int i = 0; i < n; ++i) {
        const float inL = L[i];
        const float inR = R[i];

        // Write input to delay
        delayL_[writePos_] = inL;
        delayR_[writePos_] = inR;

        float wetL = 0.f, wetR = 0.f;
        for (int v = 0; v < numVoices; ++v) {
            const float lfo = std::sin(lfoPhase_[v] * 6.28318530718f);
            const float dly = depthSamples * (0.5f + 0.5f * lfo) + 5.f;  // min 5 samples
            // Alternate voices L/R for stereo width
            if (v % 2 == 0) {
                wetL += readDelayInterp(delayL_, dly);
                wetR += readDelayInterp(delayR_, dly * 0.95f);
            } else {
                wetL += readDelayInterp(delayL_, dly * 0.95f);
                wetR += readDelayInterp(delayR_, dly);
            }
            lfoPhase_[v] += lfoInc;
            if (lfoPhase_[v] >= 1.f) lfoPhase_[v] -= 1.f;
        }

        const float invV = (numVoices > 0) ? 1.f / numVoices : 1.f;
        wetL *= invV;
        wetR *= invV;

        L[i] = dry * inL + wet * wetL;
        R[i] = dry * inR + wet * wetR;

        writePos_ = (writePos_ + 1) % MAX_DELAY_SAMPLES;
    }
}
