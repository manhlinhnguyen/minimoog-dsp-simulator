// ─────────────────────────────────────────────────────────
// FILE: ui/panels/controls/panel_midi_player.h
// BRIEF: MIDI file player transport panel
// ─────────────────────────────────────────────────────────
#pragma once
#include <string>
#include <vector>
#include "core/music/midi_file_player.h"

namespace PanelMidiPlayer {

struct State {
    std::vector<std::string> fileList;   // absolute paths
    int     selectedIdx  = -1;           // currently loaded index
    bool    listScanned  = false;
    char    midiDir[512] = "./midi";
};

void init     (State& st, const char* midiDir);
void render   (State& st, MidiFilePlayer& player);
void renderTab(State& st, MidiFilePlayer& player);

} // namespace PanelMidiPlayer
