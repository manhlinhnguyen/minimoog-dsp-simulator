// ─────────────────────────────────────────────────────────
// FILE: core/engines/moog/voice.h
// BRIEF: One complete synthesis voice: 3 OSC → Filter → VCA
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/dsp/oscillator.h"
#include "core/dsp/moog_filter.h"
#include "core/dsp/envelope.h"
#include "core/dsp/glide.h"
#include "core/dsp/param_smoother.h"
#include "core/dsp/noise.h"
#include "shared/types.h"
#include "moog_params.h"

// ════════════════════════════════════════════════════════
// VOICE — one complete synthesis path
// 3 OSC → Mixer → Moog Filter → VCA
// Filter ADSR + Amp ADSR
// Polyphony = N Voice objects running in parallel
// ════════════════════════════════════════════════════════

class Voice {
public:
    // ── Setup ────────────────────────────────────────────
    void init(float sampleRate) noexcept;

    // ── Triggers ─────────────────────────────────────────
    void noteOn(int midiNote, int velocity,
                float unisonDetuneCents = 0.0f) noexcept;
    void noteOff()   noexcept;
    void forceOff()  noexcept;  // immediate silence, for stealing
    void reset()     noexcept;

    // ── Per-sample render ────────────────────────────────
    // params: snapshot array (read once per block, no atomics)
    // [RT-SAFE]
    sample_t tick(const float params[]) noexcept;

    // ── State query ──────────────────────────────────────
    bool  isActive()    const noexcept;
    bool  isReleasing() const noexcept;
    int   getNote()     const noexcept { return note_; }
    int   getVelocity() const noexcept { return velocity_; }
    float getLevel()    const noexcept { return lastLevel_; }
    int   getAge()      const noexcept { return age_; }

    void tickAge() noexcept { ++age_; }

private:
    // ── DSP modules ──────────────────────────────────────
    Oscillator       osc_[3];
    MoogLadderFilter filter_;
    ControlEnvelope  filterEnv_;
    ControlEnvelope  ampEnv_;
    GlideProcessor   glide_;
    NoiseGenerator   noise_;

    // ── Smoothers for hot params (updated every sample) ──
    ParamSmoother cutoffSmoother_;
    ParamSmoother resSmoother_;
    ParamSmoother mixSmoother_[4];  // OSC1, OSC2, OSC3, Noise

    // ── State ────────────────────────────────────────────
    float sampleRate_  = SAMPLE_RATE_DEFAULT;
    int   note_        = 60;
    int   velocity_    = 0;
    float velAmp_      = 1.0f;   // velocity → [0,1]
    float detuneCents_ = 0.0f;   // unison offset
    float lastLevel_   = 0.0f;
    int   age_         = 0;      // samples since noteOn

    // ── Private helpers ──────────────────────────────────
    void applyOscParams   (const float p[]) noexcept;
    void applyFilterParams(const float p[]) noexcept;
    void applyEnvParams   (const float p[]) noexcept;

    hz_t  noteToHz(int midiNote,
                   float semitoneOffset = 0.0f) const noexcept;
    float kbdTrackOffset(const float p[]) const noexcept;
};
