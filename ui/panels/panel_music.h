// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_music.h
// BRIEF: Combined Music panel — Keyboard, Arpeggiator,
//        Chord Engine, Scale Quantizer, Step Sequencer
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/interfaces.h"
#include "core/engine/synth_engine.h"
#include "hal/pc/sequencer_pattern_storage.h"
#include "hal/pc/keyboard_input.h"

namespace PanelMusic {
    void render(AtomicParamStore& params, SynthEngine& engine,
                PatternStorage& patterns, KeyboardInput& kbd,
                MidiEventQueue& midiQueue);
}
