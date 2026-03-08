
// ═══════════════════════════════════════════════════════════════════════════
// FILE: core/engines/rhodes/rhodes_engine.h
// BRIEF: Rhodes Mark I — engine header
// ═══════════════════════════════════════════════════════════════════════════
#pragma once
#include "core/engines/iengine.h"
#include "core/dsp/param_smoother.h"
#include "rhodes_voice.h"
#include <atomic>
#include <array>

// ──────────────────────────────────────────────────────────────────────────
// Tham số v2 — gắn với vật lý hơn v1
//
//  RP_DECAY      : Tine decay (0→1 : ngắn→dài). Ảnh hưởng sustain tổng thể.
//  RP_TONE       : Pickup distance / brightness. 0=ấm tối, 1=sáng chói.
//                   Điều khiển alpha/beta nonlinearity + partial balance.
//  RP_VEL_SENS   : Velocity sensitivity. 0=fixed soft, 1=full dynamic.
//  RP_STEREO_SPREAD: Độ rộng stereo note-based panning.
//  RP_RELEASE    : Damper release time (ms). Rhodes sustain pedal feel.
//  RP_TREMOLO_RATE / DEPTH: Suitcase-style stereo autopan tremolo.
//  RP_VIBRATO_RATE / DEPTH: Pitch vibrato (FM style, pre-pickup).
//  RP_DRIVE      : Output soft saturation drive (amp grit).
//  RP_MASTER_VOL : Output gain.
// ──────────────────────────────────────────────────────────────────────────
enum RhodesParam : int {
    RP_DECAY         = 0,   // 0..1
    RP_TONE          = 1,   // 0..1
    RP_VEL_SENS      = 2,   // 0..1
    RP_STEREO_SPREAD = 3,   // 0..1
    RP_RELEASE       = 4,   // 50..2000 ms
    RP_TREMOLO_RATE  = 5,   // 0..10 Hz
    RP_TREMOLO_DEPTH = 6,   // 0..1
    RP_VIBRATO_RATE  = 7,   // 0..12 Hz
    RP_VIBRATO_DEPTH = 8,   // 0..1 (mapped to cents)
    RP_DRIVE         = 9,   // 0..1  (output saturation)
    RP_MASTER_VOL    = 10,  // 0..1
    RHODES_PARAM_COUNT
};

class RhodesEngine final : public IEngine {
public:
    static constexpr int MAX_VOICES = 12;

    RhodesEngine();
    ~RhodesEngine() override = default;

    void init(float sampleRate)         noexcept override;
    void setSampleRate(float sr)        noexcept override;
    void beginBlock(int nFrames)        noexcept override;
    void tickSample(float& outL, float& outR) noexcept override;

    void noteOn    (int note, int vel)  noexcept override;
    void noteOff   (int note)           noexcept override;
    void allNotesOff()                  noexcept override;
    void allSoundOff()                  noexcept override;
    void pitchBend (int bend)           noexcept override;
    void controlChange(int cc, int val) noexcept override;

    const char* getName()     const noexcept override { return "Rhodes Mark I"; }
    const char* getCategory() const noexcept override { return "Electric Piano"; }

    int   getParamCount()             const noexcept override { return RHODES_PARAM_COUNT; }
    void  setParam(int id, float v)         noexcept override;
    float getParam(int id)            const noexcept override;
    const char* getParamName   (int id) const noexcept override;
    float       getParamMin    (int id) const noexcept override;
    float       getParamMax    (int id) const noexcept override;
    float       getParamDefault(int id) const noexcept override;

    int getActiveVoices() const noexcept override;

private:
    float sampleRate_ = 44100.0f;

    std::array<RhodesVoice, MAX_VOICES> voices_;

    // Voice stealing: oldest-active strategy
    int  oldestVoiceIdx_  = 0;
    int  voiceAgeCounter_ = 0;
    int  voiceAge_[MAX_VOICES] = {};

    // Block-cached params (tránh atomic load mỗi sample)
    float decayParam_    = 0.4f;
    float toneParam_     = 0.5f;
    float velSens_       = 0.7f;
    float stereoSpread_  = 0.6f;
    float releaseMs_     = 300.0f;
    float tremoloRate_   = 5.0f;
    float tremoloDepth_  = 0.0f;
    float vibratoRate_   = 5.0f;
    float vibratoDepth_  = 0.0f;
    float driveParam_    = 0.3f;
    float masterVol_     = 0.8f;

    // P2: smooth high-impact timbre controls to avoid zipper noise.
    ParamSmoother decaySmoother_;
    ParamSmoother toneSmoother_;
    ParamSmoother driveSmoother_;

    // Tremolo — stereo autopan (L/R 180° out of phase)
    float tremoloPhase_  = 0.0f;
    float vibratoPhase_  = 0.0f;

    // Pitch bend — real-time modal resonator frequency update
    float pitchBendSemis_     = 0.0f;
    float prevPitchModSemis_  = 0.0f;

    // DC blocker (biquad highpass ~5Hz) — pickup model đôi khi tạo DC offset
    float dcX1L_ = 0.0f, dcY1L_ = 0.0f;
    float dcX1R_ = 0.0f, dcY1R_ = 0.0f;

    // P1.2: per-trigger micro-detuning seed (LCG, advances every noteOn)
    uint32_t detuneSeed_ = 31337u;

    // P1.4: sustain pedal state — CC64
    bool pedalDown_      = false;
    bool noteHeld_[128]  = {};  // notes deferred while pedal down

    std::atomic<float> params_[RHODES_PARAM_COUNT];
    static const char* const PARAM_NAMES[RHODES_PARAM_COUNT];
    static const float PARAM_MIN[RHODES_PARAM_COUNT];
    static const float PARAM_MAX[RHODES_PARAM_COUNT];
    static const float PARAM_DEF[RHODES_PARAM_COUNT];

    // P3.4: Cabinet biquad EQ — warm vintage amp/speaker simulation
    // Two filters: low shelf +2dB@200Hz (body) + high shelf -8dB@8kHz (rolloff)
    struct Biquad {
        float b0=1, b1=0, b2=0, a1=0, a2=0;
        float z1L=0, z2L=0, z1R=0, z2R=0;

        inline void processLR(float& L, float& R) noexcept {
            const float yL = b0*L + z1L;
            z1L = b1*L - a1*yL + z2L;
            z2L = b2*L - a2*yL;
            L   = yL;
            const float yR = b0*R + z1R;
            z1R = b1*R - a1*yR + z2R;
            z2R = b2*R - a2*yR;
            R   = yR;
        }
        void reset() noexcept { z1L = z2L = z1R = z2R = 0.0f; }
    };
    Biquad cabinetLow_;   // low shelf  +2dB @ 200Hz
    Biquad cabinetHigh_;  // high shelf -8dB @ 8kHz

    // Helpers
    int  findFreeVoice()  noexcept;
    int  findVoiceByNote(int note) noexcept;
    void applyTremolo(float& L, float& R) noexcept;
    void applyCabinet(float& L, float& R) noexcept;          // P3.4
    void computeCabinetFilters(float sr) noexcept;           // P3.4 — init coefs
    void injectSympatheticEnergy(int newNote, float energy) noexcept; // P2.3
    float dcBlock(float x, float& x1, float& y1) noexcept;
    float outputSaturate(float x, float drive) noexcept;
};
