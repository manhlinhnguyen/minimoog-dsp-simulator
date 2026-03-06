// ─────────────────────────────────────────────────────────
// FILE: core/music/arpeggiator.h
// BRIEF: Arpeggiator — 6 modes, 8 rates, swing, gate
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include <array>
#include <cstdint>

enum class ArpMode : int {
    Up      = 0,
    Down    = 1,
    UpDown  = 2,
    DownUp  = 3,
    Random  = 4,
    AsPlayed= 5,
    COUNT   = 6
};

// Rate subdivisions: index → beats per step
// 0=1/1  1=1/2  2=1/4  3=1/8  4=1/16  5=1/4.  6=1/8.  7=1/4T
constexpr float ARP_RATE_BEATS[8] = {
    4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 1.5f, 0.75f, (2.0f / 3.0f)
};

class Arpeggiator {
public:
    // ── Config ───────────────────────────────────────────
    void setSampleRate(float sr)  noexcept;
    void setEnabled   (bool on)   noexcept;
    void setMode      (ArpMode m) noexcept;
    void setOctaves   (int octs)  noexcept;  // 1..4
    void setRateIndex (int idx)   noexcept;  // 0..7
    void setGate      (float g)   noexcept;  // 0..1
    void setSwing     (float s)   noexcept;  // 0..0.5
    void setBPM       (float bpm) noexcept;

    // ── Note input ───────────────────────────────────────
    void noteOn     (int note, int vel) noexcept;
    void noteOff    (int note)          noexcept;
    void allNotesOff()                  noexcept;

    // ── Clock ────────────────────────────────────────────
    struct Output {
        bool hasNoteOn  = false;
        bool hasNoteOff = false;
        int  note       = 60;       // note-on note
        int  noteOff    = 60;       // note-off note (may differ at gate=100%)
        int  velocity   = 100;
    };
    // [RT-SAFE]
    Output tick() noexcept;

    bool isEnabled()       const noexcept { return enabled_; }
    int  getCurrentNote()  const noexcept { return currentNote_; }

private:
    bool    enabled_     = false;
    ArpMode mode_        = ArpMode::Up;
    int     octaves_     = 1;
    int     rateIdx_     = 3;       // 1/8 default
    float   gate_        = 0.8f;
    float   swing_       = 0.0f;
    float   bpm_         = 120.0f;
    float   sampleRate_  = SAMPLE_RATE_DEFAULT;

    // Expanded note list (sorted + octave copies)
    std::array<int, 128> noteList_;
    int  noteListLen_    = 0;
    int  listIdx_        = 0;
    int  direction_      = 1;       // +1 or -1 for UpDown/DownUp

    // Held input notes
    std::array<int, 32> heldNotes_;
    std::array<int, 32> heldVels_;
    int  heldCount_      = 0;

    // Per-sample timing
    float   stepSamples_ = 0.0f;
    float   gateSamples_ = 0.0f;
    float   phase_       = 0.0f;
    bool    noteIsOn_    = false;
    int     currentNote_ = -1;
    int     currentVel_  = 100;

    // LCG for random mode (deterministic, no allocation)
    uint32_t randState_  = 99991u;

    void     rebuildNoteList() noexcept;
    void     updateTiming()    noexcept;
    int      nextIndex()       noexcept;

    uint32_t lcgNext() noexcept {
        randState_ = randState_ * 1664525u + 1013904223u;
        return randState_;
    }
};
