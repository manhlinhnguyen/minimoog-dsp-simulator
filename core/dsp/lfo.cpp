// ─────────────────────────────────────────────────────────
// FILE: core/dsp/lfo.cpp
// BRIEF: LFO implementation
// ─────────────────────────────────────────────────────────
#include "lfo.h"
#include <cmath>

void LFO::setSampleRate(float sr) noexcept { sampleRate_ = sr; }
void LFO::setShape(LFOShape s)    noexcept { shape_      = s; }
void LFO::sync()                  noexcept { phase_      = 0.0f; }

void LFO::setRate(hz_t hz) noexcept {
    rate_ = hz < 0.01f ? 0.01f : hz;
}

void LFO::setDepth(float d) noexcept {
    depth_ = d < 0.0f ? 0.0f : (d > 1.0f ? 1.0f : d);
}

float LFO::nextRand() noexcept {
    randState_ = randState_ * 1664525u + 1013904223u;
    // Map uint32 → [-1, +1]
    return static_cast<float>(randState_) / 2147483648.0f - 1.0f;
}

float LFO::tick() noexcept {  // [RT-SAFE]
    const float dt = rate_ / sampleRate_;
    float out = 0.0f;

    switch (shape_) {
        case LFOShape::Sine:
            out = std::sin(phase_ * TWO_PI);
            break;

        case LFOShape::Triangle:
            out = 1.0f - 4.0f * std::abs(phase_ - 0.5f);
            break;

        case LFOShape::Sawtooth:
            out = 2.0f * phase_ - 1.0f;
            break;

        case LFOShape::Square:
            out = (phase_ < 0.5f) ? 1.0f : -1.0f;
            break;

        case LFOShape::SandH:
            // Trigger new random value when phase wraps
            if (phase_ < prevPhase_)
                shValue_ = nextRand();
            out = shValue_;
            break;

        default:
            out = 0.0f;
            break;
    }

    prevPhase_ = phase_;
    phase_ += dt;
    if (phase_ >= 1.0f) phase_ -= 1.0f;

    return out * depth_;
}
