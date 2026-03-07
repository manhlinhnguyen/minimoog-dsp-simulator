// ─────────────────────────────────────────────────────────
// FILE: shared/params.h
// BRIEF: ParamID enum, ParamMeta, and normalized value helpers
// ─────────────────────────────────────────────────────────
#pragma once
#include <cstddef>
#include <cmath>

// ════════════════════════════════════════════════════════
// PARAM ID — unique identifier for every parameter.
// Used by: UI knobs, MIDI CC mapping, preset JSON keys.
// ════════════════════════════════════════════════════════

enum ParamID : int {

    // ── GLOBAL ──────────────────────────────────────────
    P_MASTER_TUNE = 0,  // -1.0..+1.0 (semitone)
    P_MASTER_VOL,       // 0.0..1.0
    P_BPM,              // 60.0..200.0

    // ── CONTROLLERS ─────────────────────────────────────
    P_GLIDE_ON,         // 0/1 (switch)
    P_GLIDE_TIME,       // 0.0..1.0 → 0..5000ms (quadratic)
    P_MOD_MIX,          // 0.0..1.0 (OSC3 ← → Noise)
    P_OSC_MOD_ON,       // 0/1
    P_FILTER_MOD_ON,    // 0/1
    P_OSC3_LFO_ON,      // 0/1 (OSC3 as LFO)

    // ── OSCILLATOR 1 ────────────────────────────────────
    P_OSC1_ON,          // 0/1
    P_OSC1_RANGE,       // 0..5 (LO,32',16',8',4',2')
    P_OSC1_FREQ,        // 0.0..1.0 → -7..+7 semitones
    P_OSC1_WAVE,        // 0..5 (WaveShape enum)

    // ── OSCILLATOR 2 ────────────────────────────────────
    P_OSC2_ON,
    P_OSC2_RANGE,
    P_OSC2_FREQ,
    P_OSC2_WAVE,

    // ── OSCILLATOR 3 ────────────────────────────────────
    P_OSC3_ON,
    P_OSC3_RANGE,
    P_OSC3_FREQ,
    P_OSC3_WAVE,

    // ── MIXER ───────────────────────────────────────────
    P_MIX_OSC1,         // 0.0..1.0
    P_MIX_OSC2,
    P_MIX_OSC3,
    P_MIX_NOISE,
    P_NOISE_COLOR,      // 0=white, 1=pink

    // ── FILTER ──────────────────────────────────────────
    P_FILTER_CUTOFF,    // 0.0..1.0 → 20Hz..20kHz (log)
    P_FILTER_EMPHASIS,  // 0.0..1.0 (resonance)
    P_FILTER_AMOUNT,    // 0.0..1.0 (env mod amount)
    P_FILTER_KBD_TRACK, // 0=off, 1=1/3, 2=2/3

    // ── FILTER ENVELOPE ─────────────────────────────────
    P_FENV_ATTACK,      // 0.0..1.0 → 1..10000ms (log)
    P_FENV_DECAY,       // 0.0..1.0 → 1..10000ms (log)
    P_FENV_SUSTAIN,     // 0.0..1.0
    P_FENV_RELEASE,     // 0.0..1.0 → 1..10000ms (log)

    // ── AMP ENVELOPE ────────────────────────────────────
    P_AENV_ATTACK,
    P_AENV_DECAY,
    P_AENV_SUSTAIN,
    P_AENV_RELEASE,

    // ── POLYPHONY ───────────────────────────────────────
    P_VOICE_MODE,       // 0=mono, 1=poly, 2=unison
    P_VOICE_COUNT,      // 1..8
    P_VOICE_STEAL,      // 0=oldest, 1=lowest, 2=quietest
    P_UNISON_DETUNE,    // 0.0..1.0 → 0..50 cents spread

    // ── ARPEGGIATOR ─────────────────────────────────────
    P_ARP_ON,           // 0/1
    P_ARP_MODE,         // 0..5
    P_ARP_OCTAVES,      // 1..4
    P_ARP_RATE,         // 0..7 (subdivision index)
    P_ARP_GATE,         // 0.0..1.0
    P_ARP_SWING,        // 0.0..0.5

    // ── SEQUENCER ───────────────────────────────────────
    P_SEQ_ON,           // 0/1
    P_SEQ_PLAYING,      // 0/1 (play/stop transport)
    P_SEQ_STEPS,        // 1..16
    P_SEQ_RATE,         // 0..7
    P_SEQ_GATE,         // 0.0..1.0
    P_SEQ_SWING,        // 0.0..0.5

    // ── CHORD ───────────────────────────────────────────
    P_CHORD_ON,         // 0/1
    P_CHORD_TYPE,       // 0..15
    P_CHORD_INVERSION,  // 0..3

    // ── SCALE ───────────────────────────────────────────
    P_SCALE_ON,         // 0/1
    P_SCALE_ROOT,       // 0..11 (C..B)
    P_SCALE_TYPE,       // 0..15

    PARAM_COUNT         // total param count (array size)
};

// Returns true for Arpeggiator, Sequencer, Chord, and Scale parameters.
// These are excluded from Moog Presets — they belong to the Music panel
// and are managed independently of the synthesizer sound.
inline bool isMusicParam(int id) noexcept {
    return id >= P_ARP_ON && id < PARAM_COUNT;
}

// ════════════════════════════════════════════════════════
// PARAM METADATA — min, max, default, display name
// ════════════════════════════════════════════════════════

struct ParamMeta {
    const char* name;        // display name
    const char* jsonKey;     // key in preset JSON
    float       defaultVal;
    float       minVal;
    float       maxVal;
    bool        isDiscrete;  // true → snap to integer
    const char* unit;        // "", "Hz", "ms", "BPM"...
};

// Defined in shared/params.cpp
extern const ParamMeta PARAM_META[PARAM_COUNT];

// ════════════════════════════════════════════════════════
// NORMALIZED VALUE HELPERS
// ════════════════════════════════════════════════════════

// Log scale: norm=0→minVal, norm=1→maxVal
inline float normToLog(float norm, float minVal, float maxVal) noexcept {
    return minVal * std::pow(maxVal / minVal, norm);
}

inline float logToNorm(float val, float minVal, float maxVal) noexcept {
    return std::log(val / minVal) / std::log(maxVal / minVal);
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
