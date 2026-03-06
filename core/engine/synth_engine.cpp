// ─────────────────────────────────────────────────────────
// FILE: core/engine/synth_engine.cpp
// BRIEF: SynthEngine implementation
// ─────────────────────────────────────────────────────────
#include "synth_engine.h"

void SynthEngine::init(AtomicParamStore* params,
                       MidiEventQueue*   midiQueue) noexcept {
    params_    = params;
    midiQueue_ = midiQueue;
    voicePool_.init(sampleRate_);
    arp_.setSampleRate(sampleRate_);
    seq_.setSampleRate(sampleRate_);
}

void SynthEngine::setSampleRate(float sr) {
    sampleRate_ = sr;
    voicePool_.init(sr);
    arp_.setSampleRate(sr);
    seq_.setSampleRate(sr);
}

void SynthEngine::setBlockSize(int bs) {
    blockSize_ = bs;
}

// ─────────────────────────────────────────────────────────
// MAIN AUDIO CALLBACK — called by RtAudio thread
// [RT-SAFE] — no allocation, no locking, no throwing
// ─────────────────────────────────────────────────────────

void SynthEngine::processBlock(sample_t* outL,
                               sample_t* outR,
                               int nFrames) noexcept {
    // 1. Snapshot all params once per block
    snapshotParams();

    // 2. Drain MIDI events queued by UI/MIDI thread
    drainMidiQueue();

    // 3. Sync music-layer config from snapshot
    syncMusicConfig();

    // 4. Handle sequencer transport edge
    const bool seqOn = paramCache_[P_SEQ_ON]     > 0.5f;
    const bool seqPl = paramCache_[P_SEQ_PLAYING] > 0.5f;
    if (seqOn &&  seqPl && !seqWasPlaying_) seq_.play();
    if (seqOn && !seqPl &&  seqWasPlaying_) seq_.stop();
    seqWasPlaying_ = seqOn && seqPl;

    // 5. Apply voice pool config (mode, count, steal mode)
    voicePool_.applyConfig(paramCache_);

    // 6. Per-sample render loop
    for (int i = 0; i < nFrames; ++i) {
        // Arpeggiator tick
        if (paramCache_[P_ARP_ON] > 0.5f) {
            auto ao = arp_.tick();
            if (ao.hasNoteOff) routeNoteOff(ao.noteOff);
            if (ao.hasNoteOn)  routeNoteOn (ao.note, ao.velocity);
        }

        // Sequencer tick
        if (seqOn && seqPl) {
            auto so = seq_.tick();
            if (so.hasNoteOff) routeNoteOff(so.noteOff);
            if (so.hasNoteOn)  routeNoteOn (so.note, so.velocity);
        }

        // Render all active voices
        sample_t l = 0.0f, r = 0.0f;
        voicePool_.tick(paramCache_, l, r);
        outL[i] = l;
        outR[i] = r;

        // Feed oscilloscope ring buffer
        oscBuf_[oscWritePos_] = l;
        oscWritePos_ = (oscWritePos_ + 1) & (OSC_BUF_SIZE - 1);
    }
}

// ─────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────

void SynthEngine::snapshotParams() noexcept {
    params_->snapshot(paramCache_);
}

void SynthEngine::drainMidiQueue() noexcept {
    MidiEvent e;
    while (midiQueue_->pop(e)) {
        switch (e.type) {
            case MidiEvent::Type::NoteOn:
                if (e.data2 > 0) onNoteOn (e.data1, e.data2);
                else             onNoteOff(e.data1);
                break;
            case MidiEvent::Type::NoteOff:
                onNoteOff(e.data1);
                break;
            case MidiEvent::Type::ControlChange:
                onCC(e.data1, e.data2);
                break;
            case MidiEvent::Type::PitchBend:
                onPitchBend(e.pitchBend);
                break;
            default:
                break;
        }
    }
}

