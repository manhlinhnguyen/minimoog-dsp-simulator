// ─────────────────────────────────────────────────────────
// FILE: core/effects/reverb_effect.cpp
// BRIEF: Schroeder comb+allpass stereo reverb implementation
// ─────────────────────────────────────────────────────────
#include "reverb_effect.h"
#include <cmath>
#include <algorithm>
#include <cstring>

// Static constexpr array definitions
constexpr int ReverbEffect::COMB_SIZES_L[NUM_COMBS];
constexpr int ReverbEffect::COMB_SIZES_R[NUM_COMBS];
constexpr int ReverbEffect::AP_SIZES[NUM_ALLPASS];

void ReverbEffect::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    reset();
}

void ReverbEffect::reset() noexcept {
    for (int c = 0; c < NUM_COMBS; ++c) {
        combBufL_[c].fill(0.f); combBufR_[c].fill(0.f);
        combPosL_[c] = combPosR_[c] = 0;
        combFilterL_[c] = combFilterR_[c] = 0.f;
    }
    for (int a = 0; a < NUM_ALLPASS; ++a) {
        apBufL_[a].fill(0.f); apBufR_[a].fill(0.f);
        apPosL_[a] = apPosR_[a] = 0;
    }
    preDelayBufL_.fill(0.f);
    preDelayBufR_.fill(0.f);
    preDelayPos_ = 0;
}

void ReverbEffect::setParam(int id, float v) noexcept {
    switch (id) {
        case 0: size_     = std::clamp(v, 0.f, 1.f);    break;
        case 1: decay_    = std::clamp(v, 0.f, 0.99f);  break;
        case 2: damping_  = std::clamp(v, 0.f, 1.f);    break;
        case 3: preDelay_ = std::clamp(v, 0.f, 0.1f);   break;
        case 4: mix_      = std::clamp(v, 0.f, 1.f);    break;
        default: break;
    }
}

float ReverbEffect::getParam(int id) const noexcept {
    switch (id) {
        case 0: return size_;
        case 1: return decay_;
        case 2: return damping_;
        case 3: return preDelay_;
        case 4: return mix_;
        default: return 0.f;
    }
}

const char* ReverbEffect::paramName(int id) const noexcept {
    static const char* names[] = {"Size", "Decay", "Damping", "Pre-Delay", "Mix"};
    return (id >= 0 && id < 5) ? names[id] : "";
}

float ReverbEffect::paramMin(int id) const noexcept {
    (void)id;
    return 0.f;
}

float ReverbEffect::paramMax(int id) const noexcept {
    switch (id) {
        case 1: return 0.99f;
        case 3: return 0.1f;
        default: return 1.f;
    }
}

float ReverbEffect::paramDefault(int id) const noexcept {
    switch (id) {
        case 0: return 0.6f;
        case 1: return 0.5f;
        case 2: return 0.4f;
        case 3: return 0.02f;
        case 4: return 0.35f;
        default: return 0.f;
    }
}

float ReverbEffect::processComb(float in,
                                 std::array<float, MAX_COMB>& buf,
                                 int& pos, int size,
                                 float& lpState,
                                 float feedback, float damp) noexcept {
    // Clamp size to valid range
    if (size < 1) size = 1;
    if (size >= MAX_COMB) size = MAX_COMB - 1;

    const float out = buf[pos];
    // Damping LP filter inside comb feedback
    lpState = lpState * damp + out * (1.f - damp);
    buf[pos] = in + lpState * feedback;
    pos = (pos + 1) % size;
    return out;
}

float ReverbEffect::processAllpass(float in,
                                    std::array<float, MAX_ALLPASS>& buf,
                                    int& pos, int size) noexcept {
    if (size < 1) size = 1;
    if (size >= MAX_ALLPASS) size = MAX_ALLPASS - 1;

    const float bufOut = buf[pos];
    const float out    = -in + bufOut;
    buf[pos] = in + bufOut * 0.5f;
    pos = (pos + 1) % size;
    return out;
}

// [RT-SAFE]
void ReverbEffect::processBlock(float* L, float* R, int n) noexcept {
    const float feedback = decay_ * 0.95f;
    const float damp     = damping_;
    const float dry      = 1.f - mix_;
    const float wet      = mix_;

    // Scale comb sizes by room size
    const float sizeScale = 0.5f + size_ * 0.5f;
    int combSizesL[NUM_COMBS], combSizesR[NUM_COMBS];
    for (int c = 0; c < NUM_COMBS; ++c) {
        combSizesL[c] = static_cast<int>(COMB_SIZES_L[c] * sizeScale);
        combSizesR[c] = static_cast<int>(COMB_SIZES_R[c] * sizeScale);
        if (combSizesL[c] >= MAX_COMB) combSizesL[c] = MAX_COMB - 1;
        if (combSizesR[c] >= MAX_COMB) combSizesR[c] = MAX_COMB - 1;
    }

    int pdSamples = static_cast<int>(preDelay_ * sampleRate_);
    if (pdSamples < 0) pdSamples = 0;
    if (pdSamples >= MAX_PREDELAY) pdSamples = MAX_PREDELAY - 1;

    for (int i = 0; i < n; ++i) {
        const float inL = L[i];
        const float inR = R[i];

        // Pre-delay
        preDelayBufL_[preDelayPos_] = inL;
        preDelayBufR_[preDelayPos_] = inR;
        int pdRd = preDelayPos_ - pdSamples;
        pdRd = ((pdRd % MAX_PREDELAY) + MAX_PREDELAY) % MAX_PREDELAY;
        const float pdL = preDelayBufL_[pdRd];
        const float pdR = preDelayBufR_[pdRd];
        preDelayPos_ = (preDelayPos_ + 1) % MAX_PREDELAY;

        // Parallel comb filters
        float revL = 0.f, revR = 0.f;
        for (int c = 0; c < NUM_COMBS; ++c) {
            revL += processComb(pdL, combBufL_[c], combPosL_[c],
                                combSizesL[c], combFilterL_[c], feedback, damp);
            revR += processComb(pdR, combBufR_[c], combPosR_[c],
                                combSizesR[c], combFilterR_[c], feedback, damp);
        }
        revL *= 0.25f;
        revR *= 0.25f;

        // Series all-pass
        for (int a = 0; a < NUM_ALLPASS; ++a) {
            revL = processAllpass(revL, apBufL_[a], apPosL_[a], AP_SIZES[a]);
            revR = processAllpass(revR, apBufR_[a], apPosR_[a], AP_SIZES[a]);
        }

        L[i] = dry * inL + wet * revL;
        R[i] = dry * inR + wet * revR;
    }
}
