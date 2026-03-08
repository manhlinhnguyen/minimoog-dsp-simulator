// ─────────────────────────────────────────────────────────
// FILE: core/engines/engine_manager.cpp
// BRIEF: EngineManager implementation
// ─────────────────────────────────────────────────────────
#include "engine_manager.h"
#include <algorithm>

EngineManager::EngineManager() {
    for (auto& e : engines_) e = nullptr;
}

void EngineManager::init(AtomicParamStore* globalParams,
                          MidiEventQueue*   midiQueue) noexcept {
    globalParams_ = globalParams;
    midiQueue_    = midiQueue;
    arp_.setSampleRate(sampleRate_);
    seq_.setSampleRate(sampleRate_);
}

int EngineManager::registerEngine(std::unique_ptr<IEngine> engine) noexcept {
    if (engineCount_ >= MAX_ENGINES || !engine) return -1;
    engine->init(sampleRate_);
    int idx = engineCount_++;
    engines_[idx] = std::move(engine);
    return idx;
}

void EngineManager::switchEngine(int idx) noexcept {
    if (idx < 0 || idx >= engineCount_) return;
    const int old = activeIdx_.load(std::memory_order_relaxed);
    if (old == idx) return;
    if (engines_[old]) engines_[old]->allSoundOff();
    activeIdx_.store(idx, std::memory_order_release);
}

int EngineManager::getActiveIndex() const noexcept {
    return activeIdx_.load(std::memory_order_relaxed);
}

IEngine* EngineManager::getEngine(int idx) const noexcept {
    if (idx < 0 || idx >= engineCount_) return nullptr;
    return engines_[idx].get();
}

IEngine* EngineManager::getActiveEngine() const noexcept {
    return getEngine(activeIdx_.load(std::memory_order_relaxed));
}

void EngineManager::setGlobalParam(int id, float v) noexcept {
    if (globalParams_) globalParams_->set(id, v);
}

float EngineManager::getGlobalParam(int id) const noexcept {
    return globalParams_ ? globalParams_->get(id) : 0.0f;
}

void EngineManager::setSeqStep(int idx, const SeqStep& s) noexcept {
    seq_.setStep(idx, s);
}

SeqStep EngineManager::getSeqStep(int idx) const noexcept {
    return seq_.getStep(idx);
}

int  EngineManager::getActiveVoices() const noexcept {
    const IEngine* e = getActiveEngine();
    return e ? e->getActiveVoices() : 0;
}

int  EngineManager::getArpNote()        const noexcept { return arp_.getCurrentNote(); }
int  EngineManager::getSeqCurrentStep() const noexcept { return seq_.getCurrentStep(); }
bool EngineManager::getSeqPlaying()     const noexcept { return seq_.isPlaying(); }
float EngineManager::getBPM() const noexcept {
    return globalParams_ ? globalParams_->get(P_BPM) : 120.0f;
}

void EngineManager::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    arp_.setSampleRate(sr);
    seq_.setSampleRate(sr);
    for (int i = 0; i < engineCount_; ++i)
        if (engines_[i]) engines_[i]->setSampleRate(sr);
}

void EngineManager::setBlockSize(int bs) noexcept {
    blockSize_ = bs;
}

// ─────────────────────────────────────────────────────────
// MAIN AUDIO CALLBACK — called by RtAudio thread
// [RT-SAFE] — no allocation, no locking, no throwing
// ─────────────────────────────────────────────────────────

