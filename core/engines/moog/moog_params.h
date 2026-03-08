// ─────────────────────────────────────────────────────────
// FILE: core/engines/moog/moog_params.h
// BRIEF: MoogEngine param IDs, metadata, and DSP norm helpers
// ─────────────────────────────────────────────────────────
#pragma once
#include <cstddef>
#include <cmath>

// ════════════════════════════════════════════════════════
// MOOG PARAM ID — local to the Moog engine.
// Matches the layout expected by Voice::tick(const float p[]).
// Indices must stay contiguous; OSC params group in blocks of 4.
// ════════════════════════════════════════════════════════

enum MoogParamID : int {

    // ── GLOBAL (engine-scoped) ───────────────────────────
    MP_MASTER_TUNE = 0,   // -1.0..+1.0 (semitone)
    MP_MASTER_VOL,        // 0.0..1.0

    // ── CONTROLLERS ──────────────────────────────────────
    MP_GLIDE_ON,          // 0/1 (switch)
    MP_GLIDE_TIME,        // 0.0..1.0 → 0..5000ms (quadratic)
    MP_MOD_MIX,           // 0.0..1.0 (OSC3 ← → Noise)
    MP_OSC_MOD_ON,        // 0/1
    MP_FILTER_MOD_ON,     // 0/1
    MP_OSC3_LFO_ON,       // 0/1 (OSC3 as LFO)

    // ── OSCILLATOR 1 ─────────────────────────────────────
    MP_OSC1_ON,           // 0/1
    MP_OSC1_RANGE,        // 0..5 (LO,32',16',8',4',2')
    MP_OSC1_FREQ,         // 0.0..1.0 → -7..+7 semitones
    MP_OSC1_WAVE,         // 0..5 (WaveShape enum)

    // ── OSCILLATOR 2 ─────────────────────────────────────
    MP_OSC2_ON,
    MP_OSC2_RANGE,
    MP_OSC2_FREQ,
    MP_OSC2_WAVE,

    // ── OSCILLATOR 3 ─────────────────────────────────────
    MP_OSC3_ON,
    MP_OSC3_RANGE,
    MP_OSC3_FREQ,
    MP_OSC3_WAVE,

    // ── MIXER ─────────────────────────────────────────────
    MP_MIX_OSC1,          // 0.0..1.0
    MP_MIX_OSC2,
    MP_MIX_OSC3,
    MP_MIX_NOISE,
    MP_NOISE_COLOR,       // 0=white, 1=pink

    // ── FILTER ───────────────────────────────────────────
    MP_FILTER_CUTOFF,     // 0.0..1.0 → 20Hz..20kHz (log)
    MP_FILTER_EMPHASIS,   // 0.0..1.0 (resonance)
    MP_FILTER_AMOUNT,     // 0.0..1.0 (env mod amount)
    MP_FILTER_KBD_TRACK,  // 0=off, 1=1/3, 2=2/3

    // ── FILTER ENVELOPE ──────────────────────────────────
    MP_FENV_ATTACK,       // 0.0..1.0 → 1..10000ms (log)
    MP_FENV_DECAY,        // 0.0..1.0 → 1..10000ms (log)
    MP_FENV_SUSTAIN,      // 0.0..1.0
    MP_FENV_RELEASE,      // 0.0..1.0 → 1..10000ms (log)

    // ── AMP ENVELOPE ─────────────────────────────────────
    MP_AENV_ATTACK,
    MP_AENV_DECAY,
    MP_AENV_SUSTAIN,
    MP_AENV_RELEASE,

    // ── POLYPHONY ─────────────────────────────────────────
    MP_VOICE_MODE,        // 0=mono, 1=poly, 2=unison
    MP_VOICE_COUNT,       // 1..8
    MP_VOICE_STEAL,       // 0=oldest, 1=lowest, 2=quietest
    MP_UNISON_DETUNE,     // 0.0..1.0 → 0..50 cents spread
    MP_NOTE_PRIORITY,     // 0=last, 1=lowest, 2=highest (mono only)

    MOOG_PARAM_COUNT      // = 42
};

// ════════════════════════════════════════════════════════
// MOOG PARAM METADATA
// ════════════════════════════════════════════════════════

struct MoogParamMeta {
    const char* name;
    float       defaultVal;
    float       minVal;
    float       maxVal;
};

