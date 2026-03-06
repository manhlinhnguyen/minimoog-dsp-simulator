#pragma once
#include "shared/interfaces.h"
#include "core/engine/synth_engine.h"
#include "hal/pc/preset_storage.h"
namespace PanelPresets {
    void render(AtomicParamStore& params,
                SynthEngine&      engine,
                PresetStorage&    storage);
}
