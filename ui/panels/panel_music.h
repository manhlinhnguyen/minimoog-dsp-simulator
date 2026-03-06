// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_music.h
// BRIEF: Combined Music panel (Arp + Chord + Scale + Seq)
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/interfaces.h"
#include "core/engine/synth_engine.h"
#include "hal/pc/pattern_storage.h"

namespace PanelMusic {
    void render(AtomicParamStore& params, SynthEngine& engine,
                PatternStorage& patterns);
}
