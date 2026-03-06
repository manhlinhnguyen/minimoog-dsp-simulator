// ─────────────────────────────────────────────────────────
// FILE: tests/test_scale_quantizer.cpp
// BRIEF: Unit tests for ScaleQuantizer
// ─────────────────────────────────────────────────────────
#include <catch2/catch_test_macros.hpp>
#include "core/music/scale_quantizer.h"

TEST_CASE("ScaleQuantizer - disabled: returns input unchanged", "[scale]") {
    ScaleQuantizer sq;
    sq.setEnabled(false);
    sq.setScale(1);  // major
    sq.setRoot(0);   // C

    for (int note = 0; note <= 127; ++note)
        REQUIRE(sq.quantize(note) == note);
}

TEST_CASE("ScaleQuantizer - C major: C note passes through", "[scale]") {
    ScaleQuantizer sq;
    sq.setEnabled(true);
    sq.setRoot(0);   // C
    sq.setScale(1);  // major

    REQUIRE(sq.quantize(60) == 60);  // C4 → C4
    REQUIRE(sq.quantize(62) == 62);  // D4 → D4
    REQUIRE(sq.quantize(64) == 64);  // E4 → E4
}

TEST_CASE("ScaleQuantizer - C major: Db snaps to nearest (C or D)", "[scale]") {
    ScaleQuantizer sq;
    sq.setEnabled(true);
    sq.setRoot(0);
    sq.setScale(1);  // major: no Db

    const int quantized = sq.quantize(61);  // C#4/Db4
    // Should snap to C4(60) or D4(62)
    REQUIRE((quantized == 60 || quantized == 62));
}

TEST_CASE("ScaleQuantizer - output is always valid MIDI", "[scale]") {
    ScaleQuantizer sq;
    sq.setEnabled(true);

    for (int s = 0; s < ScaleQuantizer::SCALE_COUNT; ++s) {
        sq.setScale(s);
        for (int root = 0; root < 12; ++root) {
            sq.setRoot(root);
            for (int note = 0; note <= 127; ++note) {
                const int out = sq.quantize(note);
                INFO("scale=" << s << " root=" << root << " note=" << note);
                REQUIRE(out >= 0);
                REQUIRE(out <= 127);
            }
        }
    }
}

TEST_CASE("ScaleQuantizer - chromatic scale: no change", "[scale]") {
    ScaleQuantizer sq;
    sq.setEnabled(true);
    sq.setScale(0);  // chromatic
    sq.setRoot(0);

    for (int note = 0; note <= 127; ++note)
        REQUIRE(sq.quantize(note) == note);
}

TEST_CASE("ScaleQuantizer - root shift works", "[scale]") {
    ScaleQuantizer sq;
    sq.setEnabled(true);
    sq.setScale(1);  // major

    // In G major (root=7), G note should pass through
    sq.setRoot(7);
    REQUIRE(sq.quantize(67) == 67);  // G4
}
