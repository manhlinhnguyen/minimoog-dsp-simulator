// ─────────────────────────────────────────────────────────
// FILE: shared/types.h
// BRIEF: Primitive types, constants, and core structs
// ─────────────────────────────────────────────────────────
#pragma once
#include <cstdint>
#include <cmath>
#include <functional>
#include <array>

// ════════════════════════════════════════════════════════
// PRIMITIVE TYPES
// ════════════════════════════════════════════════════════

using sample_t  = float;   // Audio sample [-1.0, +1.0]
using cv_t      = float;   // Control value [0.0, 1.0]
using hz_t      = float;   // Frequency in Hz
using ms_t      = float;   // Time in milliseconds
using beats_t   = float;   // Musical time in beats

// ════════════════════════════════════════════════════════
// CONSTANTS
// ════════════════════════════════════════════════════════

constexpr float SAMPLE_RATE_DEFAULT = 44100.0f;
constexpr int   BLOCK_SIZE_DEFAULT  = 256;
constexpr int   MAX_VOICES          = 8;
constexpr int   MAX_HELD_NOTES      = 32;
constexpr int   MAX_CHORD_NOTES     = 6;
constexpr float TWO_PI              = 6.28318530718f;
constexpr float HALF_PI             = 1.57079632679f;
constexpr float LOG2                = 0.69314718056f;
constexpr float A4_HZ               = 440.0f;
constexpr int   A4_MIDI             = 69;
constexpr int   MIDI_NOTE_MIN       = 0;
constexpr int   MIDI_NOTE_MAX       = 127;

// ════════════════════════════════════════════════════════
// MIDI EVENT
// ════════════════════════════════════════════════════════

struct MidiEvent {
    enum class Type : uint8_t {
        NoteOn = 0,
        NoteOff,
        ControlChange,
        PitchBend,
        Aftertouch,
        ProgramChange,
        Clock,
        Start,
        Stop,
        Continue,
        Invalid
    };

    Type    type      = Type::Invalid;
    uint8_t channel   = 0;    // 0-based (0 = Ch1)
    uint8_t data1     = 0;    // note / cc number
    uint8_t data2     = 0;    // velocity / value
    int16_t pitchBend = 0;    // -8192..+8191
};

// ════════════════════════════════════════════════════════
// HELD NOTE (voice tracking)
// ════════════════════════════════════════════════════════

struct HeldNote {
    int  midiNote = -1;
    int  velocity = 0;

    bool isValid() const noexcept { return midiNote >= 0; }
};

// ════════════════════════════════════════════════════════
// VOICE MODE & STEAL MODE
// ════════════════════════════════════════════════════════

enum class VoiceMode : int { Mono = 0, Poly = 1, Unison = 2 };
enum class StealMode : int { Oldest = 0, Lowest = 1, Quietest = 2 };