void EngineManager::processBlock(sample_t* outL,
                                  sample_t* outR,
                                  int nFrames) noexcept {
    // 1. Snapshot global params (BPM, music)
    snapshotGlobal();

    // 2. Acquire active engine (acquire ensures we see fully-initialised engine)
    const int idx = activeIdx_.load(std::memory_order_acquire);
    IEngine* eng  = (idx >= 0 && idx < engineCount_) ? engines_[idx].get() : nullptr;
    if (!eng) {
        for (int i = 0; i < nFrames; ++i) { outL[i] = 0.0f; outR[i] = 0.0f; }
        return;
    }

    // 3. Pre-block hook — snapshot params + configure VoicePool FIRST
    eng->beginBlock(nFrames);

    // 4a. Tick MIDI file player → dispatch events with current block config
    {
        MidiFilePlayer::BlockEvents fev;
        midiPlayer_.tick(nFrames, sampleRate_, fev);

        const bool midiNowPlaying = midiPlayer_.isPlaying();
        if (midiWasPlaying_ && !midiNowPlaying)
            eng->allNotesOff();
        midiWasPlaying_ = midiNowPlaying;

        for (int i = 0; i < fev.count; ++i) {
            const MidiEvent& e = fev.events[i];
            switch (e.type) {
                case MidiEvent::Type::NoteOn:
                    if (e.data2 > 0) routeNoteOn(eng, e.data1, e.data2);
                    else             routeNoteOff(eng, e.data1);
                    break;
                case MidiEvent::Type::NoteOff:
                    routeNoteOff(eng, e.data1);
                    break;
                case MidiEvent::Type::ControlChange:
                    onCC(eng, e.data1, e.data2);
                    break;
                case MidiEvent::Type::PitchBend:
                    onPitchBend(eng, e.pitchBend);
                    break;
                default: break;
            }
        }
    }

    // 4b. Drain MIDI → music layer → engine
    drainMidiQueue();

    // 5. Sync music layer config
    syncMusicConfig();

    // 6. Sequencer transport edge
    const bool seqOn = globalCache_[P_SEQ_ON]     > 0.5f;
    const bool seqPl = globalCache_[P_SEQ_PLAYING] > 0.5f;
    if (seqOn &&  seqPl && !seqWasPlaying_) seq_.play();
    if (seqOn && !seqPl &&  seqWasPlaying_) seq_.stop();
    seqWasPlaying_ = seqOn && seqPl;

    // 7. Per-sample loop
    const float masterVol = globalCache_[P_MASTER_VOL];  // Output panel knob
    for (int i = 0; i < nFrames; ++i) {
        // Arpeggiator
        if (globalCache_[P_ARP_ON] > 0.5f) {
            auto ao = arp_.tick();
            if (ao.hasNoteOff) routeNoteOff(eng, ao.noteOff);
            if (ao.hasNoteOn)  routeNoteOn (eng, ao.note, ao.velocity);
        }
        // Sequencer
        if (seqOn && seqPl) {
            auto so = seq_.tick();
            if (so.hasNoteOff) routeNoteOff(eng, so.noteOff);
            if (so.hasNoteOn)  routeNoteOn (eng, so.note, so.velocity);
        }

        float l = 0.0f, r = 0.0f;
        eng->tickSample(l, r);
        outL[i] = l * masterVol;
        outR[i] = r * masterVol;

        // Feed oscilloscope ring buffers — captures all engines uniformly
        oscBufL_[oscWritePos_] = outL[i];
        oscBufR_[oscWritePos_] = outR[i];
        oscWritePos_ = (oscWritePos_ + 1) & (OSC_BUF_SIZE - 1);
    }
}

void EngineManager::getOscBuffer(float out[OSC_BUF_SIZE], int& writePos) const noexcept {
    writePos = oscWritePos_;
    for (int i = 0; i < OSC_BUF_SIZE; ++i)
        out[i] = (oscBufL_[i] + oscBufR_[i]) * 0.5f;
}

void EngineManager::getOscBufferStereo(float outL[OSC_BUF_SIZE], float outR[OSC_BUF_SIZE],
                                        int& writePos) const noexcept {
    writePos = oscWritePos_;
    for (int i = 0; i < OSC_BUF_SIZE; ++i) {
        outL[i] = oscBufL_[i];
        outR[i] = oscBufR_[i];
    }
}

// ─────────────────────────────────────────────────────────
// PRIVATE HELPERS
// ─────────────────────────────────────────────────────────

void EngineManager::snapshotGlobal() noexcept {
    if (globalParams_) globalParams_->snapshot(globalCache_);
}

