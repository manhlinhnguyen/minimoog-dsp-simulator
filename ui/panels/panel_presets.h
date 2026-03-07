// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_presets.h
// BRIEF: Combined preset browser — Moog Presets + Effect Presets tabs
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/interfaces.h"
#include "hal/pc/moog_preset_storage.h"
#include "hal/pc/effect_preset_storage.h"
#include "core/effects/effect_chain.h"

namespace PanelPresets {
    void render(AtomicParamStore&    params,
                PresetStorage&       moogStorage,
                EffectChain&         effectChain,
                EffectPresetStorage& effectStorage);
}
