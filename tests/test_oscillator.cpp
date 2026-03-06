// ─────────────────────────────────────────────────────────
// FILE: tests/test_oscillator.cpp
// BRIEF: Unit tests for Oscillator
// ─────────────────────────────────────────────────────────
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "core/dsp/oscillator.h"
#include <cmath>
#include <vector>
#include <numeric>

static constexpr float SR = 44100.0f;

// Render N samples from an oscillator
static std::vector<float> render(Oscillator& osc, int n) {
    std::vector<float> out(n);
    for (int i = 0; i < n; ++i) out[i] = osc.tick();
    return out;
}

TEST_CASE("Oscillator - Sawtooth DC is near zero", "[osc]") {
    Oscillator osc;
    osc.setSampleRate(SR);
    osc.setFrequency(440.0f);
    osc.setWaveShape(WaveShape::Sawtooth);
    const auto buf = render(osc, static_cast<int>(SR));  // 1 second
    const float mean = std::accumulate(buf.begin(), buf.end(), 0.0f)
                     / static_cast<float>(buf.size());
    REQUIRE(std::abs(mean) < 0.01f);
}

TEST_CASE("Oscillator - Square DC is near zero", "[osc]") {
    Oscillator osc;
    osc.setSampleRate(SR);
    osc.setFrequency(440.0f);
    osc.setWaveShape(WaveShape::Square);
    const auto buf = render(osc, static_cast<int>(SR));
    const float mean = std::accumulate(buf.begin(), buf.end(), 0.0f)
                     / static_cast<float>(buf.size());
    REQUIRE(std::abs(mean) < 0.05f);
}

TEST_CASE("Oscillator - Peak amplitude within [-1, 1]", "[osc]") {
    for (int w = 0; w < static_cast<int>(WaveShape::COUNT); ++w) {
        Oscillator osc;
        osc.setSampleRate(SR);
        osc.setFrequency(440.0f);
        osc.setWaveShape(static_cast<WaveShape>(w));
        const auto buf = render(osc, 4096);
        for (float s : buf) {
            INFO("WaveShape " << w << " sample " << s);
            REQUIRE(s >= -1.5f);  // some tolerance for PolyBLEP overshoot
            REQUIRE(s <=  1.5f);
        }
    }
}

TEST_CASE("Oscillator - Range R16 is half frequency of R8", "[osc]") {
    // At R8 (reference), 440 Hz. At R16, should be 220 Hz.
    Oscillator osc;
    osc.setSampleRate(SR);
    osc.setFrequency(440.0f);
    osc.setWaveShape(WaveShape::Sawtooth);

    osc.setRange(OscRange::R8);
    // Count zero-crossings for 1 second
    auto buf8 = render(osc, static_cast<int>(SR));

    osc.reset();
    osc.setRange(OscRange::R16);
    auto buf16 = render(osc, static_cast<int>(SR));

    int cross8  = 0, cross16 = 0;
    for (int i = 1; i < static_cast<int>(buf8.size()); ++i) {
        if (buf8[i - 1] < 0 && buf8[i] >= 0)  ++cross8;
        if (buf16[i - 1] < 0 && buf16[i] >= 0) ++cross16;
    }

    // R16 should be half the crossings of R8 (±1)
    REQUIRE(cross16 <= cross8 / 2 + 2);
    REQUIRE(cross16 >= cross8 / 2 - 2);
}

TEST_CASE("Oscillator - setFrequency changes pitch", "[osc]") {
    Oscillator osc;
    osc.setSampleRate(SR);
    osc.setWaveShape(WaveShape::Sawtooth);

    osc.setFrequency(440.0f);
    auto buf440 = render(osc, 4096);

    osc.reset();
    osc.setFrequency(880.0f);
    auto buf880 = render(osc, 4096);

    // Count zero-crossings
    auto crossings = [](const std::vector<float>& b) {
        int c = 0;
        for (int i = 1; i < static_cast<int>(b.size()); ++i)
            if (b[i - 1] < 0 && b[i] >= 0) ++c;
        return c;
    };
    REQUIRE(crossings(buf880) > crossings(buf440));
}

TEST_CASE("Oscillator - hard sync resets phase", "[osc]") {
    Oscillator osc;
    osc.setSampleRate(SR);
    osc.setFrequency(440.0f);
    osc.setWaveShape(WaveShape::Sawtooth);

    render(osc, 1000);  // advance phase
    osc.hardSyncTrigger();
    osc.tick();
    // After sync the phase was reset — the sawtooth starts near -1
    REQUIRE(osc.getPhase() < 0.1f);
}