static const MoogParamMeta MOOG_PARAM_META[MOOG_PARAM_COUNT] = {
    // name                  default   min      max
    // ── GLOBAL ──────────────────────────────────────────
    { "Master Tune",          0.0f,   -1.0f,    1.0f  },
    { "Master Volume",        0.8f,    0.0f,    1.0f  },

    // ── CONTROLLERS ─────────────────────────────────────
    { "Glide On",             0.0f,    0.0f,    1.0f  },
    { "Glide Time",           0.1f,    0.0f,    1.0f  },
    { "Mod Mix",              0.0f,    0.0f,    1.0f  },
    { "OSC Mod On",           0.0f,    0.0f,    1.0f  },
    { "Filter Mod On",        0.0f,    0.0f,    1.0f  },
    { "OSC3 LFO Mode",        0.0f,    0.0f,    1.0f  },

    // ── OSCILLATOR 1 ────────────────────────────────────
    { "OSC1 On",              1.0f,    0.0f,    1.0f  },
    { "OSC1 Range",           3.0f,    0.0f,    5.0f  },
    { "OSC1 Freq",            0.5f,    0.0f,    1.0f  },
    { "OSC1 Wave",            3.0f,    0.0f,    5.0f  },

    // ── OSCILLATOR 2 ────────────────────────────────────
    { "OSC2 On",              1.0f,    0.0f,    1.0f  },
    { "OSC2 Range",           3.0f,    0.0f,    5.0f  },
    { "OSC2 Freq",            0.5f,    0.0f,    1.0f  },
    { "OSC2 Wave",            3.0f,    0.0f,    5.0f  },

    // ── OSCILLATOR 3 ────────────────────────────────────
    { "OSC3 On",              1.0f,    0.0f,    1.0f  },
    { "OSC3 Range",           3.0f,    0.0f,    5.0f  },
    { "OSC3 Freq",            0.5f,    0.0f,    1.0f  },
    { "OSC3 Wave",            3.0f,    0.0f,    5.0f  },

    // ── MIXER ───────────────────────────────────────────
    { "Mix OSC1",             1.0f,    0.0f,    1.0f  },
    { "Mix OSC2",             0.0f,    0.0f,    1.0f  },
    { "Mix OSC3",             0.0f,    0.0f,    1.0f  },
    { "Mix Noise",            0.0f,    0.0f,    1.0f  },
    { "Noise Color",          0.0f,    0.0f,    1.0f  },

    // ── FILTER ──────────────────────────────────────────
    { "Filter Cutoff",        0.7f,    0.0f,    1.0f  },
    { "Filter Emphasis",      0.0f,    0.0f,    1.0f  },
    { "Filter Env Amount",    0.5f,    0.0f,    1.0f  },
    { "Filter Kbd Track",     0.0f,    0.0f,    2.0f  },

    // ── FILTER ENVELOPE ─────────────────────────────────
    { "F.Env Attack",         0.1f,    0.0f,    1.0f  },
    { "F.Env Decay",          0.3f,    0.0f,    1.0f  },
    { "F.Env Sustain",        0.5f,    0.0f,    1.0f  },
    { "F.Env Release",        0.3f,    0.0f,    1.0f  },

    // ── AMP ENVELOPE ────────────────────────────────────
    { "A.Env Attack",         0.05f,   0.0f,    1.0f  },
    { "A.Env Decay",          0.3f,    0.0f,    1.0f  },
    { "A.Env Sustain",        0.7f,    0.0f,    1.0f  },
    { "A.Env Release",        0.3f,    0.0f,    1.0f  },

    // ── POLYPHONY ────────────────────────────────────────
    { "Voice Mode",           1.0f,    0.0f,    2.0f  },
    { "Voice Count",          4.0f,    1.0f,    8.0f  },
    { "Voice Steal",          0.0f,    0.0f,    2.0f  },
    { "Unison Detune",        0.2f,    0.0f,    1.0f  },
    { "Note Priority",        0.0f,    0.0f,    2.0f  },
};

// ════════════════════════════════════════════════════════
// NORMALIZED VALUE HELPERS
// ════════════════════════════════════════════════════════

// Log scale: norm=0→minVal, norm=1→maxVal
inline float normToLog(float norm, float minVal, float maxVal) noexcept {
    return minVal * std::pow(maxVal / minVal, norm);
}

// Cutoff: 0..1 → 20Hz..20000Hz
inline float normToCutoffHz(float norm) noexcept {
    return normToLog(norm, 20.0f, 20000.0f);
}

// Envelope time: 0..1 → 1ms..10000ms
inline float normToEnvMs(float norm) noexcept {
    return normToLog(norm, 1.0f, 10000.0f);
}

// Glide time: 0..1 → 0ms..5000ms (quadratic for fine control)
inline float normToGlideMs(float norm) noexcept {
    return norm * norm * 5000.0f;
}

// OSC freq offset: 0..1 → -7..+7 semitones
inline float normToSemitones(float norm) noexcept {
    return (norm - 0.5f) * 14.0f;
}
