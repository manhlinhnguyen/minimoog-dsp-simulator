// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_drums.h
// BRIEF: Hybrid drum machine UI panel
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/drums/drum_engine.h"

namespace PanelDrums {
    void render(DrumEngine& engine);
    void renderContent(DrumEngine& engine);
}
