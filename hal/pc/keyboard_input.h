// ─────────────────────────────────────────────────────────
// FILE: hal/pc/keyboard_input.h
// BRIEF: QWERTY keyboard → MIDI note mapping via GLFW
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include "shared/interfaces.h"
#include "core/util/spsc_queue.h"
#include <GLFW/glfw3.h>

// QWERTY → MIDI mapping (piano-style layout)
// Bottom row: Z X C V B N M , . /  → C..E+1 (white keys)
//             S D   G H J   L ;    → C#..D# (black keys)
// Upper row:  Q W E R T Y U I      → C+1..C+2
//             2 3   5 6 7          → C#+1..A#+1
// [ ] = Octave down / up

class KeyboardInput {
public:
    void init(GLFWwindow*     window,
              MidiEventQueue& queue) noexcept;

    void update() noexcept;  // call each frame for octave shift

    static void glfwKeyCallback(GLFWwindow* window,
                                int key, int scancode,
                                int action, int mods);

    int  getOctave()                          const noexcept { return octave_; }
    bool isNoteOn(int note)                   const noexcept {
        return (note >= 0 && note < 128) ? noteState_[note] : false;
    }
    void getActiveNotes(bool out[128])        const noexcept;

    // Send NoteOff for every currently-held note and clear all key state.
    // Call before octave shift or on focus loss to prevent stuck notes.
    void allNotesOff() noexcept;

    static void glfwFocusCallback(GLFWwindow* window, int focused);

private:
    GLFWwindow*     window_        = nullptr;
    MidiEventQueue* queue_         = nullptr;
    int             octave_        = 4;
    bool            keyState_[512] = {};
    bool            noteState_[128] = {};

    static KeyboardInput* instance_;
    GLFWkeyfun              prevCallback_ = nullptr;  // chained (e.g. ImGui's)

    int  keyToNote(int glfwKey) const noexcept;
    void sendNoteOn (int note)        noexcept;
    void sendNoteOff(int note)        noexcept;
};
