// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_mellotron.h
// BRIEF: Mellotron M400 tape emulation UI panel
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/mellotron/mellotron_engine.h"

namespace PanelMellotron {
    void render(MellotronEngine& engine);
    void renderContent(MellotronEngine& engine);
}
