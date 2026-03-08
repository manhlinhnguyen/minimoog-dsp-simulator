// ─────────────────────────────────────────────────────────
// FILE: core/effects/gain_effect.h
// BRIEF: Gain/Overdrive/Distortion effect
// ─────────────────────────────────────────────────────────
#pragma once
#include "effect_base.h"

// Params: 0=Mode(0=Boost,1=Overdrive,2=Distortion) 1=Gain 2=Asymmetry 3=Level 4=Tone
class GainEffect : public IEffect {
public:
    void  init(float sampleRate)      noexcept override;
    void  reset()                     noexcept override;
    void  setParam(int id, float v)   noexcept override;
    float getParam(int id) const      noexcept override;
    int   paramCount()     const      noexcept override { return 5; }
    const char* paramName(int id) const noexcept override;
    float paramMin(int id)   const    noexcept override;
    float paramMax(int id)   const    noexcept override;
    float paramDefault(int id) const  noexcept override;
    void  processBlock(float* L, float* R, int n,
                       const EffectContext& ctx) noexcept override;
    EffectType  type()     const      noexcept override { return EffectType::Gain; }
    const char* typeName() const      noexcept override { return "Gain"; }

private:
    float sampleRate_ = 44100.f;
    float mode_       = 0.f;   // 0=Boost, 1=Overdrive, 2=Distortion
    float gain_       = 1.f;   // 0..4
    float asymmetry_  = 0.f;   // -1..+1
    float level_      = 1.f;   // 0..1
    float tone_       = 1.f;   // 0..1  (LP filter blend)

    // Simple 1-pole LP for tone control (per channel)
    float lpStateL_ = 0.f;
    float lpStateR_ = 0.f;

    float saturate(float x) const noexcept;
};
