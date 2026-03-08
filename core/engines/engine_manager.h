// ─────────────────────────────────────────────────────────
// FILE: core/engines/engine_manager.h
// BRIEF: Multi-engine manager — owns music layer + active engine
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/iengine.h"
#include "core/music/arpeggiator.h"
#include "core/music/sequencer.h"
#include "core/music/chord_engine.h"
#include "core/music/scale_quantizer.h"
#include "core/music/midi_file_player.h"
#include "shared/interfaces.h"
#include "shared/params.h"
#include <atomic>
#include <array>
#include <memory>

// ════════════════════════════════════════════════════════
// ENGINE MANAGER — top-level IAudioProcessor
//
// Owns the music layer (Arp / Seq / Chord / Scale) shared
// across all engines.  Each registered IEngine owns its
// own params; the global AtomicParamStore holds only
// BPM + music params (P_BPM .. PARAM_COUNT-1 range).
//
// Thread model:
//   UI thread   → switchEngine(), setGlobalParam(), engine()->setParam()
//   Audio thread → processBlock() → drainMidiQueue() → tickSample()
// ════════════════════════════════════════════════════════

class EngineManager : public IAudioProcessor {
public:
    static constexpr int MAX_ENGINES = 8;

    EngineManager();
    ~EngineManager() override = default;

    // ── Init ──────────────────────────────────────────────
    void init(AtomicParamStore* globalParams,
              MidiEventQueue*   midiQueue) noexcept;

    // ── Engine registration (call before audio starts) ────
    // Takes ownership.  Returns index, -1 on failure.
    int registerEngine(std::unique_ptr<IEngine> engine) noexcept;

    // ── Engine selection (UI thread) ──────────────────────
    // allSoundOff on old engine, then atomically switches.
    void switchEngine(int idx) noexcept;
    int  getActiveIndex() const noexcept;
    int  getEngineCount() const noexcept { return engineCount_; }

    // Raw pointer — valid for UI read only (engine lives forever once registered)
    IEngine* getEngine(int idx) const noexcept;
    IEngine* getActiveEngine() const noexcept;

    // ── Global params (BPM / music) ───────────────────────
    void  setGlobalParam(int id, float v) noexcept;
    float getGlobalParam(int id) const noexcept;

    // ── Sequencer step editing (UI thread only) ───────────
    void    setSeqStep(int idx, const SeqStep& s) noexcept;
    SeqStep getSeqStep(int idx)             const noexcept;

    // ── Queries (UI thread read) ──────────────────────────
    int  getActiveVoices()    const noexcept;
    // Oscilloscope feed — works for all engines (captured at engine output)
    static constexpr int OSC_BUF_SIZE = IEngine::OSC_BUF_SIZE;  // 2048
    void getOscBuffer      (float out [OSC_BUF_SIZE], int& writePos) const noexcept;
    void getOscBufferStereo(float outL[OSC_BUF_SIZE], float outR[OSC_BUF_SIZE],
                            int& writePos) const noexcept;
    int  getArpNote()         const noexcept;
    int  getSeqCurrentStep()  const noexcept;  // current playback step
    bool getSeqPlaying()      const noexcept;
    float getBPM()            const noexcept;

    // ── MIDI file player ─────────────────────────────────
    MidiFilePlayer& getMidiPlayer() noexcept { return midiPlayer_; }

    // ── IAudioProcessor ───────────────────────────────────
    // [RT-SAFE]
    void processBlock(sample_t* outL,
                      sample_t* outR,
                      int nFrames) noexcept override;

    void setSampleRate(float sr) noexcept override;
    void setBlockSize (int bs)   noexcept override;

private:
    // ── Engines ────────────────────────────────────────────
    std::array<std::unique_ptr<IEngine>, MAX_ENGINES> engines_;
    int             engineCount_ = 0;
    std::atomic<int> activeIdx_  {0};

    // ── MIDI file player ─────────────────────────────────
    MidiFilePlayer midiPlayer_;

    // ── Music layer ───────────────────────────────────────
    Arpeggiator    arp_;
    StepSequencer  seq_;
    ChordEngine    chord_;
    ScaleQuantizer scale_;

    // ── External refs ─────────────────────────────────────
    AtomicParamStore* globalParams_ = nullptr;
    MidiEventQueue*   midiQueue_    = nullptr;

    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    int   blockSize_  = BLOCK_SIZE_DEFAULT;

    // Per-block snapshot of global params (BPM / music)
    float globalCache_[PARAM_COUNT] = {};

    // State
    bool seqWasPlaying_  = false;
    bool midiWasPlaying_ = false;

    // ── Oscilloscope ring buffer (written by audio thread, read by UI) ───
    float oscBufL_[IEngine::OSC_BUF_SIZE] = {};
    float oscBufR_[IEngine::OSC_BUF_SIZE] = {};
    int   oscWritePos_                     = 0;

    // ── Private helpers ───────────────────────────────────
    void snapshotGlobal()  noexcept;
    void drainMidiQueue()  noexcept;
    void syncMusicConfig() noexcept;

    void onNoteOn    (IEngine* eng, int note, int vel) noexcept;
    void onNoteOff   (IEngine* eng, int note)          noexcept;
    void onCC        (IEngine* eng, int cc,  int val)  noexcept;
    void onPitchBend (IEngine* eng, int bend)          noexcept;

    // Note routing through chord + scale
    void routeNoteOn (IEngine* eng, int note, int vel) noexcept;
    void routeNoteOff(IEngine* eng, int note)          noexcept;
};
