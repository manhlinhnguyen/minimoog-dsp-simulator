// ─────────────────────────────────────────────────────────
// FILE: ui/widgets/seq_display.h
// BRIEF: Interactive step sequencer grid widget
// ─────────────────────────────────────────────────────────
#pragma once
#include "imgui.h"
#include "core/music/sequencer.h"
#include "shared/interfaces.h"

namespace SeqDisplay {
    // Draws interactive step sequencer grid.
    // steps:      array of MAX_STEPS SeqStep (read/write — modified directly)
    // activeStep: currently playing step (-1 = none)
    // store:      AtomicParamStore (reserved for future BPM/transport display)
    void draw(SeqStep steps[], int stepCount,
              int activeStep, AtomicParamStore& store);
}
