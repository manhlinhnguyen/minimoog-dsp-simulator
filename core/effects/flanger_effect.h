// ─────────────────────────────────────────────────────────
// FILE: core/effects/flanger_effect.h
// BRIEF: Stereo flanger effect — pre-allocated delay line
// ─────────────────────────────────────────────────────────
#pragma once
#include "effect_base.h"
#include <array>

// Params: 0=Depth 1=Rate 2=Feedback 3=Mix
class FlangerEffect : public IEffect {
public:
    static constexpr int MAX_DELAY_SAMPLES = 2048;  // ~46ms @ 44100

    void  init(float sampleRate)      noexcept override;
    void  reset()                     noexcept override;
    void  setParam(int id, float v)   noexcept override;
    float getParam(int id) const      noexcept override;
    int   paramCount()     const      noexcept override { return 4; }
    const char* paramName(int id) const noexcept override;
    float paramMin(int id)   const    noexcept override;
    float paramMax(int id)   const    noexcept override;
    float paramDefault(int id) const  noexcept override;
    void  processBlock(float* L, float* R, int n,
                       const EffectContext& ctx) noexcept override;
    EffectType  type()     const      noexcept override { return EffectType::Flanger; }
    const char* typeName() const      noexcept override { return "Flanger"; }

private:
    float sampleRate_ = 44100.f;
    float depth_      = 0.7f;
    float rate_       = 0.3f;
    float feedback_   = 0.5f;
    float mix_        = 0.5f;

    float lfoPhase_ = 0.f;

    std::array<float, MAX_DELAY_SAMPLES> delayL_{};
    std::array<float, MAX_DELAY_SAMPLES> delayR_{};
    int writePos_ = 0;

    float readInterp(const std::array<float, MAX_DELAY_SAMPLES>& buf,
                     float delaySamples) const noexcept;
};
