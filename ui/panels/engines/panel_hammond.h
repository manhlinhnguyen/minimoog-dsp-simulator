// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_hammond.h
// BRIEF: Hammond B-3 engine UI panel
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/hammond/hammond_engine.h"

namespace PanelHammond {
    void render(HammondEngine& engine);
    void renderContent(HammondEngine& engine);
}
