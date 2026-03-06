// ─────────────────────────────────────────────────────────
// FILE: tests/test_sequencer.cpp
// BRIEF: Unit tests for StepSequencer
// ─────────────────────────────────────────────────────────
#include <catch2/catch_test_macros.hpp>
#include "core/music/sequencer.h"

static constexpr float SR  = 44100.0f;
static constexpr float BPM = 240.0f;  // fast for testing

TEST_CASE("Sequencer - no output when not playing", "[seq]") {
    StepSequencer seq;
    seq.setSampleRate(SR);
    seq.setEnabled(true);
    seq.setBPM(BPM);

    for (int i = 0; i < 10000; ++i) {
        auto out = seq.tick();
        REQUIRE(!out.hasNoteOn);
    }
}

TEST_CASE("Sequencer - produces note-on after play()", "[seq]") {
    StepSequencer seq;
    seq.setSampleRate(SR);
    seq.setEnabled(true);
    seq.setBPM(BPM);
    seq.setRateIndex(3);  // 1/8

    SeqStep step;
    step.note     = 60;
    step.velocity = 100;
    step.active   = true;
    seq.setStep(0, step);
    seq.play();

    bool gotNoteOn = false;
    for (int i = 0; i < static_cast<int>(SR * 2); ++i) {
        auto out = seq.tick();
        if (out.hasNoteOn) { gotNoteOn = true; break; }
    }
    REQUIRE(gotNoteOn);
}

TEST_CASE("Sequencer - stop sends note-off and resets", "[seq]") {
    StepSequencer seq;
    seq.setSampleRate(SR);
    seq.setEnabled(true);
    seq.setBPM(BPM);

    SeqStep step;
    step.active = true;
    seq.setStep(0, step);
    seq.play();

    // Run a bit then stop
    for (int i = 0; i < 5000; ++i) seq.tick();
    seq.stop();

    REQUIRE(!seq.isPlaying());
    REQUIRE(seq.getCurrentStep() == 0);
}

TEST_CASE("Sequencer - rest steps produce no note-on", "[seq]") {
    StepSequencer seq;
    seq.setSampleRate(SR);
    seq.setEnabled(true);
    seq.setBPM(BPM);
    seq.setStepCount(1);
    seq.setRateIndex(3);

    SeqStep step;
    step.active = false;  // rest
    seq.setStep(0, step);
    seq.play();

    for (int i = 0; i < static_cast<int>(SR * 2); ++i) {
        auto out = seq.tick();
        REQUIRE(!out.hasNoteOn);
    }
}

TEST_CASE("Sequencer - clearAll marks all steps as rest", "[seq]") {
    StepSequencer seq;
    seq.setSampleRate(SR);
    seq.setEnabled(true);
    seq.setBPM(BPM);
    seq.setRateIndex(3);

    // Set all steps active
    for (int i = 0; i < StepSequencer::MAX_STEPS; ++i) {
        SeqStep s;
        s.active = true;
        seq.setStep(i, s);
    }
    seq.clearAll();
    seq.play();

    for (int i = 0; i < static_cast<int>(SR * 2); ++i) {
        auto out = seq.tick();
        REQUIRE(!out.hasNoteOn);
    }
}
