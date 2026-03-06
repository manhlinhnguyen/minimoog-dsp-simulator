// ─────────────────────────────────────────────────────────
// FILE: tests/test_envelope.cpp
// BRIEF: Unit tests for ControlEnvelope
// ─────────────────────────────────────────────────────────
#include <catch2/catch_test_macros.hpp>
#include "core/dsp/envelope.h"
#include <vector>

static constexpr float SR = 44100.0f;

static std::vector<float> runEnv(ControlEnvelope& env, int n) {
    std::vector<float> out(n);
    for (int i = 0; i < n; ++i) out[i] = env.tick();
    return out;
}

TEST_CASE("Envelope - starts at zero (Idle)", "[env]") {
    ControlEnvelope env;
    env.setSampleRate(SR);
    REQUIRE(env.getLevel() == 0.0f);
    REQUIRE(env.getStage() == ControlEnvelope::Stage::Idle);
    REQUIRE(!env.isActive());
}

TEST_CASE("Envelope - attack reaches 1.0", "[env]") {
    ControlEnvelope env;
    env.setSampleRate(SR);
    env.setAttack(10.0f);   // 10 ms
    env.setDecay(100.0f);
    env.setSustain(0.7f);
    env.setRelease(100.0f);

    env.noteOn();
    float peak = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        const float v = env.tick();
        if (v > peak) peak = v;
    }
    REQUIRE(peak >= 0.99f);
}

TEST_CASE("Envelope - sustain holds at sustain level", "[env]") {
    ControlEnvelope env;
    env.setSampleRate(SR);
    env.setAttack(1.0f);
    env.setDecay(1.0f);
    env.setSustain(0.5f);
    env.setRelease(100.0f);

    env.noteOn();
    // Run past attack and decay
    for (int i = 0; i < 5000; ++i) env.tick();

    REQUIRE(env.getStage() == ControlEnvelope::Stage::Sustain);
    REQUIRE(std::abs(env.getLevel() - 0.5f) < 0.01f);
}

TEST_CASE("Envelope - release decays to zero", "[env]") {
    ControlEnvelope env;
    env.setSampleRate(SR);
    env.setAttack(1.0f);
    env.setDecay(1.0f);
    env.setSustain(0.8f);
    env.setRelease(50.0f);

    env.noteOn();
    for (int i = 0; i < 5000; ++i) env.tick();  // reach sustain

    env.noteOff();
    for (int i = 0; i < static_cast<int>(SR); ++i) env.tick();  // wait 1s

    REQUIRE(env.getStage() == ControlEnvelope::Stage::Idle);
    REQUIRE(env.getLevel() == 0.0f);
}

TEST_CASE("Envelope - retrigger from release (no click)", "[env]") {
    ControlEnvelope env;
    env.setSampleRate(SR);
    env.setAttack(50.0f);
    env.setDecay(50.0f);
    env.setSustain(0.7f);
    env.setRelease(200.0f);

    env.noteOn();
    for (int i = 0; i < 8000; ++i) env.tick();

    env.noteOff();
    for (int i = 0; i < 500; ++i) env.tick();

    // Retrigger while releasing
    const float levelBeforeRetrigger = env.getLevel();
    env.noteOn();
    const float levelAfterRetrigger = env.getLevel();

    // Should retrigger from current level, not reset to zero
    REQUIRE(std::abs(levelAfterRetrigger - levelBeforeRetrigger) < 0.01f);
    REQUIRE(env.getStage() == ControlEnvelope::Stage::Attack);
}

TEST_CASE("Envelope - reset returns to Idle", "[env]") {
    ControlEnvelope env;
    env.setSampleRate(SR);
    env.setAttack(100.0f);
    env.noteOn();
    for (int i = 0; i < 100; ++i) env.tick();

    env.reset();
    REQUIRE(env.getStage() == ControlEnvelope::Stage::Idle);
    REQUIRE(env.getLevel() == 0.0f);
}
