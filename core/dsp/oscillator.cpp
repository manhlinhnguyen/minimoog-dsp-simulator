// ─────────────────────────────────────────────────────────
// FILE: core/dsp/oscillator.cpp
// BRIEF: Band-limited oscillator implementation
// ─────────────────────────────────────────────────────────
#include "oscillator.h"
#include "core/util/math_utils.h"
#include <cmath>

void Oscillator::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    updateEffectiveHz();
}

void Oscillator::setFrequency(hz_t hz) noexcept {
    freq_ = clamp(hz, 0.001f, 20000.0f);
    updateEffectiveHz();
}

void Oscillator::setWaveShape(WaveShape s) noexcept {
    shape_ = s;
}

void Oscillator::setRange(OscRange r) noexcept {
    range_ = r;
    updateEffectiveHz();
}

void Oscillator::setAmplitude(float a) noexcept {
    amplitude_ = clamp(a, 0.0f, 1.0f);
}

void Oscillator::setPulseWidth(float pw) noexcept {
    pulseWidth_ = clamp(pw, 0.1f, 0.9f);
}

void Oscillator::hardSyncTrigger() noexcept {
    syncPending_ = true;
}

void Oscillator::reset() noexcept {
    phase_       = 0.0f;
    syncPending_ = false;
}

void Oscillator::updateEffectiveHz() noexcept {
    effectiveHz_ = freq_ * RANGE_MULT[static_cast<int>(range_)];
}

// PolyBLEP correction residual near a discontinuity.
// t  = fractional phase at the discontinuity point
// dt = phase increment per sample (freq / sampleRate)
// Reference: Välimäki & Pekonen (2007)
float Oscillator::polyblep(float t, float dt) const noexcept {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    } else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

sample_t Oscillator::renderSawtooth() const noexcept {
    const float dt = effectiveHz_ / sampleRate_;
    float value = 2.0f * phase_ - 1.0f;  // raw bipolar saw
    value -= polyblep(phase_, dt);        // correct at phase=0
    return value;
}

sample_t Oscillator::renderReverseSaw() const noexcept {
    const float dt = effectiveHz_ / sampleRate_;
    float value = 1.0f - 2.0f * phase_;  // inverted saw
    value += polyblep(phase_, dt);
    return value;
}

sample_t Oscillator::renderSquare() const noexcept {
    const float dt = effectiveHz_ / sampleRate_;
    float value = (phase_ < pulseWidth_) ? 1.0f : -1.0f;
    value += polyblep(phase_, dt);
    value -= polyblep(std::fmod(phase_ - pulseWidth_ + 1.0f, 1.0f), dt);
    return value;
}

sample_t Oscillator::renderTriangle() noexcept {
    // Analytic triangle: 4|phase - 0.5| - 1
    // No PolyBLEP needed (triangle has no discontinuity in amplitude)
    return 2.0f * std::abs(2.0f * phase_ - 1.0f) - 1.0f;
}

sample_t Oscillator::renderTriSaw() noexcept {
    // Crossfade triangle (phase<0.5) and saw (phase>=0.5)
    if (phase_ < 0.5f)
        return 4.0f * phase_ - 1.0f;  // rising ramp
    else
        return 3.0f - 4.0f * phase_;  // falling edge
}

sample_t Oscillator::tick() noexcept {  // [RT-SAFE]
    if (syncPending_) {
        phase_       = 0.0f;
        syncPending_ = false;
    }

    sample_t out;
    switch (shape_) {
        case WaveShape::Sawtooth:   out = renderSawtooth();   break;
        case WaveShape::ReverseSaw: out = renderReverseSaw(); break;
        case WaveShape::Square:     out = renderSquare();     break;
        case WaveShape::WidePulse: {
            const float saved = pulseWidth_;
            pulseWidth_       = 0.1f;
            out               = renderSquare();
            pulseWidth_       = saved;
            break;
        }
        case WaveShape::Triangle:   out = renderTriangle();   break;
        case WaveShape::TriSaw:     out = renderTriSaw();     break;
        default:                    out = 0.0f;               break;
    }

    const float dt = effectiveHz_ / sampleRate_;
    phase_ += dt;
    if (phase_ >= 1.0f) phase_ -= 1.0f;

    return out * amplitude_;
}
