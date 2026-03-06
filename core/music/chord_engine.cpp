// ─────────────────────────────────────────────────────────
// FILE: core/music/chord_engine.cpp
// BRIEF: Chord engine implementation
// ─────────────────────────────────────────────────────────
#include "chord_engine.h"
#include "core/util/math_utils.h"

// 16 chord voicings (root-relative semitone intervals)
const ChordVoicing ChordEngine::CHORDS[CHORD_COUNT] = {
    { "Major",    {0,  4,  7,  0,  0,  0}, 3 },
    { "Minor",    {0,  3,  7,  0,  0,  0}, 3 },
    { "Dom7",     {0,  4,  7, 10,  0,  0}, 4 },
    { "Maj7",     {0,  4,  7, 11,  0,  0}, 4 },
    { "Min7",     {0,  3,  7, 10,  0,  0}, 4 },
    { "Sus2",     {0,  2,  7,  0,  0,  0}, 3 },
    { "Sus4",     {0,  5,  7,  0,  0,  0}, 3 },
    { "Dim",      {0,  3,  6,  0,  0,  0}, 3 },
    { "Dim7",     {0,  3,  6,  9,  0,  0}, 4 },
    { "Aug",      {0,  4,  8,  0,  0,  0}, 3 },
    { "Min7b5",   {0,  3,  6, 10,  0,  0}, 4 },
    { "Add9",     {0,  4,  7, 14,  0,  0}, 4 },
    { "Maj9",     {0,  4,  7, 11, 14,  0}, 5 },
    { "Min9",     {0,  3,  7, 10, 14,  0}, 5 },
    { "Power",    {0,  7,  0,  0,  0,  0}, 2 },
    { "Octave",   {0, 12,  0,  0,  0,  0}, 2 },
};

void ChordEngine::setEnabled(bool on) noexcept   { enabled_   = on; }
void ChordEngine::setChordType(int idx) noexcept  { chordType_ = clamp(idx, 0, CHORD_COUNT - 1); }
void ChordEngine::setInversion(int inv) noexcept  { inversion_ = clamp(inv, 0, 3); }

ChordEngine::Output ChordEngine::expand(
        int rootNote, int velocity) const noexcept {
    Output out{};
    if (!enabled_) {
        out.notes[0]      = rootNote;
        out.velocities[0] = velocity;
        out.count         = 1;
        return out;
    }

    const ChordVoicing& cv = CHORDS[chordType_];
    out.count = cv.noteCount;
    for (int i = 0; i < cv.noteCount; ++i) {
        out.notes[i]      = clamp(rootNote + cv.intervals[i], 0, 127);
        out.velocities[i] = velocity;
    }
    applyInversion(out.notes, out.count);
    return out;
}

void ChordEngine::applyInversion(
        int notes[], int count) const noexcept {
    // Each inversion: raise the lowest note by one octave
    for (int inv = 0; inv < inversion_; ++inv) {
        int loIdx = 0;
        for (int i = 1; i < count; ++i)
            if (notes[i] < notes[loIdx]) loIdx = i;
        notes[loIdx] = clamp(notes[loIdx] + 12, 0, 127);
    }
}
