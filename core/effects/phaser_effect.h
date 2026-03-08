// ─────────────────────────────────────────────────────────
// FILE: core/effects/phaser_effect.h
// BRIEF: 4-stage all-pass phaser effect
// ─────────────────────────────────────────────────────────
#pragma once
#include "effect_base.h"

// Params: 0=Depth 1=Rate 2=Feedback 3=Mix
class PhaserEffect : public IEffect {
public:
    static constexpr int NUM_STAGES = 4;

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
    EffectType  type()     const      noexcept override { return EffectType::Phaser; }
    const char* typeName() const      noexcept override { return "Phaser"; }

private:
    float sampleRate_ = 44100.f;
    float depth_      = 0.7f;
    float rate_       = 0.4f;
    float feedback_   = 0.5f;
    float mix_        = 0.5f;

    float lfoPhase_ = 0.f;

    // All-pass state (L and R, 4 stages each)
    float apL_[NUM_STAGES] = {};
    float apR_[NUM_STAGES] = {};

    float fbL_ = 0.f;
    float fbR_ = 0.f;

    // Process one all-pass stage
    static float allpass(float in, float coef, float& state) noexcept;
};
