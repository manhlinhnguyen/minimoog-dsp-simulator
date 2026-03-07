// ─────────────────────────────────────────────────────────
// FILE: core/effects/reverb_effect.h
// BRIEF: Schroeder comb+allpass stereo reverb (pre-allocated)
// ─────────────────────────────────────────────────────────
#pragma once
#include "effect_base.h"
#include <array>

// Params: 0=Size 1=Decay 2=Damping 3=PreDelay 4=Mix
class ReverbEffect : public IEffect {
public:
    static constexpr int NUM_COMBS    = 4;
    static constexpr int NUM_ALLPASS  = 2;
    static constexpr int MAX_COMB     = 4096;   // ~93ms @ 44100
    static constexpr int MAX_ALLPASS  = 1024;
    static constexpr int MAX_PREDELAY = 4410;   // 100ms max

    void  init(float sampleRate)      noexcept override;
    void  reset()                     noexcept override;
    void  setParam(int id, float v)   noexcept override;
    float getParam(int id) const      noexcept override;
    int   paramCount()     const      noexcept override { return 5; }
    const char* paramName(int id) const noexcept override;
    float paramMin(int id)   const    noexcept override;
    float paramMax(int id)   const    noexcept override;
    float paramDefault(int id) const  noexcept override;
    void  processBlock(float* L, float* R, int n) noexcept override;
    EffectType  type()     const      noexcept override { return EffectType::Reverb; }
    const char* typeName() const      noexcept override { return "Reverb"; }

private:
    float sampleRate_ = 44100.f;
    float size_       = 0.6f;    // 0..1
    float decay_      = 0.5f;    // 0..1
    float damping_    = 0.4f;    // 0..1
    float preDelay_   = 0.02f;   // 0..0.1s
    float mix_        = 0.35f;   // 0..1

    // Comb filters — different lengths for L and R (slight detuning for stereo)
    static constexpr int COMB_SIZES_L[NUM_COMBS] = {1657, 1777, 1901, 2053};
    static constexpr int COMB_SIZES_R[NUM_COMBS] = {1617, 1747, 1871, 2017};

    std::array<float, MAX_COMB> combBufL_[NUM_COMBS]{};
    std::array<float, MAX_COMB> combBufR_[NUM_COMBS]{};
    int combPosL_[NUM_COMBS] = {};
    int combPosR_[NUM_COMBS] = {};
    float combFilterL_[NUM_COMBS] = {};
    float combFilterR_[NUM_COMBS] = {};

    // All-pass filters
    static constexpr int AP_SIZES[NUM_ALLPASS] = {557, 441};
    std::array<float, MAX_ALLPASS> apBufL_[NUM_ALLPASS]{};
    std::array<float, MAX_ALLPASS> apBufR_[NUM_ALLPASS]{};
    int apPosL_[NUM_ALLPASS] = {};
    int apPosR_[NUM_ALLPASS] = {};

    // Pre-delay
    std::array<float, MAX_PREDELAY> preDelayBufL_{};
    std::array<float, MAX_PREDELAY> preDelayBufR_{};
    int preDelayPos_ = 0;

    float processComb(float in, std::array<float, MAX_COMB>& buf,
                      int& pos, int size, float& lpState,
                      float feedback, float damp) noexcept;
    float processAllpass(float in, std::array<float, MAX_ALLPASS>& buf,
                         int& pos, int size) noexcept;
};
