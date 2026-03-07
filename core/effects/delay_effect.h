// ─────────────────────────────────────────────────────────
// FILE: core/effects/delay_effect.h
// BRIEF: Stereo delay (max 2s), pre-allocated circular buffer
// ─────────────────────────────────────────────────────────
#pragma once
#include "effect_base.h"
#include <array>

// Params: 0=Time(0..2s) 1=Feedback 2=Mix 3=BpmSync(0/1)
class DelayEffect : public IEffect {
public:
    static constexpr int MAX_DELAY_SAMPLES = 88200;  // 2s @ 44100

    void  init(float sampleRate)      noexcept override;
    void  reset()                     noexcept override;
    void  setParam(int id, float v)   noexcept override;
    float getParam(int id) const      noexcept override;
    int   paramCount()     const      noexcept override { return 4; }
    const char* paramName(int id) const noexcept override;
    float paramMin(int id)   const    noexcept override;
    float paramMax(int id)   const    noexcept override;
    float paramDefault(int id) const  noexcept override;
    void  processBlock(float* L, float* R, int n) noexcept override;
    EffectType  type()     const      noexcept override { return EffectType::Delay; }
    const char* typeName() const      noexcept override { return "Delay"; }

private:
    float sampleRate_ = 44100.f;
    float time_       = 0.375f;  // seconds
    float feedback_   = 0.4f;
    float mix_        = 0.4f;
    float bpmSync_    = 0.f;     // 0=off, 1=on (not yet wired to BPM param)

    std::array<float, MAX_DELAY_SAMPLES> bufL_{};
    std::array<float, MAX_DELAY_SAMPLES> bufR_{};
    int writePos_ = 0;

    float readInterp(const std::array<float, MAX_DELAY_SAMPLES>& buf,
                     int delaySamples) const noexcept;
};
