// ─────────────────────────────────────────────────────────
// FILE: core/engines/iengine.h
// BRIEF: Abstract interface every synth engine must implement
// ─────────────────────────────────────────────────────────
#pragma once
#include <cstddef>

// ════════════════════════════════════════════════════════
// IEngine — pure virtual interface for all synth engines.
//
// Threading contract:
//   init / setSampleRate / setParam / getParam → any thread
//   beginBlock / tickSample / noteOn / noteOff / ... → audio thread [RT-SAFE]
// ════════════════════════════════════════════════════════

class IEngine {
public:
    virtual ~IEngine() = default;

    // ── Setup ─────────────────────────────────────────────
    virtual void init(float sampleRate) noexcept = 0;
    virtual void setSampleRate(float sr) noexcept = 0;

    // ── Per-block hook (snapshot params before sample loop) ──
    // [RT-SAFE] — called once per processBlock before tickSample loop
    virtual void beginBlock(int /*nFrames*/) noexcept {}

    // ── Per-sample render ─────────────────────────────────
    // [RT-SAFE] — no alloc, no lock, no throw
    virtual void tickSample(float& outL, float& outR) noexcept = 0;

    // ── MIDI events (called from audio thread) ────────────
    // [RT-SAFE]
    virtual void noteOn    (int note, int vel) noexcept = 0;
    virtual void noteOff   (int note)          noexcept = 0;
    virtual void allNotesOff()                 noexcept = 0;
    virtual void allSoundOff()                 noexcept = 0;
    virtual void pitchBend (int bend)          noexcept = 0;  // -8192..+8191
    virtual void controlChange(int cc, int val) noexcept = 0;

    // ── Identity ─────────────────────────────────────────
    virtual const char* getName()     const noexcept = 0;
    virtual const char* getCategory() const noexcept = 0;

    // ── Parameter interface ──────────────────────────────
    // Engines use their own 0-based param IDs.
    // setParam is called from the UI thread (atomic store inside).
    virtual int   getParamCount()   const noexcept = 0;
    virtual void  setParam(int id, float v) noexcept = 0;
    virtual float getParam(int id)  const noexcept = 0;

    // Static metadata (no state needed)
    virtual const char* getParamName   (int id) const noexcept = 0;
    virtual float       getParamMin    (int id) const noexcept = 0;
    virtual float       getParamMax    (int id) const noexcept = 0;
    virtual float       getParamDefault(int id) const noexcept = 0;

    // ── Optional queries ─────────────────────────────────
    virtual int  getActiveVoices() const noexcept { return 0; }

    static constexpr int OSC_BUF_SIZE = 2048;
    // Fill out[OSC_BUF_SIZE] with recent samples and set writePos.
    // Default: no-op (engine doesn't provide scope feed).
    virtual void getOscBuffer(float /*out*/[OSC_BUF_SIZE],
                              int&  /*writePos*/) const noexcept {}
};
