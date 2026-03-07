// ─────────────────────────────────────────────────────────
// FILE: core/effects/eq_effect.h
// BRIEF: 5-band parametric equalizer using biquad filters
//   Band 0 : Low Shelf   80 Hz
//   Band 1 : Peak       320 Hz  Q=1.0
//   Band 2 : Peak      1000 Hz  Q=1.0
//   Band 3 : Peak      3500 Hz  Q=1.0
//   Band 4 : High Shelf 10 kHz
// Params: 0-4 = band gain (dB, -12..+12)  5 = output level
//
// Thread safety: pendingCoef_ written by UI thread; coef_ written
// only by audio thread (copied from pending when coefDirty_ is set).
// Protects coef_ from half-written struct reads during processBlock.
// ─────────────────────────────────────────────────────────
#pragma once
#include "effect_base.h"
#include <atomic>

class EqEffect : public IEffect {
public:
    static constexpr int BANDS = 5;

    void  init(float sampleRate)        noexcept override;
    void  reset()                       noexcept override;
    void  setParam(int id, float v)     noexcept override;
    float getParam(int id)  const       noexcept override;
    int   paramCount()      const       noexcept override { return 6; }
    const char* paramName(int id) const noexcept override;
    float paramMin(int id)        const noexcept override;
    float paramMax(int id)        const noexcept override;
    float paramDefault(int id)    const noexcept override;
    void  processBlock(float* L, float* R, int n) noexcept override;

    EffectType  type()     const noexcept override { return EffectType::Equalizer; }
    const char* typeName() const noexcept override { return "Equalizer"; }

private:
    float sampleRate_ = 44100.f;
    float gainDb_[BANDS] = {0.f, 0.f, 0.f, 0.f, 0.f};
    float level_         = 1.f;

    struct BiquadCoef  { float b0, b1, b2, a1, a2; };
    struct BiquadState { float x1, x2, y1, y2; };

    // Double-buffered coefficients:
    //   pendingCoef_ — written by UI thread (setParam) via computeCoef()
    //   coef_        — read/written only by audio thread (processBlock flushes pending)
    BiquadCoef  coef_[BANDS]        = {};
    BiquadCoef  pendingCoef_[BANDS] = {};
    // coefDirty_[b]: set (release) by UI, cleared (relaxed) by audio after copy
    std::atomic<bool> coefDirty_[BANDS];

    BiquadState stateL_[BANDS] = {};
    BiquadState stateR_[BANDS] = {};

    // Computes biquad coefficients into `out` for the given band gain.
    void  computeCoef(int band, BiquadCoef& out) const noexcept;
    float processSample(float x, BiquadState& s, const BiquadCoef& c) const noexcept;

    // Fixed filter frequencies and shape parameters
    static constexpr float FREQ[BANDS] = {80.f, 320.f, 1000.f, 3500.f, 10000.f};
    static constexpr float SHELF_S     = 0.7f;
    static constexpr float PEAK_Q      = 1.0f;
};
