// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_dx7.h
// BRIEF: Yamaha DX7 FM synth UI panel
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/dx7/dx7_engine.h"

namespace PanelDX7 {
    void render(DX7Engine& engine);
    void renderContent(DX7Engine& engine);
}
