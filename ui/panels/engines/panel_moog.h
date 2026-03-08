// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_moog.h
// BRIEF: "Mini Moog Model D Engine" combined panel
//        (Oscillators + Mixer + Filter & Envelopes)
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/moog/moog_engine.h"

namespace PanelEngine {
    void render(MoogEngine& eng);
    void renderContent(MoogEngine& eng);
}
