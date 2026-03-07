// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_moog_engine.h
// BRIEF: "Mini Moog Model D Engine" combined panel
//        (Oscillators + Mixer + Filter & Envelopes)
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/interfaces.h"
#include "shared/params.h"

namespace PanelEngine {
    void render(AtomicParamStore& params);
}
