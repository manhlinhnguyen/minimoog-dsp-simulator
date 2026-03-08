// ─────────────────────────────────────────────────────────
// FILE: core/engines/moog/moog_engine.h
// BRIEF: Moog Model D synth engine — IEngine implementation
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/iengine.h"
#include "voice_pool.h"
#include "moog_params.h"
#include <atomic>

// ════════════════════════════════════════════════════════
// MoogEngine
// Full Moog DSP (VoicePool + oscBuf) implementing IEngine.
// Owns its own atomic param array — no shared global store.
// ════════════════════════════════════════════════════════

class MoogEngine final : public IEngine {
public:
    MoogEngine();
    ~MoogEngine() override = default;

    // ── IEngine ──────────────────────────────────────────
    void init(float sampleRate) noexcept override;
    void setSampleRate(float sr) noexcept override;
    void beginBlock(int nFrames) noexcept override;
    void tickSample(float& outL, float& outR) noexcept override;

    void noteOn    (int note, int vel)  noexcept override;
    void noteOff   (int note)           noexcept override;
    void allNotesOff()                  noexcept override;
    void allSoundOff()                  noexcept override;
    void pitchBend (int bend)           noexcept override;
    void controlChange(int cc, int val) noexcept override;

    const char* getName()     const noexcept override { return "MiniMoog Model D"; }
    const char* getCategory() const noexcept override { return "Synth"; }

    int   getParamCount()          const noexcept override { return MOOG_PARAM_COUNT; }
    void  setParam(int id, float v)      noexcept override;
    float getParam(int id)         const noexcept override;

    const char* getParamName   (int id) const noexcept override;
    float       getParamMin    (int id) const noexcept override;
    float       getParamMax    (int id) const noexcept override;
    float       getParamDefault(int id) const noexcept override;

    int  getActiveVoices() const noexcept override;
    void getOscBuffer(float out[OSC_BUF_SIZE],
                      int&  writePos) const noexcept override;

private:
    VoicePool voicePool_;

    // Engine-owned param store — UI thread writes, audio thread reads via snapshot
    std::atomic<float> params_[MOOG_PARAM_COUNT] = {};

    float sampleRate_ = SAMPLE_RATE_DEFAULT;

    // Param snapshot written by beginBlock(), read by tickSample()
    float        paramCache_[MOOG_PARAM_COUNT] = {};
    const float* paramCachePtr_ = nullptr;

    // Oscilloscope ring buffer
    float oscBuf_[OSC_BUF_SIZE] = {};
    int   oscWritePos_           = 0;
};
