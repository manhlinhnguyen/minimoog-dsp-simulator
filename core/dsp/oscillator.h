// ─────────────────────────────────────────────────────────
// FILE: core/dsp/oscillator.h
// BRIEF: Band-limited oscillator with PolyBLEP anti-aliasing
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"

enum class WaveShape : int {
    Triangle   = 0,  // ∧ symmetric triangle
    TriSaw     = 1,  // /\ triangle-sawtooth hybrid
    ReverseSaw = 2,  // \  falling sawtooth
    Sawtooth   = 3,  // /  rising sawtooth (most common)
    Square     = 4,  // ⊓  50% duty square
    WidePulse  = 5,  // ⊓  ~10% duty pulse (nasal)
    COUNT      = 6
};

// OSC range (octave multiplier)
enum class OscRange : int {
    LO  = 0,  // very slow (~1/4 Hz) — LFO use
    R32 = 1,  // 32' (lowest pitched)
    R16 = 2,  // 16'
    R8  = 3,  // 8'  (concert pitch)
    R4  = 4,  // 4'
    R2  = 5,  // 2'  (highest)
};

class Oscillator {
public:
    // ── Initialization ───────────────────────────────────
    void setSampleRate(float sr) noexcept;
    void reset() noexcept;   // phase = 0

    // ── Parameters ───────────────────────────────────────
    void setFrequency(hz_t hz)     noexcept;
    void setWaveShape(WaveShape s) noexcept;
    void setRange(OscRange r)      noexcept;
    void setAmplitude(float a)     noexcept;  // 0..1
    void setPulseWidth(float pw)   noexcept;  // 0.1..0.9

    // ── Hard sync ────────────────────────────────────────
    // Call when master OSC crosses zero → resets phase.
    void hardSyncTrigger() noexcept;

    // ── Render ───────────────────────────────────────────
    // [RT-SAFE]
    sample_t tick() noexcept;

    // ── State query ──────────────────────────────────────
    float     getPhase()     const noexcept { return phase_; }
    hz_t      getFrequency() const noexcept { return freq_; }
    WaveShape getShape()     const noexcept { return shape_; }

private:
    float     sampleRate_  = SAMPLE_RATE_DEFAULT;
    hz_t      freq_        = 440.0f;
    hz_t      effectiveHz_ = 440.0f;  // freq_ × rangeMultiplier
    float     amplitude_   = 1.0f;
    float     pulseWidth_  = 0.5f;
    WaveShape shape_       = WaveShape::Sawtooth;
    OscRange  range_       = OscRange::R8;
    float     phase_       = 0.0f;    // [0, 1)
    bool      syncPending_ = false;

    // PolyBLEP correction at discontinuity points.
    // Reference: Välimäki & Pekonen (2007)
    float    polyblep(float t, float dt) const noexcept;

    sample_t renderSawtooth()   const noexcept;
    sample_t renderReverseSaw() const noexcept;
    sample_t renderSquare()     const noexcept;
    sample_t renderTriangle()   noexcept;
    sample_t renderTriSaw()     noexcept;

    void updateEffectiveHz() noexcept;

    // Range → octave multiplier
    static constexpr float RANGE_MULT[6] = {
        0.0625f,  // LO  = /16
        0.125f,   // 32'
        0.25f,    // 16'
        0.5f,     // 8'  (reference)
        1.0f,     // 4'
        2.0f      // 2'
    };
};
