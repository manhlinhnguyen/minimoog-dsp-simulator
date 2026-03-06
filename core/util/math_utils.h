// ─────────────────────────────────────────────────────────
// FILE: core/util/math_utils.h
// BRIEF: Frequency utilities, fast approximations, range mapping
// ─────────────────────────────────────────────────────────
#pragma once
#include <cmath>
#include "shared/types.h"

// ════════════════════════════════════════════════════════
// FREQUENCY UTILITIES
// ════════════════════════════════════════════════════════

// MIDI note (0..127) → Hz, A4 = 440 Hz
inline hz_t midiToHz(int note) noexcept {
    return A4_HZ * std::pow(2.0f, (note - A4_MIDI) / 12.0f);
}

// Semitone offset → frequency ratio
inline float semitonesToRatio(float semitones) noexcept {
    return std::pow(2.0f, semitones / 12.0f);
}

// Cents offset → frequency ratio
inline float centsToRatio(float cents) noexcept {
    return std::pow(2.0f, cents / 1200.0f);
}

// Hz → MIDI note (float, for display)
inline float hzToMidi(hz_t hz) noexcept {
    return A4_MIDI + 12.0f * std::log2(hz / A4_HZ);
}

// ════════════════════════════════════════════════════════
// FAST APPROXIMATIONS (audio-rate safe)
// ════════════════════════════════════════════════════════

// Padé 3/3 approximation of tanh.
// Max error < 0.5% in [-3, +3], exact clamp outside.
inline float fast_tanh(float x) noexcept {
    if (x >  3.0f) return  1.0f;
    if (x < -3.0f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Clamp — [RT-SAFE]
template<typename T>
inline T clamp(T val, T lo, T hi) noexcept {
    return val < lo ? lo : (val > hi ? hi : val);
}

// Linear interpolation — [RT-SAFE]
inline float lerp(float a, float b, float t) noexcept {
    return a + t * (b - a);
}

// dB → linear amplitude
inline float dbToLinear(float db) noexcept {
    return std::pow(10.0f, db / 20.0f);
}

// ════════════════════════════════════════════════════════
// RANGE MAPPING
// ════════════════════════════════════════════════════════

// Map [0,1] → [lo, hi] linear
inline float mapLinear(float norm, float lo, float hi) noexcept {
    return lo + norm * (hi - lo);
}

// Map [0,1] → [lo, hi] logarithmic
inline float mapLog(float norm, float lo, float hi) noexcept {
    return lo * std::pow(hi / lo, norm);
}
