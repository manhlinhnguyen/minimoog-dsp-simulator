// ─────────────────────────────────────────────────────────
// FILE: core/effects/delay_effect.cpp
// BRIEF: Stereo delay implementation
// ─────────────────────────────────────────────────────────
#include "delay_effect.h"
#include <cmath>
#include <algorithm>

void DelayEffect::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    reset();
}

void DelayEffect::reset() noexcept {
    bufL_.fill(0.f);
    bufR_.fill(0.f);
    writePos_ = 0;
}

void DelayEffect::setParam(int id, float v) noexcept {
    switch (id) {
        case 0: time_     = std::clamp(v, 0.01f, 2.f);   break;
        case 1: feedback_ = std::clamp(v, 0.f, 0.95f);   break;
        case 2: mix_      = std::clamp(v, 0.f, 1.f);     break;
        case 3: bpmSync_  = (v > 0.5f) ? 1.f : 0.f;      break;
        case 4: syncDiv_  = std::clamp(v, 0.f, 6.f);     break;
        default: break;
    }
}

float DelayEffect::getParam(int id) const noexcept {
    switch (id) {
        case 0: return time_;
        case 1: return feedback_;
        case 2: return mix_;
        case 3: return bpmSync_;
        case 4: return syncDiv_;
        default: return 0.f;
    }
}

const char* DelayEffect::paramName(int id) const noexcept {
    static const char* names[] = {
        "Time", "Feedback", "Mix", "BPM Sync", "Sync Div"
    };
    return (id >= 0 && id < 5) ? names[id] : "";
}

float DelayEffect::paramMin(int id) const noexcept {
    if (id == 0) return 0.01f;
    if (id == 4) return 0.0f;
    return 0.f;
}

float DelayEffect::paramMax(int id) const noexcept {
    switch (id) {
        case 0: return 2.f;
        case 1: return 0.95f;
        case 2: return 1.f;
        case 3: return 1.f;
        case 4: return 6.f;
        default: return 1.f;
    }
}

float DelayEffect::paramDefault(int id) const noexcept {
    switch (id) {
        case 0: return 0.375f;
        case 1: return 0.4f;
        case 2: return 0.4f;
        case 3: return 0.f;
        case 4: return 3.f;   // 1/8 note
        default: return 0.f;
    }
}

float DelayEffect::readInterp(const std::array<float, MAX_DELAY_SAMPLES>& buf,
                               int delaySamples) const noexcept {
    int rpos = writePos_ - delaySamples;
    rpos = ((rpos % MAX_DELAY_SAMPLES) + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES;
    return buf[rpos];
}

// [RT-SAFE]
void DelayEffect::processBlock(float* L, float* R, int n,
                               const EffectContext& ctx) noexcept {
    int delaySamples = static_cast<int>(time_ * sampleRate_);

    if (bpmSync_ > 0.5f) {
        const float bpm = std::clamp(ctx.bpm, 20.0f, 300.0f);
        static constexpr float kDivBeats[7] = {
            4.0f,        // 1/1
            2.0f,        // 1/2
            1.0f,        // 1/4
            0.5f,        // 1/8
            0.25f,       // 1/16
            1.0f / 3.0f, // 1/8T
            1.5f         // 1/4.
        };
        const int divIdx = std::clamp(static_cast<int>(syncDiv_ + 0.5f), 0, 6);
        const float delaySec = (60.0f / bpm) * kDivBeats[divIdx];
        delaySamples = static_cast<int>(delaySec * sampleRate_);
    }

    if (delaySamples < 1) delaySamples = 1;
    if (delaySamples >= MAX_DELAY_SAMPLES) delaySamples = MAX_DELAY_SAMPLES - 1;

    const float dry = 1.f - mix_;
    const float wet = mix_;

    for (int i = 0; i < n; ++i) {
        const float inL = L[i];
        const float inR = R[i];

        const float dlyL = readInterp(bufL_, delaySamples);
        const float dlyR = readInterp(bufR_, delaySamples);

        bufL_[writePos_] = inL + dlyL * feedback_;
        bufR_[writePos_] = inR + dlyR * feedback_;

        L[i] = dry * inL + wet * dlyL;
        R[i] = dry * inR + wet * dlyR;

        writePos_ = (writePos_ + 1) % MAX_DELAY_SAMPLES;
    }
}
