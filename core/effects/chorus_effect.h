// ─────────────────────────────────────────────────────────
// FILE: core/effects/chorus_effect.h
// BRIEF: Stereo chorus effect — pre-allocated delay lines
// ─────────────────────────────────────────────────────────
#pragma once
#include "effect_base.h"
#include <array>

// Params: 0=Depth 1=Rate 2=Voices 3=Mix
class ChorusEffect : public IEffect {
public:
    static constexpr int MAX_DELAY_SAMPLES = 4096;  // ~93ms @ 44100
    static constexpr int MAX_VOICES        = 4;

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
    EffectType  type()     const      noexcept override { return EffectType::Chorus; }
    const char* typeName() const      noexcept override { return "Chorus"; }

private:
    float sampleRate_ = 44100.f;
    float depth_      = 0.5f;   // 0..1
    float rate_       = 0.5f;   // 0.1..5 Hz
    float voices_     = 2.f;    // 1..4
    float mix_        = 0.5f;   // 0..1

    // LFO phases per voice (L/R offset for stereo width)
    float lfoPhase_[MAX_VOICES] = {};

    // Delay line (circular buffer, stereo)
    std::array<float, MAX_DELAY_SAMPLES> delayL_{};
    std::array<float, MAX_DELAY_SAMPLES> delayR_{};
    int   writePos_ = 0;

    float readDelayInterp(const std::array<float, MAX_DELAY_SAMPLES>& buf,
                          float delaySamples) const noexcept;
};
