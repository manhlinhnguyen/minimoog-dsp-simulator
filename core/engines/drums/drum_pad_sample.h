// ─────────────────────────────────────────────────────────
// FILE: core/engines/drums/drum_pad_sample.h
// BRIEF: Sample-playback drum pad (pads 9-16)
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include <vector>
#include <string>

// ════════════════════════════════════════════════════════
// DrumPadSample — PCM playback pad
// WAV is loaded by drum_sample_loader (HAL layer).
// The audio thread only reads: no alloc in tick().
// ════════════════════════════════════════════════════════

struct DrumPadSample {
    // Loaded by HAL — pre-allocated at init, read-only in audio thread
    std::vector<float> audioL;   // left channel (mono if audioR empty)
    std::vector<float> audioR;   // right channel (empty = mono)
    float              sampleRate = 44100.0f;
    std::string        name;     // display name (UI thread only)
    bool               loaded = false;

    // Per-pad params
    float volume   = 0.8f;
    float pitch    = 1.0f;    // playback rate multiplier (0.5..2.0)
    float decay    = 1.0f;    // attenuation of release tail (0..1)
    float pan      = 0.5f;

    // Playback state (audio thread)
    bool    playing  = false;
    float   readPos  = 0.0f;   // fractional sample position
    float   velScale = 1.0f;
    float   ampEnv   = 1.0f;   // decay envelope

    void trigger(int velocity) noexcept;

    // [RT-SAFE]
    void tick(float& outL, float& outR) noexcept;

private:
    float readSampleL(int idx) const noexcept;
    float readSampleR(int idx) const noexcept;
};
