// ─────────────────────────────────────────────────────────
// FILE: core/dsp/envelope.h
// BRIEF: ADSR control envelope (linear attack, exponential D/R)
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include <cmath>

class ControlEnvelope {
public:
    enum class Stage : uint8_t {
        Idle = 0, Attack, Decay, Sustain, Release
    };

    struct Params {
        ms_t  attack  =    10.0f;
        ms_t  decay   =   200.0f;
        float sustain =     0.7f;
        ms_t  release =   500.0f;
    };

    void setSampleRate(float sr) noexcept;
    void setParams(const Params& p) noexcept;
    void setAttack(ms_t ms)   noexcept;
    void setDecay(ms_t ms)    noexcept;
    void setSustain(float lv) noexcept;
    void setRelease(ms_t ms)  noexcept;

    void noteOn()  noexcept;   // retrigger-safe (no click)
    void noteOff() noexcept;
    void reset()   noexcept;   // instant silence

    // [RT-SAFE]
    float tick() noexcept;

    Stage getStage()  const noexcept { return stage_; }
    float getLevel()  const noexcept { return level_; }
    bool  isActive()  const noexcept { return stage_ != Stage::Idle; }

private:
    float  sampleRate_ = SAMPLE_RATE_DEFAULT;
    Params p_;
    Stage  stage_      = Stage::Idle;
    float  level_      = 0.0f;

    float  atkInc_     = 0.0f;
    float  decCoeff_   = 0.0f;
    float  relCoeff_   = 0.0f;

    void recalcCoeffs() noexcept;

    // One-pole exponential coefficient: time to reach 1/e of distance
    float calcExpCoeff(ms_t ms) const noexcept {
        if (ms < 0.01f) return 0.0f;
        return std::exp(-1.0f / (ms * 0.001f * sampleRate_));
    }
};
