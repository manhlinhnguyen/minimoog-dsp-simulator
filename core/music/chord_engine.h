// ─────────────────────────────────────────────────────────
// FILE: core/music/chord_engine.h
// BRIEF: Chord expansion — 16 chord types, 4 inversions
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include <array>

struct ChordVoicing {
    const char* name;
    int         intervals[MAX_CHORD_NOTES];  // semitone offsets from root
    int         noteCount;
};

class ChordEngine {
public:
    static constexpr int   CHORD_COUNT = 16;
    static const ChordVoicing CHORDS[CHORD_COUNT];

    void setEnabled   (bool on) noexcept;
    void setChordType (int idx) noexcept;  // 0..15
    void setInversion (int inv) noexcept;  // 0..3

    struct Output {
        int notes    [MAX_CHORD_NOTES];
        int velocities[MAX_CHORD_NOTES];
        int count;
    };

    // Returns 1 note when disabled, N notes when enabled
    Output expand(int rootNote, int velocity) const noexcept;

    bool isEnabled()    const noexcept { return enabled_; }
    int  getType()      const noexcept { return chordType_; }
    int  getInversion() const noexcept { return inversion_; }

private:
    bool enabled_   = false;
    int  chordType_ = 0;
    int  inversion_ = 0;

    void applyInversion(int notes[], int count) const noexcept;
};
