// ─────────────────────────────────────────────────────────
// FILE: core/engines/dx7/dx7_engine.h
// BRIEF: Yamaha DX7 6-operator FM synth engine
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/iengine.h"
#include "dx7_voice.h"
#include <atomic>
#include <array>

// DX7 params:
//   0    : Algorithm (1-32)
//   1-6  : Op 0..5 Ratio
//   7-12 : Op 0..5 Level
//   13-18: Op 0..5 Attack
//   19-24: Op 0..5 Decay
//   25-30: Op 0..5 Sustain
//   31-36: Op 0..5 Release
//   37-42: Op 0..5 VelSens
//   43-48: Op 0..5 FixedMode (0=ratio,1=fixedHz)
//   49-54: Op 0..5 FixedHz (20..8000)
//   55-60: Op 0..5 KbdRateScale (0..1)
//   61   : Feedback
//   62   : Master Volume

enum DX7Param : int {
    DX7P_ALGORITHM = 0,
    DX7P_OP0_RATIO,  DX7P_OP1_RATIO,  DX7P_OP2_RATIO,
    DX7P_OP3_RATIO,  DX7P_OP4_RATIO,  DX7P_OP5_RATIO,
    DX7P_OP0_LEVEL,  DX7P_OP1_LEVEL,  DX7P_OP2_LEVEL,
    DX7P_OP3_LEVEL,  DX7P_OP4_LEVEL,  DX7P_OP5_LEVEL,
    DX7P_OP0_ATK,    DX7P_OP1_ATK,    DX7P_OP2_ATK,
    DX7P_OP3_ATK,    DX7P_OP4_ATK,    DX7P_OP5_ATK,
    DX7P_OP0_DEC,    DX7P_OP1_DEC,    DX7P_OP2_DEC,
    DX7P_OP3_DEC,    DX7P_OP4_DEC,    DX7P_OP5_DEC,
    DX7P_OP0_SUS,    DX7P_OP1_SUS,    DX7P_OP2_SUS,
    DX7P_OP3_SUS,    DX7P_OP4_SUS,    DX7P_OP5_SUS,
    DX7P_OP0_REL,    DX7P_OP1_REL,    DX7P_OP2_REL,
    DX7P_OP3_REL,    DX7P_OP4_REL,    DX7P_OP5_REL,
    DX7P_OP0_VEL,    DX7P_OP1_VEL,    DX7P_OP2_VEL,
    DX7P_OP3_VEL,    DX7P_OP4_VEL,    DX7P_OP5_VEL,
    DX7P_OP0_FIXED_MODE, DX7P_OP1_FIXED_MODE, DX7P_OP2_FIXED_MODE,
    DX7P_OP3_FIXED_MODE, DX7P_OP4_FIXED_MODE, DX7P_OP5_FIXED_MODE,
    DX7P_OP0_FIXED_HZ, DX7P_OP1_FIXED_HZ, DX7P_OP2_FIXED_HZ,
    DX7P_OP3_FIXED_HZ, DX7P_OP4_FIXED_HZ, DX7P_OP5_FIXED_HZ,
    DX7P_OP0_KBD_RATE, DX7P_OP1_KBD_RATE, DX7P_OP2_KBD_RATE,
    DX7P_OP3_KBD_RATE, DX7P_OP4_KBD_RATE, DX7P_OP5_KBD_RATE,
    DX7P_FEEDBACK,
    DX7P_MASTER_VOL,
    DX7_PARAM_COUNT
};

// ════════════════════════════════════════════════════════
// DX7Engine — 8-voice 6-operator FM synthesizer
// ════════════════════════════════════════════════════════

class DX7Engine final : public IEngine {
public:
    static constexpr int MAX_VOICES = 8;

    DX7Engine();
    ~DX7Engine() override = default;

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

    const char* getName()     const noexcept override { return "Yamaha DX7"; }
    const char* getCategory() const noexcept override { return "FM Synth"; }

    int   getParamCount()   const noexcept override { return DX7_PARAM_COUNT; }
    void  setParam(int id, float v) noexcept override;
    float getParam(int id)  const noexcept override;
    const char* getParamName   (int id) const noexcept override;
    float       getParamMin    (int id) const noexcept override;
    float       getParamMax    (int id) const noexcept override;
    float       getParamDefault(int id) const noexcept override;

    int getActiveVoices() const noexcept override;

private:
    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    std::array<DX7Voice, MAX_VOICES> voices_;

    // Voice stealing: oldest-active strategy (replaces round-robin voiceIdx_)
    int   voiceAge_[MAX_VOICES]  = {};
    int   voiceAgeCounter_       = 0;

    // Pitch bend — real-time all playing operators
    float pitchBendSemis_ = 0.0f;
    float pitchBendCache_ = 1.0f;   // pow(2, semis/12)

    // Cached params
    int   algorithm_  = 0;
    float ratios_[6]  = {1,1,1,1,1,1};
    float levels_[6]  = {1,1,0,0,0,0};
    float attacks_[6] = {};
    float decays_[6]  = {0.5f,0.5f,0.5f,0.5f,0.5f,0.5f};
    float sustains_[6]= {0.7f,0.7f,0.7f,0.7f,0.7f,0.7f};
    float releases_[6]= {0.3f,0.3f,0.3f,0.3f,0.3f,0.3f};
    float velSens_[6] = {0.5f,0.5f,0.5f,0.5f,0.5f,0.5f};
    bool  fixedMode_[6] = {false,false,false,false,false,false};
    float fixedHz_[6]   = {440.f,440.f,440.f,440.f,440.f,440.f};
    float kbdRate_[6]   = {0.f,0.f,0.f,0.f,0.f,0.f};
    float feedback_   = 0.0f;
    float masterVol_  = 0.8f;

    std::atomic<float> params_[DX7_PARAM_COUNT];

    static const char* const PARAM_NAMES[DX7_PARAM_COUNT];
};
