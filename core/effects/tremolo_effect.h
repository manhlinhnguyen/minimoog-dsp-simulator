// ─────────────────────────────────────────────────────────
// FILE: core/effects/tremolo_effect.h
// BRIEF: Amplitude tremolo with selectable LFO waveform
// ─────────────────────────────────────────────────────────
#pragma once
#include "effect_base.h"

// Params: 0=Depth 1=Rate 2=Waveform(0=Sine,1=Triangle,2=Square)
class TremoloEffect : public IEffect {
public:
    void  init(float sampleRate)      noexcept override;
    void  reset()                     noexcept override;
    void  setParam(int id, float v)   noexcept override;
    float getParam(int id) const      noexcept override;
    int   paramCount()     const      noexcept override { return 3; }
    const char* paramName(int id) const noexcept override;
    float paramMin(int id)   const    noexcept override;
    float paramMax(int id)   const    noexcept override;
    float paramDefault(int id) const  noexcept override;
    void  processBlock(float* L, float* R, int n,
                       const EffectContext& ctx) noexcept override;
    EffectType  type()     const      noexcept override { return EffectType::Tremolo; }
    const char* typeName() const      noexcept override { return "Tremolo"; }

private:
    float sampleRate_ = 44100.f;
    float depth_      = 0.5f;
    float rate_       = 4.f;
    float waveform_   = 0.f;   // 0=Sine, 1=Triangle, 2=Square

    float lfoPhase_ = 0.f;

    float lfoValue() const noexcept;
};
