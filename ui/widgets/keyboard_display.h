// ─────────────────────────────────────────────────────────
// FILE: ui/widgets/keyboard_display.h
// BRIEF: Interactive 2-octave piano keyboard widget
// ─────────────────────────────────────────────────────────
#pragma once
#include "imgui.h"
#include <functional>

namespace KeyboardDisplay {
    // Draws a 2-octave piano keyboard.
    // activeNotes: 128-element bool array; true = key lit (nullptr = none lit)
    // baseNote:    lowest MIDI note shown (should be a C, e.g. 48 = C3)
    void draw(const bool* activeNotes, int baseNote, ImVec2 size,
              std::function<void(int note, bool on)> onKey = nullptr);
}