void EngineManager::drainMidiQueue() noexcept {
    if (!midiQueue_) return;
    const int idx = activeIdx_.load(std::memory_order_relaxed);
    IEngine* eng  = (idx >= 0 && idx < engineCount_) ? engines_[idx].get() : nullptr;

    MidiEvent e;
    while (midiQueue_->pop(e)) {
        switch (e.type) {
            case MidiEvent::Type::NoteOn:
                if (e.data2 > 0) onNoteOn (eng, e.data1, e.data2);
                else             onNoteOff(eng, e.data1);
                break;
            case MidiEvent::Type::NoteOff:
                onNoteOff(eng, e.data1);
                break;
            case MidiEvent::Type::ControlChange:
                onCC(eng, e.data1, e.data2);
                break;
            case MidiEvent::Type::PitchBend:
                onPitchBend(eng, e.pitchBend);
                break;
            default:
                break;
        }
    }
}

void EngineManager::syncMusicConfig() noexcept {
    const float bpm = globalCache_[P_BPM];

    arp_.setBPM      (bpm);
    arp_.setMode     (static_cast<ArpMode>(static_cast<int>(globalCache_[P_ARP_MODE])));
    arp_.setOctaves  (static_cast<int>(globalCache_[P_ARP_OCTAVES]));
    arp_.setRateIndex(static_cast<int>(globalCache_[P_ARP_RATE]));
    arp_.setGate     (globalCache_[P_ARP_GATE]);
    arp_.setSwing    (globalCache_[P_ARP_SWING]);
    arp_.setEnabled  (globalCache_[P_ARP_ON] > 0.5f);

    seq_.setBPM      (bpm);
    seq_.setStepCount(static_cast<int>(globalCache_[P_SEQ_STEPS]));
    seq_.setRateIndex(static_cast<int>(globalCache_[P_SEQ_RATE]));
    seq_.setGlobalGate(globalCache_[P_SEQ_GATE]);
    seq_.setSwing    (globalCache_[P_SEQ_SWING]);
    seq_.setEnabled  (globalCache_[P_SEQ_ON] > 0.5f);

    chord_.setEnabled  (globalCache_[P_CHORD_ON] > 0.5f);
    chord_.setChordType(static_cast<int>(globalCache_[P_CHORD_TYPE]));
    chord_.setInversion(static_cast<int>(globalCache_[P_CHORD_INVERSION]));

    scale_.setEnabled(globalCache_[P_SCALE_ON] > 0.5f);
    scale_.setRoot   (static_cast<int>(globalCache_[P_SCALE_ROOT]));
    scale_.setScale  (static_cast<int>(globalCache_[P_SCALE_TYPE]));
}

void EngineManager::onNoteOn(IEngine* eng, int note, int vel) noexcept {
    if (!eng) return;
    const int qNote = scale_.quantize(note);
    if (globalCache_[P_ARP_ON] > 0.5f)
        arp_.noteOn(qNote, vel);
    else
        routeNoteOn(eng, qNote, vel);
}

void EngineManager::onNoteOff(IEngine* eng, int note) noexcept {
    if (!eng) return;
    const int qNote = scale_.quantize(note);
    if (globalCache_[P_ARP_ON] > 0.5f)
        arp_.noteOff(qNote);
    else
        routeNoteOff(eng, qNote);
}

void EngineManager::routeNoteOn(IEngine* eng, int note, int vel) noexcept {
    if (!eng) return;
    const auto co = chord_.expand(note, vel);
    for (int i = 0; i < co.count; ++i)
        eng->noteOn(co.notes[i], co.velocities[i]);
}

void EngineManager::routeNoteOff(IEngine* eng, int note) noexcept {
    if (!eng) return;
    const auto co = chord_.expand(note, 0);
    for (int i = 0; i < co.count; ++i)
        eng->noteOff(co.notes[i]);
}

void EngineManager::onCC(IEngine* eng, int cc, int val) noexcept {
    if (!eng) return;
    eng->controlChange(cc, val);
}

void EngineManager::onPitchBend(IEngine* eng, int bend) noexcept {
    if (!eng) return;
    eng->pitchBend(bend);
}
