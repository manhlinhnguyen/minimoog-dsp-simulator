// ─────────────────────────────────────────────────────────
// FILE: core/music/arpeggiator.cpp
// BRIEF: Arpeggiator implementation
// ─────────────────────────────────────────────────────────
#include "arpeggiator.h"
#include "core/util/math_utils.h"
#include <algorithm>

void Arpeggiator::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    updateTiming();
}

void Arpeggiator::setEnabled(bool on) noexcept {
    enabled_ = on;
    if (!on && noteIsOn_) noteIsOn_ = false;
}

void Arpeggiator::setMode(ArpMode m) noexcept {
    if (mode_ == m) return;   // avoid rebuild every block
    mode_ = m;
    rebuildNoteList();
}

void Arpeggiator::setOctaves(int o) noexcept {
    const int c = clamp(o, 1, 4);
    if (octaves_ == c) return;  // avoid rebuild every block
    octaves_ = c;
    rebuildNoteList();
}

void Arpeggiator::setRateIndex(int idx) noexcept {
    rateIdx_ = clamp(idx, 0, 7);
    updateTiming();
}

void Arpeggiator::setGate(float g) noexcept {
    gate_ = clamp(g, 0.01f, 1.0f);
    updateTiming();
}

void Arpeggiator::setSwing(float s) noexcept {
    swing_ = clamp(s, 0.0f, 0.5f);
}

void Arpeggiator::setBPM(float bpm) noexcept {
    bpm_ = clamp(bpm, 20.0f, 300.0f);
    updateTiming();
}

void Arpeggiator::noteOn(int note, int vel) noexcept {
    if (heldCount_ >= 32) return;
    // Avoid duplicate
    for (int i = 0; i < heldCount_; ++i)
        if (heldNotes_[i] == note) return;
    heldNotes_[heldCount_] = note;
    heldVels_ [heldCount_] = vel;
    ++heldCount_;
    rebuildNoteList();
}

void Arpeggiator::noteOff(int note) noexcept {
    for (int i = 0; i < heldCount_; ++i) {
        if (heldNotes_[i] == note) {
            for (int j = i; j < heldCount_ - 1; ++j) {
                heldNotes_[j] = heldNotes_[j + 1];
                heldVels_ [j] = heldVels_ [j + 1];
            }
            --heldCount_;
            rebuildNoteList();
            return;
        }
    }
}

void Arpeggiator::allNotesOff() noexcept {
    heldCount_   = 0;
    noteListLen_ = 0;
    noteIsOn_    = false;
    currentNote_ = -1;
}

void Arpeggiator::updateTiming() noexcept {
    const float samplesPerBeat = sampleRate_ * 60.0f / bpm_;
    stepSamples_ = samplesPerBeat * ARP_RATE_BEATS[rateIdx_];
    gateSamples_ = stepSamples_ * gate_;
}

void Arpeggiator::rebuildNoteList() noexcept {
    if (heldCount_ == 0) { noteListLen_ = 0; return; }

    int sorted[32];
    for (int i = 0; i < heldCount_; ++i)
        sorted[i] = heldNotes_[i];

    if (mode_ != ArpMode::AsPlayed)
        std::sort(sorted, sorted + heldCount_);

    noteListLen_ = 0;
    for (int oct = 0; oct < octaves_; ++oct) {
        for (int i = 0; i < heldCount_; ++i) {
            const int n = sorted[i] + oct * 12;
            if (n <= 127)
                noteList_[noteListLen_++] = n;
        }
    }

    // Always reset position so the very first nextIndex() returns index 0.
    // nextIndex() increments before returning, so we start one before index 0.
    listIdx_ = noteListLen_ - 1;
}

int Arpeggiator::nextIndex() noexcept {
    if (noteListLen_ == 0) return -1;

    switch (mode_) {
        case ArpMode::Up:
        case ArpMode::AsPlayed:
            listIdx_ = (listIdx_ + 1) % noteListLen_;
            break;

        case ArpMode::Down:
            listIdx_ = (listIdx_ - 1 + noteListLen_) % noteListLen_;
            break;

        case ArpMode::UpDown:
        case ArpMode::DownUp:
            listIdx_ += direction_;
            if (listIdx_ >= noteListLen_) {
                listIdx_   = noteListLen_ - 2;
                direction_ = -1;
            } else if (listIdx_ < 0) {
                listIdx_   = 1;
                direction_ = 1;
            }
            break;

        case ArpMode::Random:
            listIdx_ = static_cast<int>(
                lcgNext() % static_cast<uint32_t>(noteListLen_));
            break;

        default:
            listIdx_ = 0;
            break;
    }
    return listIdx_;
}

Arpeggiator::Output Arpeggiator::tick() noexcept {  // [RT-SAFE]
    Output out{};
    if (!enabled_ || noteListLen_ == 0) return out;

    phase_ += 1.0f;

    // Swing: odd steps are delayed
    const float effectiveStep = (listIdx_ % 2 == 1)
        ? stepSamples_ * (1.0f + swing_)
        : stepSamples_ * (1.0f - swing_);

    // Gate-off?
    if (noteIsOn_ && phase_ >= gateSamples_) {
        out.hasNoteOff = true;
        out.noteOff    = currentNote_;
        noteIsOn_      = false;
    }

    // New step?
    if (phase_ >= effectiveStep) {
        phase_ -= effectiveStep;
        const int idx = nextIndex();
        if (idx >= 0) {
            if (noteIsOn_) {            // gate=100% case
                out.hasNoteOff = true;
                out.noteOff    = currentNote_;
                noteIsOn_      = false;
            }
            currentNote_  = noteList_[idx];
            currentVel_   = 100;
            out.hasNoteOn = true;
            out.note      = currentNote_;
            out.velocity  = currentVel_;
            noteIsOn_     = true;
        }
    }
    return out;
}
