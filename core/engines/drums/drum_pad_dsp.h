// ─────────────────────────────────────────────────────────
// FILE: core/engines/drums/drum_pad_dsp.h
// BRIEF: DSP synthesis drum pads (808-style, pads 1-8)
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include "core/dsp/noise.h"
#include <cmath>

// ════════════════════════════════════════════════════════
// DrumPadDsp — synthesised percussion voice
// Type determines synthesis model.
// ════════════════════════════════════════════════════════

enum class DspDrumType : int {
    Kick    = 0,   // Sine pitch sweep
    Snare   = 1,   // Osc + noise
    HiHatC  = 2,   // 6 squares + BP filter (closed)
    HiHatO  = 3,   // Same + longer decay (open)
    Clap    = 4,   // Noise bursts
    TomLow  = 5,   // Pitch sweep, lower
    TomMid  = 6,   // Pitch sweep, mid
    Rimshot = 7,   // Transient click + ring
    COUNT   = 8
};

struct DrumPadDsp {
    DspDrumType type   = DspDrumType::Kick;
    bool        active = false;

    // General-purpose oscillators
    float phase1 = 0.0f, phase2 = 0.0f;
    float freq1  = 60.0f, freq2 = 200.0f;
    float freqDecayRate = 0.9999f;  // pitch sweep

    // Amplitude envelope
    float ampEnv  = 0.0f;
    float decayRate = 0.999f;

    // Per-pad params (set from DrumEngine)
    float volume  = 0.8f;
    float pitch   = 0.5f;    // 0..1 → base freq range
    float decay   = 0.5f;    // 0..1 → decay time
    float pan     = 0.5f;    // 0=left 1=right

    // Kick-specific sweep params (set from DrumEngine global)
    float sweepDepth = 0.5f; // 0..1 → sweep range multiplier
    float sweepTime  = 0.2f; // 0..1 → 10ms..200ms

    float sampleRate = SAMPLE_RATE_DEFAULT;

    void trigger(int velocity) noexcept;
    void setSampleRate(float sr) noexcept { sampleRate = sr; }

    // [RT-SAFE]
    void tick(float& outL, float& outR) noexcept;

private:
    NoiseGenerator noise_;

    float nextNoise() noexcept { return noise_.tick(); }
};
