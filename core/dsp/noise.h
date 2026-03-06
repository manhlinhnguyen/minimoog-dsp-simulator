// ─────────────────────────────────────────────────────────
// FILE: core/dsp/noise.h
// BRIEF: White and pink noise generator (header-only, no alloc)
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include <cstdint>

// ════════════════════════════════════════════════════════
// NOISE GENERATOR
// White: flat spectrum, uniform random (32-bit LCG)
// Pink:  -3dB/octave, Voss-McCartney algorithm (7 stages)
// ════════════════════════════════════════════════════════

class NoiseGenerator {
public:
    enum class Color : int { White = 0, Pink = 1 };

    void setColor(Color c) noexcept { color_ = c; }

    // [RT-SAFE]
    sample_t tick() noexcept {
        return (color_ == Color::Pink) ? tickPink() : tickWhite();
    }

private:
    Color    color_   = Color::White;
    uint32_t state_   = 22222u;  // LCG state

    // Voss-McCartney pink noise state (7 stages, no alloc)
    float pink_[7]    = {};
    int   pinkIdx_    = 0;

    // 32-bit LCG → [-1, +1]
    float lcg() noexcept {
        state_ = state_ * 1664525u + 1013904223u;
        return static_cast<float>(static_cast<int32_t>(state_))
               * (1.0f / 2147483648.0f);
    }

    sample_t tickWhite() noexcept { return lcg(); }

    sample_t tickPink() noexcept {
        // Voss-McCartney: update one stage per sample (round-robin)
        const int stage = pinkIdx_ & 7;
        if (stage < 7)
            pink_[stage] = lcg();
        ++pinkIdx_;
        float sum = 0.0f;
        for (int i = 0; i < 7; ++i) sum += pink_[i];
        return sum * (1.0f / 7.0f);
    }
};
