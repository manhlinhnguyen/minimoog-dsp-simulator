// ─────────────────────────────────────────────────────────
// FILE: hal/pc/keyboard_input.cpp
// BRIEF: QWERTY keyboard MIDI implementation
// ─────────────────────────────────────────────────────────
#include "keyboard_input.h"
#include <cstring>

KeyboardInput* KeyboardInput::instance_ = nullptr;

void KeyboardInput::init(GLFWwindow*     window,
                          MidiEventQueue& queue) noexcept {
    window_   = window;
    queue_    = &queue;
    instance_ = this;
    std::memset(keyState_,  0, sizeof(keyState_));
    std::memset(noteState_, 0, sizeof(noteState_));
    prevCallback_ = glfwSetKeyCallback(window, glfwKeyCallback);
    glfwSetWindowFocusCallback(window, glfwFocusCallback);
}

void KeyboardInput::update() noexcept {
    // Octave shift — debounced via key-repeat handled in callback
    (void)window_;  // currently handled via callback
}

int KeyboardInput::keyToNote(int key) const noexcept {
    const int base = octave_ * 12;
    switch (key) {
        // Bottom row — white keys
        case GLFW_KEY_Z:         return base + 0;   // C
        case GLFW_KEY_X:         return base + 2;   // D
        case GLFW_KEY_C:         return base + 4;   // E
        case GLFW_KEY_V:         return base + 5;   // F
        case GLFW_KEY_B:         return base + 7;   // G
        case GLFW_KEY_N:         return base + 9;   // A
        case GLFW_KEY_M:         return base + 11;  // B
        case GLFW_KEY_COMMA:     return base + 12;  // C+1
        case GLFW_KEY_PERIOD:    return base + 14;  // D+1
        case GLFW_KEY_SLASH:     return base + 16;  // E+1
        // Bottom row — black keys
        case GLFW_KEY_S:         return base + 1;   // C#
        case GLFW_KEY_D:         return base + 3;   // D#
        case GLFW_KEY_G:         return base + 6;   // F#
        case GLFW_KEY_H:         return base + 8;   // G#
        case GLFW_KEY_J:         return base + 10;  // A#
        case GLFW_KEY_L:         return base + 13;  // C#+1
        case GLFW_KEY_SEMICOLON: return base + 15;  // D#+1
        // Upper row — white keys (octave+1)
        case GLFW_KEY_Q:         return base + 12;  // C+1
        case GLFW_KEY_W:         return base + 14;  // D+1
        case GLFW_KEY_E:         return base + 16;  // E+1
        case GLFW_KEY_R:         return base + 17;  // F+1
        case GLFW_KEY_T:         return base + 19;  // G+1
        case GLFW_KEY_Y:         return base + 21;  // A+1
        case GLFW_KEY_U:         return base + 23;  // B+1
        case GLFW_KEY_I:         return base + 24;  // C+2
        // Upper row — black keys
        case GLFW_KEY_2:         return base + 13;  // C#+1
        case GLFW_KEY_3:         return base + 15;  // D#+1
        case GLFW_KEY_5:         return base + 18;  // F#+1
        case GLFW_KEY_6:         return base + 20;  // G#+1
        case GLFW_KEY_7:         return base + 22;  // A#+1
        default:                 return -1;
    }
}

void KeyboardInput::sendNoteOn(int note) noexcept {
    if (!queue_ || note < 0 || note > 127) return;
    noteState_[note] = true;
    MidiEvent ev;
    ev.type  = MidiEvent::Type::NoteOn;
    ev.data1 = static_cast<uint8_t>(note);
    ev.data2 = 100;
    queue_->push(ev);
}

void KeyboardInput::sendNoteOff(int note) noexcept {
    if (!queue_ || note < 0 || note > 127) return;
    noteState_[note] = false;
    MidiEvent ev;
    ev.type  = MidiEvent::Type::NoteOff;
    ev.data1 = static_cast<uint8_t>(note);
    ev.data2 = 0;
    queue_->push(ev);
}

void KeyboardInput::getActiveNotes(bool out[128]) const noexcept {
    std::memcpy(out, noteState_, 128 * sizeof(bool));
}

void KeyboardInput::allNotesOff() noexcept {
    for (int n = 0; n < 128; ++n) {
        if (noteState_[n]) {
            noteState_[n] = false;
            if (queue_) {
                MidiEvent ev;
                ev.type  = MidiEvent::Type::NoteOff;
                ev.data1 = static_cast<uint8_t>(n);
                ev.data2 = 0;
                queue_->push(ev);
            }
        }
    }
    std::memset(keyState_, 0, sizeof(keyState_));
}

void KeyboardInput::glfwFocusCallback(GLFWwindow* /*window*/, int focused) {
    if (!instance_) return;
    if (!focused)
        instance_->allNotesOff();  // window lost focus — release all held keys
}

void KeyboardInput::glfwKeyCallback(GLFWwindow* window,
                                     int key, int scan,
                                     int action, int mods) {
    if (!instance_) return;
    // Chain to previous callback (e.g. ImGui's) so text input still works
    if (instance_->prevCallback_)
        instance_->prevCallback_(window, key, scan, action, mods);
    if (key < 0 || key >= 512) return;

    // Octave shift — release all held notes first to avoid stuck notes
    if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS) {
        instance_->allNotesOff();
        if (instance_->octave_ > 0) --instance_->octave_;
        return;
    }
    if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) {
        instance_->allNotesOff();
        if (instance_->octave_ < 9) ++instance_->octave_;
        return;
    }

    const int note = instance_->keyToNote(key);
    if (note < 0) return;

    if (action == GLFW_PRESS && !instance_->keyState_[key]) {
        instance_->keyState_[key] = true;
        instance_->sendNoteOn(note);
    } else if (action == GLFW_RELEASE && instance_->keyState_[key]) {
        instance_->keyState_[key] = false;
        instance_->sendNoteOff(note);
    }
}
