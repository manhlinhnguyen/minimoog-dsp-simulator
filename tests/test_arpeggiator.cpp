// ─────────────────────────────────────────────────────────
// FILE: tests/test_arpeggiator.cpp
// BRIEF: Unit tests for Arpeggiator
// ─────────────────────────────────────────────────────────
#include <catch2/catch_test_macros.hpp>
#include "core/music/arpeggiator.h"

static constexpr float SR  = 44100.0f;
static constexpr float BPM = 120.0f;

TEST_CASE("Arpeggiator - no output when disabled", "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(SR);
    arp.setBPM(BPM);
    arp.setEnabled(false);
    arp.noteOn(60, 100);

    for (int i = 0; i < 10000; ++i) {
        auto out = arp.tick();
        REQUIRE(!out.hasNoteOn);
        REQUIRE(!out.hasNoteOff);
    }
}

TEST_CASE("Arpeggiator - no output when no notes held", "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(SR);
    arp.setBPM(BPM);
    arp.setEnabled(true);
    arp.setRateIndex(3);  // 1/8

    for (int i = 0; i < 10000; ++i) {
        auto out = arp.tick();
        REQUIRE(!out.hasNoteOn);
    }
}

TEST_CASE("Arpeggiator - produces note-on after holding a note", "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(SR);
    arp.setBPM(BPM);
    arp.setEnabled(true);
    arp.setRateIndex(3);  // 1/8 note
    arp.noteOn(60, 100);

    bool gotNoteOn = false;
    for (int i = 0; i < static_cast<int>(SR * 2); ++i) {
        auto out = arp.tick();
        if (out.hasNoteOn) { gotNoteOn = true; break; }
    }
    REQUIRE(gotNoteOn);
}

TEST_CASE("Arpeggiator - Up mode plays notes in ascending order", "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(SR);
    arp.setBPM(240.0f);  // fast for testing
    arp.setEnabled(true);
    arp.setMode(ArpMode::Up);
    arp.setRateIndex(3);
    arp.noteOn(60, 100);
    arp.noteOn(64, 100);
    arp.noteOn(67, 100);

    std::vector<int> played;
    for (int i = 0; i < static_cast<int>(SR * 4); ++i) {
        auto out = arp.tick();
        if (out.hasNoteOn) {
            played.push_back(out.note);
            if (static_cast<int>(played.size()) >= 6) break;
        }
    }

    REQUIRE(played.size() >= 3);
    // First 3 notes should be non-decreasing
    for (size_t i = 1; i < std::min(played.size(), size_t{3}); ++i)
        REQUIRE(played[i] >= played[i - 1]);
}

TEST_CASE("Arpeggiator - allNotesOff stops arp", "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(SR);
    arp.setBPM(BPM);
    arp.setEnabled(true);
    arp.setRateIndex(3);
    arp.noteOn(60, 100);
    arp.allNotesOff();

    for (int i = 0; i < 10000; ++i) {
        auto out = arp.tick();
        REQUIRE(!out.hasNoteOn);
    }
}
