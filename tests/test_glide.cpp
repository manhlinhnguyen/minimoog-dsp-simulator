// ─────────────────────────────────────────────────────────
// FILE: tests/test_glide.cpp
// BRIEF: Unit tests for GlideProcessor
// ─────────────────────────────────────────────────────────
#include <catch2/catch_test_macros.hpp>
#include "core/dsp/glide.h"
#include <cmath>

static constexpr float SR = 44100.0f;

TEST_CASE("Glide - disabled: instant jump to target", "[glide]") {
    GlideProcessor g;
    g.setSampleRate(SR);
    g.setEnabled(false);
    g.jumpTo(440.0f);
    g.setTarget(880.0f);
    REQUIRE(g.tick() == 880.0f);
}

TEST_CASE("Glide - enabled: interpolates over time", "[glide]") {
    GlideProcessor g;
    g.setSampleRate(SR);
    g.setGlideTime(100.0f);  // 100 ms
    g.setEnabled(true);
    g.jumpTo(440.0f);
    g.setTarget(880.0f);

    // After 1 sample the glide shouldn't have jumped all the way
    const float after1 = g.tick();
    REQUIRE(after1 > 440.0f);
    REQUIRE(after1 < 880.0f);
}

TEST_CASE("Glide - reaches target eventually", "[glide]") {
    GlideProcessor g;
    g.setSampleRate(SR);
    g.setGlideTime(10.0f);  // 10 ms
    g.setEnabled(true);
    g.jumpTo(440.0f);
    g.setTarget(880.0f);

    float last = 0.0f;
    for (int i = 0; i < static_cast<int>(SR); ++i)
        last = g.tick();

    REQUIRE(std::abs(last - 880.0f) < 1.0f);  // within 1 Hz
}

TEST_CASE("Glide - jumpTo snaps instantly", "[glide]") {
    GlideProcessor g;
    g.setSampleRate(SR);
    g.setGlideTime(500.0f);
    g.setEnabled(true);
    g.jumpTo(440.0f);
    g.jumpTo(880.0f);

    REQUIRE(g.getCurrent() == 880.0f);
    REQUIRE(!g.isGliding());
}

TEST_CASE("Glide - zero glide time is instant", "[glide]") {
    GlideProcessor g;
    g.setSampleRate(SR);
    g.setGlideTime(0.0f);
    g.setEnabled(true);
    g.jumpTo(440.0f);
    g.setTarget(880.0f);
    REQUIRE(g.tick() == 880.0f);
}
