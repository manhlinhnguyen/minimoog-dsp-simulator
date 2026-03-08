// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_presets.h
// BRIEF: Combined preset browser — Engine Presets + Effect Presets tabs
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/engine_manager.h"
#include "hal/pc/engine_preset_storage.h"
#include "hal/pc/effect_preset_storage.h"
#include "hal/pc/global_preset_storage.h"
#include "core/effects/effect_chain.h"

namespace PanelPresets {
    void render(EngineManager&       mgr,
                EnginePresetStorage& engineStorage,
                EffectChain&         effectChain,
                EffectPresetStorage& effectStorage,
                GlobalPresetStorage& globalStorage);
}