void SynthEngine::syncMusicConfig() noexcept {
    const float bpm = paramCache_[P_BPM];

    arp_.setBPM      (bpm);
    arp_.setMode     (static_cast<ArpMode>(static_cast<int>(paramCache_[P_ARP_MODE])));
    arp_.setOctaves  (static_cast<int>(paramCache_[P_ARP_OCTAVES]));
    arp_.setRateIndex(static_cast<int>(paramCache_[P_ARP_RATE]));
    arp_.setGate     (paramCache_[P_ARP_GATE]);
    arp_.setSwing    (paramCache_[P_ARP_SWING]);
    arp_.setEnabled  (paramCache_[P_ARP_ON] > 0.5f);

    seq_.setBPM      (bpm);
    seq_.setStepCount(static_cast<int>(paramCache_[P_SEQ_STEPS]));
    seq_.setRateIndex(static_cast<int>(paramCache_[P_SEQ_RATE]));
    seq_.setGlobalGate(paramCache_[P_SEQ_GATE]);
    seq_.setSwing    (paramCache_[P_SEQ_SWING]);
    seq_.setEnabled  (paramCache_[P_SEQ_ON] > 0.5f);

    chord_.setEnabled  (paramCache_[P_CHORD_ON] > 0.5f);
    chord_.setChordType(static_cast<int>(paramCache_[P_CHORD_TYPE]));
    chord_.setInversion(static_cast<int>(paramCache_[P_CHORD_INVERSION]));

    scale_.setEnabled(paramCache_[P_SCALE_ON] > 0.5f);
    scale_.setRoot   (static_cast<int>(paramCache_[P_SCALE_ROOT]));
    scale_.setScale  (static_cast<int>(paramCache_[P_SCALE_TYPE]));
}

void SynthEngine::onNoteOn(int note, int vel) noexcept {
    const int qNote = scale_.quantize(note);
    if (paramCache_[P_ARP_ON] > 0.5f)
        arp_.noteOn(qNote, vel);
    else
        routeNoteOn(qNote, vel);
}

void SynthEngine::onNoteOff(int note) noexcept {
    const int qNote = scale_.quantize(note);
    if (paramCache_[P_ARP_ON] > 0.5f)
        arp_.noteOff(qNote);
    else
        routeNoteOff(qNote);
}

void SynthEngine::routeNoteOn(int note, int vel) noexcept {
    const auto co = chord_.expand(note, vel);
    for (int i = 0; i < co.count; ++i)
        voicePool_.noteOn(co.notes[i], co.velocities[i]);
}

void SynthEngine::routeNoteOff(int note) noexcept {
    const auto co = chord_.expand(note, 0);
    for (int i = 0; i < co.count; ++i)
        voicePool_.noteOff(co.notes[i]);
}

void SynthEngine::onCC(int cc, int val) noexcept {
    switch (cc) {
        case 1:  params_->set(P_MOD_MIX,    val / 127.0f); break;
        case 7:  params_->set(P_MASTER_VOL, val / 127.0f); break;
        case 64: if (val < 64) voicePool_.allNotesOff();   break;
        case 120: case 123: voicePool_.allSoundOff();       break;
        default: break;
    }
}

void SynthEngine::onPitchBend(int bend) noexcept {
    // ±2 semitone pitch bend → master tune
    const float semi = (bend / 8192.0f) * 2.0f;
    params_->set(P_MASTER_TUNE, semi);
}

int   SynthEngine::getActiveVoices() const noexcept { return voicePool_.getActiveCount(); }
int   SynthEngine::getArpNote()      const noexcept { return arp_.getCurrentNote(); }
int   SynthEngine::getSeqStep()      const noexcept { return seq_.getCurrentStep(); }
bool  SynthEngine::getSeqPlaying()   const noexcept { return seq_.isPlaying(); }
float SynthEngine::getBPM()          const noexcept {
    return params_ ? params_->get(P_BPM) : 120.0f;
}

void    SynthEngine::setSeqStep(int idx, const SeqStep& s) noexcept { seq_.setStep(idx, s); }
SeqStep SynthEngine::getSeqStep(int idx)              const noexcept { return seq_.getStep(idx); }

void SynthEngine::getOscBuffer(float out[OSC_BUF_SIZE], int& writePos) const noexcept {
    writePos = oscWritePos_;
    for (int i = 0; i < OSC_BUF_SIZE; ++i) out[i] = oscBuf_[i];
}
