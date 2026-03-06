// ─────────────────────────────────────────────────────────
// FILE: core/music/scale_quantizer.cpp
// BRIEF: Scale quantizer implementation
// ─────────────────────────────────────────────────────────
#include "scale_quantizer.h"
#include "core/util/math_utils.h"

const ScaleDef ScaleQuantizer::SCALES[SCALE_COUNT] = {
    { "Chromatic",  {1,1,1,1,1,1,1,1,1,1,1,1} },
    { "Major",      {1,0,1,0,1,1,0,1,0,1,0,1} },
    { "Nat.Minor",  {1,0,1,1,0,1,0,1,1,0,1,0} },
    { "Harm.Minor", {1,0,1,1,0,1,0,1,1,0,0,1} },
    { "Mel.Minor",  {1,0,1,1,0,1,0,1,0,1,0,1} },
    { "Dorian",     {1,0,1,1,0,1,0,1,0,1,1,0} },
    { "Phrygian",   {1,1,0,1,0,1,0,1,1,0,1,0} },
    { "Lydian",     {1,0,1,0,1,0,1,1,0,1,0,1} },
    { "Mixolydian", {1,0,1,0,1,1,0,1,0,1,1,0} },
    { "Locrian",    {1,1,0,1,0,1,1,0,1,0,1,0} },
    { "Penta.Maj",  {1,0,1,0,1,0,0,1,0,1,0,0} },
    { "Penta.Min",  {1,0,0,1,0,1,0,1,0,0,1,0} },
    { "Blues",      {1,0,0,1,0,1,1,1,0,0,1,0} },
    { "WholeTone",  {1,0,1,0,1,0,1,0,1,0,1,0} },
    { "Diminished", {1,0,1,1,0,1,1,0,1,1,0,1} },
    { "Augmented",  {1,0,0,1,1,0,0,1,1,0,0,1} },
};

void ScaleQuantizer::setEnabled(bool on) noexcept { enabled_ = on; }

void ScaleQuantizer::setRoot(int s) noexcept {
    root_ = ((s % 12) + 12) % 12;
}

void ScaleQuantizer::setScale(int idx) noexcept {
    scaleIdx_ = clamp(idx, 0, SCALE_COUNT - 1);
}

int ScaleQuantizer::quantize(int midiNote) const noexcept {
    if (!enabled_) return midiNote;

    const ScaleDef& sc  = SCALES[scaleIdx_];
    const int octave    = midiNote / 12;
    const int semitone  = midiNote % 12;

    // Position relative to root
    const int rel = ((semitone - root_) % 12 + 12) % 12;

    // Find nearest scale degree by searching ±6 semitones
    int bestRel  = rel;
    int bestDist = 13;
    for (int d = 0; d <= 6; ++d) {
        const int up   = (rel + d) % 12;
        const int down = ((rel - d) % 12 + 12) % 12;
        if (sc.degrees[up]   && d < bestDist) { bestDist = d; bestRel = up;   }
        if (sc.degrees[down] && d < bestDist) { bestDist = d; bestRel = down; }
    }

    const int quantizedSemi = (bestRel + root_) % 12;
    return clamp(octave * 12 + quantizedSemi, 0, 127);
}
