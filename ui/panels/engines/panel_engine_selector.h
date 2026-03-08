// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_engine_selector.h
// BRIEF: Accordion engine selector — switches active engine
//        and shows its sub-panel
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/engine_manager.h"
#include "core/engines/moog/moog_engine.h"
#include "core/engines/hammond/hammond_engine.h"
#include "core/engines/rhodes/rhodes_engine.h"
#include "core/engines/dx7/dx7_engine.h"
#include "core/engines/mellotron/mellotron_engine.h"
#include "core/engines/drums/drum_engine.h"

namespace PanelEngineSelector {
    void render(EngineManager& mgr);
    void renderEngine(EngineManager& mgr);
}
