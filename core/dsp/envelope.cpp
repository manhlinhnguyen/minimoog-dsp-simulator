// ─────────────────────────────────────────────────────────
// FILE: core/dsp/envelope.cpp
// BRIEF: ADSR control envelope implementation
// ─────────────────────────────────────────────────────────
#include "envelope.h"

void ControlEnvelope::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    recalcCoeffs();
}

void ControlEnvelope::setParams(const Params& p) noexcept {
    p_ = p;
    recalcCoeffs();
}

void ControlEnvelope::setAttack(ms_t ms) noexcept {
    p_.attack = ms < 0.01f ? 0.01f : ms;
    recalcCoeffs();
}

void ControlEnvelope::setDecay(ms_t ms) noexcept {
    p_.decay = ms < 0.01f ? 0.01f : ms;
    recalcCoeffs();
}

void ControlEnvelope::setSustain(float lv) noexcept {
    p_.sustain = lv < 0.0f ? 0.0f : (lv > 1.0f ? 1.0f : lv);
}

void ControlEnvelope::setRelease(ms_t ms) noexcept {
    p_.release = ms < 0.01f ? 0.01f : ms;
    recalcCoeffs();
}

void ControlEnvelope::recalcCoeffs() noexcept {
    // Attack: linear rise (1 / samples)
    const float atkSamples = p_.attack * 0.001f * sampleRate_;
    atkInc_   = (atkSamples > 0.0f) ? (1.0f / atkSamples) : 1.0f;
    // Decay / Release: exponential one-pole
    decCoeff_ = calcExpCoeff(p_.decay);
    relCoeff_ = calcExpCoeff(p_.release);
}

void ControlEnvelope::noteOn() noexcept {
    // Retrigger from current level — no click, no reset to zero
    stage_ = Stage::Attack;
}

void ControlEnvelope::noteOff() noexcept {
    if (stage_ != Stage::Idle)
        stage_ = Stage::Release;
}

void ControlEnvelope::reset() noexcept {
    stage_ = Stage::Idle;
    level_ = 0.0f;
}

float ControlEnvelope::tick() noexcept {  // [RT-SAFE]
    switch (stage_) {

        case Stage::Attack:
            level_ += atkInc_;
            if (level_ >= 1.0f) {
                level_ = 1.0f;
                stage_ = Stage::Decay;
            }
            break;

        case Stage::Decay:
            // Exponential decay toward sustain level
            level_ = p_.sustain + (level_ - p_.sustain) * decCoeff_;
            if (level_ <= p_.sustain + 0.0001f) {
                level_ = p_.sustain;
                stage_ = Stage::Sustain;
            }
            break;

        case Stage::Sustain:
            level_ = p_.sustain;
            break;

        case Stage::Release:
            level_ *= relCoeff_;
            if (level_ <= 0.0001f) {
                level_ = 0.0f;
                stage_ = Stage::Idle;
            }
            break;

        case Stage::Idle:
        default:
            level_ = 0.0f;
            break;
    }
    return level_;
}
