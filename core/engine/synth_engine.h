// ─────────────────────────────────────────────────────────
// FILE: core/engine/synth_engine.h
// BRIEF: Top-level audio processor — orchestrates all subsystems
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/voice/voice_pool.h"
#include "core/music/arpeggiator.h"
#include "core/music/sequencer.h"
#include "core/music/chord_engine.h"
#include "core/music/scale_quantizer.h"
#include "shared/interfaces.h"
#include "shared/params.h"
#include <array>

// ════════════════════════════════════════════════════════
// SYNTH ENGINE — top-level audio processor
// Owns all subsystems (pre-allocated).
// Called from RtAudio callback — ZERO allocation, ZERO locks.
// ════════════════════════════════════════════════════════

class SynthEngine : public IAudioProcessor {
public:
    SynthEngine()  = default;
    ~SynthEngine() = default;

    SynthEngine(const SynthEngine&) = delete;
    SynthEngine& operator=(const SynthEngine&) = delete;

    // ── Init (call before audio starts) ──────────────────
    void init(AtomicParamStore* params,
              MidiEventQueue*   midiQueue) noexcept;
    void setSampleRate(float sr) override;
    void setBlockSize (int bs)   override;

    // ── Audio callback (RtAudio thread) ──────────────────
    // [RT-SAFE]
    void processBlock(sample_t* outL,
                      sample_t* outR,
                      int nFrames) noexcept override;

    // ── Read-only queries (safe from UI thread) ───────────
    int   getActiveVoices() const noexcept;
    int   getArpNote()      const noexcept;
    int   getSeqStep()      const noexcept;
    bool  getSeqPlaying()   const noexcept;
    float getBPM()          const noexcept;

    // ── Oscilloscope ring buffer (audio writes, UI reads) ─────
    // Acceptable benign data race — pure visualization, no RT-safety needed.
    static constexpr int OSC_BUF_SIZE = 2048;
    void getOscBuffer(float out[OSC_BUF_SIZE], int& writePos) const noexcept;

    // ── Sequencer step editing (UI thread only) ────────────
    // Writes directly to seq_ step array. Technically a
    // benign data race (UI writes, audio reads small struct),
    // acceptable in DSP practice — worst case is a 1-block glitch.
    void    setSeqStep(int idx, const SeqStep& s) noexcept;
    SeqStep getSeqStep(int idx)             const noexcept;

private:
    // Subsystems (pre-allocated, no heap use in audio thread)
    VoicePool      voicePool_;
    Arpeggiator    arp_;
    StepSequencer  seq_;
    ChordEngine    chord_;
    ScaleQuantizer scale_;

    // External wiring (set at init, read-only thereafter)
    AtomicParamStore* params_    = nullptr;
    MidiEventQueue*   midiQueue_ = nullptr;

    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    int   blockSize_  = BLOCK_SIZE_DEFAULT;

    // Per-block param snapshot (avoids per-sample atomic overhead)
    float paramCache_[PARAM_COUNT] = {};

    // State tracking
    bool seqWasPlaying_ = false;

    // Oscilloscope ring buffer (audio thread writes)
    float oscBuf_[OSC_BUF_SIZE] = {};
    int   oscWritePos_          = 0;

    // ── Private helpers ───────────────────────────────────
    void snapshotParams()       noexcept;
    void drainMidiQueue()       noexcept;
    void syncMusicConfig()      noexcept;

    void onNoteOn   (int note, int vel) noexcept;
    void onNoteOff  (int note)          noexcept;
    void onCC       (int cc,  int val)  noexcept;
    void onPitchBend(int bend)          noexcept;

    // Routes note through chord engine → voice pool
    void routeNoteOn (int note, int vel) noexcept;
    void routeNoteOff(int note)          noexcept;
};
