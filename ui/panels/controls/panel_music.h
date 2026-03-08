// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_music.h
// BRIEF: Combined Music panel — Keyboard, Arpeggiator,
//        Chord Engine, Scale Quantizer, Step Sequencer
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/interfaces.h"
#include "core/engines/engine_manager.h"
#include "hal/pc/sequencer_pattern_storage.h"
#include "hal/pc/keyboard_input.h"
#include "ui/panels/controls/panel_midi_player.h"

namespace PanelMusic {
    void render(AtomicParamStore& params, EngineManager& mgr,
                PatternStorage& patterns, KeyboardInput& kbd,
                MidiEventQueue& midiQueue,
                PanelMidiPlayer::State& midiSt, MidiFilePlayer& midiPlayer);
}
