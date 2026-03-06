#pragma once
#include "imgui.h"
#include "core/music/sequencer.h"
#include "shared/interfaces.h"

namespace SeqDisplay {
    // Draws interactive step sequencer grid.
    // steps: array of MAX_STEPS SeqStep (read/write)
    // activeStep: currently playing step (-1 = none)
    void draw(SeqStep steps[], int stepCount,
              int activeStep, AtomicParamStore& store);
}
