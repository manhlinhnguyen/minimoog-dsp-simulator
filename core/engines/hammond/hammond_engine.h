// ─────────────────────────────────────────────────────────
// FILE: core/engines/hammond/hammond_engine.h
// BRIEF: Hammond B-3 tonewheel organ engine
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/iengine.h"
#include "hammond_voice.h"
#include "core/dsp/param_smoother.h"
#include <atomic>
#include <array>

// ── Hammond param IDs ─────────────────────────────────────
enum HammondParam : int {
    HP_DRAWBAR_1 = 0, HP_DRAWBAR_2, HP_DRAWBAR_3, HP_DRAWBAR_4,
    HP_DRAWBAR_5,     HP_DRAWBAR_6, HP_DRAWBAR_7, HP_DRAWBAR_8,
    HP_DRAWBAR_9,     // 0-8 feet positions
    HP_PERC_ON,       // 0/1
    HP_PERC_HARM,     // 0=2nd, 1=3rd harmonic
    HP_PERC_DECAY,    // 0=fast(600ms), 1=slow(1500ms)
    HP_PERC_LEVEL,    // 0..1
    HP_VIB_MODE,      // 0=off, 1-3=V1..V3, 4-6=C1..C3
    HP_VIB_DEPTH,     // 0..1
    HP_LESLIE_ON,     // 0/1
    HP_LESLIE_SPEED,  // 0=slow, 1=fast
    HP_LESLIE_MIX,    // 0..1 dry/wet
    HP_LESLIE_SPREAD, // 0..1 stereo width
    HP_CLICK_LEVEL,   // 0..1
    HP_OVERDRIVE,     // 0..1 (tube preamp saturation)
    HP_MASTER_VOL,    // 0..1
    HAMMOND_PARAM_COUNT
};

// ════════════════════════════════════════════════════════
// HammondEngine — 61-voice additive organ
// ════════════════════════════════════════════════════════

class HammondEngine final : public IEngine {
public:
    static constexpr int MAX_VOICES = 61;

    HammondEngine();
    ~HammondEngine() override = default;

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

    const char* getName()     const noexcept override { return "Hammond B-3"; }
    const char* getCategory() const noexcept override { return "Organ"; }

    int   getParamCount()   const noexcept override { return HAMMOND_PARAM_COUNT; }
    void  setParam(int id, float v) noexcept override;
    float getParam(int id)  const noexcept override;
    const char* getParamName   (int id) const noexcept override;
    float       getParamMin    (int id) const noexcept override;
    float       getParamMax    (int id) const noexcept override;
    float       getParamDefault(int id) const noexcept override;

    int getActiveVoices() const noexcept override;

private:
    float sampleRate_ = SAMPLE_RATE_DEFAULT;

    std::array<HammondVoice, MAX_VOICES> voices_;
    int activeCount_ = 0;

    // Cached params (snapshot in beginBlock)
    float drawbars_[9]    = {8,8,8,0,0,0,0,0,0};
    bool  percOn_         = false;
    int   percHarm_       = 0;
    float percDecayRate_  = 1.0f;
    float percLevel_      = 0.8f;
    float clickLevel_     = 0.3f;
    float overdrive_      = 0.0f;
    float masterVol_      = 0.8f;

    // Leslie params (snapshot in beginBlock)
    bool  leslieOn_        = false;
    bool  leslieFast_      = false;
    float leslieMix_       = 0.65f;
    float leslieSpread_    = 0.75f;

    // Pitch bend — real-time, all playing voices
    float pitchBendSemis_ = 0.0f;
    float pitchBendCache_ = 1.0f;   // pow(2, semis/12), updated in pitchBend()

    // Drawbar ParamSmoothing (5ms) — eliminates zipper noise on drawbar moves
    ParamSmoother drawbarSmooth_[9];
    float         drawbarsSmoothed_[9] = {8,8,8,0,0,0,0,0,0};

    // P3: Overdrive smoother (prevents clicks on drive changes)
    ParamSmoother overdriveSmoother_;

    // Vibrato/Chorus LFO
    float vibPhase_       = 0.0f;
    float vibRate_        = 6.0f;   // Hz
    int   vibMode_        = 0;      // 0=off
    float vibDepth_       = 0.5f;

    // Leslie rotor state (RT-safe, no allocation)
    float hornPhase_      = 0.0f;
    float drumPhase_      = 0.0f;
    float hornRateHz_     = 0.8f;
    float drumRateHz_     = 0.67f;
    float hornTargetHz_   = 0.8f;
    float drumTargetHz_   = 0.67f;

    float lpState_        = 0.0f; // low rotor crossover low-pass
    float hpState_        = 0.0f; // horn crossover high-pass

    static constexpr int LESLIE_DELAY_SAMPLES = 2048;
    std::array<float, LESLIE_DELAY_SAMPLES> leslieDelay_{};
    int leslieWritePos_ = 0;

    float readDelay(float delaySamples) noexcept;
    void processLeslie(float inMono, float& outL, float& outR) noexcept;

    // Percussion: fires only on first note after all released
    bool percFired_       = false;

    // Atomic param store
    std::atomic<float> params_[HAMMOND_PARAM_COUNT];

    static const char* const PARAM_NAMES[HAMMOND_PARAM_COUNT];
    static const float PARAM_MIN[HAMMOND_PARAM_COUNT];
    static const float PARAM_MAX[HAMMOND_PARAM_COUNT];
    static const float PARAM_DEF[HAMMOND_PARAM_COUNT];
};
