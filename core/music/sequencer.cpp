// ─────────────────────────────────────────────────────────
// FILE: core/music/sequencer.cpp
// BRIEF: Step sequencer implementation
// ─────────────────────────────────────────────────────────
#include "sequencer.h"
#include "core/util/math_utils.h"

void StepSequencer::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    updateTiming();
}

void StepSequencer::setEnabled(bool on) noexcept { enabled_ = on; }

void StepSequencer::setBPM(float bpm) noexcept {
    bpm_ = clamp(bpm, 20.0f, 300.0f);
    updateTiming();
}

void StepSequencer::setStepCount(int n) noexcept {
    stepCount_ = clamp(n, 1, MAX_STEPS);
}

void StepSequencer::setRateIndex(int idx) noexcept {
    rateIdx_ = clamp(idx, 0, 7);
    updateTiming();
}

void StepSequencer::setGlobalGate(float g) noexcept {
    globalGate_ = clamp(g, 0.01f, 1.0f);
}

void StepSequencer::setSwing(float s) noexcept {
    swing_ = clamp(s, 0.0f, 0.5f);
}

void StepSequencer::setStep(int idx, const SeqStep& s) noexcept {
    if (idx >= 0 && idx < MAX_STEPS) steps_[idx] = s;
}

SeqStep StepSequencer::getStep(int idx) const noexcept {
    if (idx >= 0 && idx < MAX_STEPS) return steps_[idx];
    return {};
}

void StepSequencer::clearAll() noexcept {
    for (auto& s : steps_) {
        s = SeqStep{};
        s.active = false;  // "clear" means rest, not active
    }
}

void StepSequencer::play() noexcept {
    playing_ = true;
    phase_   = stepSamples_;  // force step 0 to fire on first tick
    curStep_ = 0;
    noteIsOn_   = false;
    activeNote_ = -1;
}

void StepSequencer::stop() noexcept {
    playing_    = false;
    phase_      = 0.0f;
    curStep_    = 0;
    noteIsOn_   = false;
    activeNote_ = -1;
}

void StepSequencer::reset() noexcept {
    phase_   = 0.0f;
    curStep_ = 0;
}

void StepSequencer::updateTiming() noexcept {
    const float spb = sampleRate_ * 60.0f / bpm_;
    stepSamples_    = spb * ARP_RATE_BEATS[rateIdx_];
}

void StepSequencer::advance() noexcept {
    curStep_ = (curStep_ + 1) % stepCount_;
}

StepSequencer::Output StepSequencer::tick() noexcept {  // [RT-SAFE]
    Output out{};
    if (!enabled_ || !playing_) return out;

    phase_ += 1.0f;

    const SeqStep& st = steps_[curStep_];

    // Swing: odd steps are delayed
    const float effStep     = (curStep_ % 2 == 1)
        ? stepSamples_ * (1.0f + swing_)
        : stepSamples_ * (1.0f - swing_);
    const float gateSamples = effStep * st.gate * globalGate_;

    // Gate off (within-step gate end)
    if (noteIsOn_ && !st.tie && phase_ >= gateSamples) {
        out.hasNoteOff = true;
        out.noteOff    = activeNote_;
        noteIsOn_      = false;
    }

    // New step boundary
    if (phase_ >= effStep) {
        phase_ -= effStep;

        // Note-off if still sounding (tie or gate=100%)
        if (noteIsOn_) {
            out.hasNoteOff = true;
            out.noteOff    = activeNote_;
            noteIsOn_      = false;
        }

        // Play current step BEFORE advancing (fixes step-0 skip)
        out.justStepped = true;
        out.stepIdx     = curStep_;
        if (st.active) {
            activeNote_   = st.note;
            out.hasNoteOn = true;
            out.note      = st.note;
            out.velocity  = st.velocity;
            noteIsOn_     = true;
        }

        advance();
    }
    return out;
}
