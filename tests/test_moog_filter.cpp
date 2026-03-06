// ─────────────────────────────────────────────────────────
// FILE: tests/test_moog_filter.cpp
// BRIEF: Unit tests for MoogLadderFilter
// ─────────────────────────────────────────────────────────
#include <catch2/catch_test_macros.hpp>
#include "core/dsp/moog_filter.h"
#include <cmath>
#include <vector>

static constexpr float SR = 44100.0f;

// Generate a small-signal sinewave, run through filter, return RMS.
// Amplitude must be small enough not to saturate the Huovilainen tanh stages.
static constexpr float TEST_AMP = 0.1f;

static float rmsAfterFilter(MoogLadderFilter& f, float freqHz, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float in  = TEST_AMP * std::sin(TWO_PI * freqHz * i / SR);
        const float out = f.process(in);
        sum += out * out;
    }
    return std::sqrt(sum / n);
}

TEST_CASE("MoogFilter - passes low freqs, attenuates high", "[filter]") {
    MoogLadderFilter f;
    f.setSampleRate(SR);
    f.setCutoff(1000.0f);
    f.setResonance(0.0f);
    f.reset();

    const float rmsLow = rmsAfterFilter(f, 100.0f,  4096);

    f.reset();
    const float rmsHigh = rmsAfterFilter(f, 10000.0f, 4096);

    REQUIRE(rmsLow  > 0.01f);   // signal passes
    REQUIRE(rmsHigh < rmsLow);  // high freq attenuated
}

TEST_CASE("MoogFilter - stable at max resonance", "[filter]") {
    MoogLadderFilter f;
    f.setSampleRate(SR);
    f.setCutoff(1000.0f);
    f.setResonance(1.0f);
    f.reset();

    bool stable = true;
    for (int i = 0; i < 44100; ++i) {
        const float out = f.process(0.0f);
        if (!std::isfinite(out) || std::abs(out) > 100.0f)
            stable = false;
    }
    REQUIRE(stable);
}

TEST_CASE("MoogFilter - reset clears internal state", "[filter]") {
    MoogLadderFilter f;
    f.setSampleRate(SR);
    f.setCutoff(500.0f);
    f.setResonance(0.5f);

    // Drive the filter with signal
    for (int i = 0; i < 1000; ++i)
        f.process(std::sin(TWO_PI * 440.0f * i / SR));

    f.reset();

    // After reset, silence input → output should be near zero
    for (int i = 0; i < 100; ++i) {
        const float out = f.process(0.0f);
        REQUIRE(std::abs(out) < 0.01f);
    }
}

TEST_CASE("MoogFilter - setCutoff clamps to valid range", "[filter]") {
    MoogLadderFilter f;
    f.setSampleRate(SR);

    f.setCutoff(-1000.0f);
    REQUIRE(f.getCutoff() >= 20.0f);

    f.setCutoff(100000.0f);
    REQUIRE(f.getCutoff() <= SR * 0.5f);
}

TEST_CASE("MoogFilter - resonance clamps to [0,1]", "[filter]") {
    MoogLadderFilter f;
    f.setSampleRate(SR);

    f.setResonance(-0.5f);
    REQUIRE(f.getResonance() == 0.0f);

    f.setResonance(1.5f);
    REQUIRE(f.getResonance() == 1.0f);
}
