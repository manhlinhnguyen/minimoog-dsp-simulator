// ─────────────────────────────────────────────────────────
// FILE: tests/test_voice.cpp
// BRIEF: Unit tests for Voice
// ─────────────────────────────────────────────────────────
#include <catch2/catch_test_macros.hpp>
#include "core/voice/voice.h"
#include "shared/params.h"
#include <cmath>
#include <array>

static constexpr float SR = 44100.0f;

// Build a minimal param array with defaults
static std::array<float, PARAM_COUNT> makeDefaultParams() {
    std::array<float, PARAM_COUNT> p{};
    for (int i = 0; i < PARAM_COUNT; ++i)
        p[i] = PARAM_META[i].defaultVal;
    return p;
}

TEST_CASE("Voice - inactive by default", "[voice]") {
    Voice v;
    v.init(SR);
    REQUIRE(!v.isActive());
}

TEST_CASE("Voice - becomes active after noteOn", "[voice]") {
    Voice v;
    v.init(SR);
    v.noteOn(60, 100);
    REQUIRE(v.isActive());
}

TEST_CASE("Voice - goes to releasing after noteOff", "[voice]") {
    Voice v;
    v.init(SR);
    v.noteOn(60, 100);

    auto p = makeDefaultParams();
    // Run past attack/decay
    for (int i = 0; i < 5000; ++i) v.tick(p.data());

    v.noteOff();
    REQUIRE(v.isReleasing());
}

TEST_CASE("Voice - becomes inactive after release completes", "[voice]") {
    Voice v;
    v.init(SR);
    v.noteOn(60, 100);

    auto p = makeDefaultParams();
    p[P_AENV_ATTACK]  = 0.0f;
    p[P_AENV_DECAY]   = 0.0f;
    p[P_AENV_SUSTAIN] = 0.8f;
    p[P_AENV_RELEASE] = 0.01f;  // very short release

    for (int i = 0; i < 1000; ++i) v.tick(p.data());

    v.noteOff();
    for (int i = 0; i < static_cast<int>(SR); ++i) v.tick(p.data());

    REQUIRE(!v.isActive());
}

TEST_CASE("Voice - produces non-zero output while active", "[voice]") {
    Voice v;
    v.init(SR);
    v.noteOn(60, 100);

    auto p = makeDefaultParams();
    p[P_MIX_OSC1]  = 1.0f;
    p[P_OSC1_ON]   = 1.0f;

    float maxAbs = 0.0f;
    for (int i = 0; i < 4096; ++i) {
        const float out = v.tick(p.data());
        if (std::abs(out) > maxAbs) maxAbs = std::abs(out);
    }
    REQUIRE(maxAbs > 0.001f);
}

TEST_CASE("Voice - forceOff silences immediately", "[voice]") {
    Voice v;
    v.init(SR);
    v.noteOn(60, 100);

    auto p = makeDefaultParams();
    for (int i = 0; i < 1000; ++i) v.tick(p.data());

    v.forceOff();
    REQUIRE(!v.isActive());
}
