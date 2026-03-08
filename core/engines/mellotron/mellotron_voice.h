// ─────────────────────────────────────────────────────────
// FILE: core/engines/mellotron/mellotron_voice.h
// BRIEF: Mellotron M400 single voice (wavetable playback)
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include "mellotron_tables.h"

struct MellotronVoice {
    bool  active      = false;
    int   note        = 60;

    float phase       = 0.0f;   // 0..1 position in table
    float phaseInc    = 0.0f;   // per-sample increment

    // Tape runout envelope (starts at 1, fades after runout time)
    float heldSamples = 0.0f;   // how many samples note has been held
    float runoutSamps = 0.0f;   // total samples before runout starts

    // ADSR (soft attack/release for tape feel)
    float envVal      = 0.0f;
    float attackRate  = 0.0f;
    float releaseRate = 0.0f;
    bool  releasing   = false;

    float pitchOffset = 1.0f;   // tape speed (0.8..1.2)

    void trigger(int midiNote, float sampleRate,
                 float tapeSpeedFactor,
                 float attackMs, float releaseMs,
                 float runoutTimeS) noexcept;

    void release() noexcept { releasing = true; }

    // [RT-SAFE]
    void tick(MellotronTape tape, float pitchFactor, float& outL, float& outR) noexcept;
};
