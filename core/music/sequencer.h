// ─────────────────────────────────────────────────────────
// FILE: core/music/sequencer.h
// BRIEF: 16-step sequencer with swing, tie, and gate control
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"
#include "core/music/arpeggiator.h"  // ARP_RATE_BEATS
#include <array>

struct SeqStep {
    int   note     = 60;
    int   velocity = 100;
    float gate     = 0.8f;   // 0..1 (overrides global gate)
    bool  active   = true;   // false = rest
    bool  tie      = false;  // legato into next step
};

class StepSequencer {
public:
    static constexpr int MAX_STEPS = 16;

    // ── Setup ────────────────────────────────────────────
    void setSampleRate(float sr) noexcept;

    // ── Config ───────────────────────────────────────────
    void setEnabled   (bool on)   noexcept;
    void setBPM       (float bpm) noexcept;
    void setStepCount (int n)     noexcept;  // 1..16
    void setRateIndex (int idx)   noexcept;  // 0..7
    void setGlobalGate(float g)   noexcept;  // 0..1
    void setSwing     (float s)   noexcept;  // 0..0.5

    // ── Step editing ─────────────────────────────────────
    void    setStep (int idx, const SeqStep& s) noexcept;
    SeqStep getStep (int idx)              const noexcept;
    void    clearAll()                           noexcept;

    // ── Transport ────────────────────────────────────────
    void play()  noexcept;
    void stop()  noexcept;   // sends note-off, resets to step 0
    void reset() noexcept;   // resets phase without stopping

    // ── Per-sample output ────────────────────────────────
    struct Output {
        bool hasNoteOn   = false;
        bool hasNoteOff  = false;
        int  note        = 60;     // note-on note
        int  noteOff     = 60;     // note-off note (separate from note-on)
        int  velocity    = 100;
        int  stepIdx     = 0;
        bool justStepped = false;  // for GUI step highlight
    };
    // [RT-SAFE]
    Output tick() noexcept;

    // ── Query ────────────────────────────────────────────
    bool isPlaying()      const noexcept { return playing_; }
    bool isEnabled()      const noexcept { return enabled_; }
    int  getCurrentStep() const noexcept { return curStep_; }

private:
    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    bool  enabled_    = false;
    bool  playing_    = false;
    float bpm_        = 120.0f;
    int   stepCount_  = 16;
    int   rateIdx_    = 3;      // 1/8
    float globalGate_ = 0.8f;
    float swing_      = 0.0f;

    std::array<SeqStep, MAX_STEPS> steps_;
    int   curStep_     = 0;
    float phase_       = 0.0f;
    float stepSamples_ = 0.0f;
    bool  noteIsOn_    = false;
    int   activeNote_  = -1;

    void updateTiming() noexcept;
    void advance()      noexcept;
};
