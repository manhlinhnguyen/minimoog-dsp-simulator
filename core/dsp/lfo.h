// ─────────────────────────────────────────────────────────
// FILE: core/dsp/lfo.h
// BRIEF: Low Frequency Oscillator for pitch/filter modulation
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include <cstdint>

enum class LFOShape : int {
    Sine     = 0,
    Triangle = 1,
    Sawtooth = 2,
    Square   = 3,
    SandH    = 4,   // Sample & Hold (random)
    COUNT    = 5
};

// ════════════════════════════════════════════════════════
// LFO — Low Frequency Oscillator
// Output: -1.0..+1.0 bipolar CV
// Rate:   0.01Hz..20Hz
// Note:   OSC3 can act as LFO (P_OSC3_LFO_ON).
//         In that mode OSC3 bypasses the mixer and feeds
//         directly into ModMatrix as LFO source.
// ════════════════════════════════════════════════════════

class LFO {
public:
    void setSampleRate(float sr) noexcept;
    void setRate(hz_t hz)        noexcept;  // 0.01..20
    void setShape(LFOShape s)    noexcept;
    void setDepth(float d)       noexcept;  // 0.0..1.0

    // Reset phase (MIDI Start / note-on sync)
    void sync() noexcept;

    // [RT-SAFE] — returns depth-scaled output in [-1.0, +1.0]
    float tick() noexcept;

    float getPhase() const noexcept { return phase_; }
    hz_t  getRate()  const noexcept { return rate_; }

private:
    float    sampleRate_ = SAMPLE_RATE_DEFAULT;
    hz_t     rate_       = 1.0f;
    float    depth_      = 1.0f;
    LFOShape shape_      = LFOShape::Sine;
    float    phase_      = 0.0f;    // [0, 1)
    float    prevPhase_  = 0.0f;
    float    shValue_    = 0.0f;    // S&H current hold value

    // LCG for S&H (deterministic, no allocation)
    uint32_t randState_  = 12345u;
    float    nextRand()  noexcept;
};
