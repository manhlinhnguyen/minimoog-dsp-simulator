// ─────────────────────────────────────────────────────────
// FILE: core/engines/mellotron/mellotron_engine.h
// BRIEF: Mellotron M400 tape emulation engine
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/iengine.h"
#include "mellotron_voice.h"
#include <atomic>
#include <array>

enum MellotronParam : int {
    MP_TAPE = 0,        // 0=Strings, 1=Choir, 2=Flute, 3=Brass
    MP_VOLUME,          // 0..1
    MP_PITCH_SPREAD,    // 0..1 (random detune between voices)
    MP_TAPE_SPEED,      // 0.8..1.2
    MP_RUNOUT_TIME,     // 2..12 seconds
    MP_ATTACK,          // 0..1 → 5ms..300ms
    MP_RELEASE,         // 0..1 → 20ms..1000ms
    MP_WOW_DEPTH,       // 0..1 (mapped to 0..2% speed)
    MP_WOW_RATE,        // 0..1 (mapped to 0.1..3 Hz)
    MP_FLUTTER_DEPTH,   // 0..1 (mapped to 0..0.6% speed)
    MP_FLUTTER_RATE,    // 0..1 (mapped to 6..18 Hz)
    MELLOTRON_PARAM_COUNT
};

class MellotronEngine final : public IEngine {
public:
    static constexpr int MAX_VOICES = 35;   // full M400 keyboard

    MellotronEngine();
    ~MellotronEngine() override = default;

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

    const char* getName()     const noexcept override { return "Mellotron M400"; }
    const char* getCategory() const noexcept override { return "Tape"; }

    int   getParamCount()   const noexcept override { return MELLOTRON_PARAM_COUNT; }
    void  setParam(int id, float v) noexcept override;
    float getParam(int id)  const noexcept override;
    const char* getParamName   (int id) const noexcept override;
    float       getParamMin    (int id) const noexcept override;
    float       getParamMax    (int id) const noexcept override;
    float       getParamDefault(int id) const noexcept override;

    int getActiveVoices() const noexcept override;

private:
    float sampleRate_   = SAMPLE_RATE_DEFAULT;
    std::array<MellotronVoice, MAX_VOICES> voices_;

    // Cached params
    MellotronTape tape_     = MellotronTape::Strings;
    float volume_           = 0.8f;
    float pitchSpread_      = 0.0f;
    float tapeSpeed_        = 1.0f;
    float runoutTimeSec_    = 8.0f;
    float attackMs_         = 20.0f;
    float releaseMs_        = 200.0f;
    float wowDepthPct_      = 0.0f;
    float wowRateHz_        = 0.7f;
    float flutterDepthPct_  = 0.0f;
    float flutterRateHz_    = 10.0f;

    // Modulation oscillators (engine-level tape transport wow/flutter)
    float wowPhase_         = 0.0f;
    float flutterPhase_     = 0.0f;

    // Pitch bend — real-time tape speed factor
    float pitchBendSemis_ = 0.0f;
    float pitchBendCache_ = 1.0f;   // pow(2, semis/12)

    std::atomic<float> params_[MELLOTRON_PARAM_COUNT];

    static const char* const PARAM_NAMES[MELLOTRON_PARAM_COUNT];
    static const float       PARAM_MIN[MELLOTRON_PARAM_COUNT];
    static const float       PARAM_MAX[MELLOTRON_PARAM_COUNT];
    static const float       PARAM_DEF[MELLOTRON_PARAM_COUNT];

    // Simple LCG for pitch spread per-note jitter
    uint32_t rng_ = 31337u;
    float nextRng() noexcept {
        rng_ = rng_ * 1664525u + 1013904223u;
        return (static_cast<float>(rng_) / 4294967296.0f);
    }

    // P2: constant tape hiss bed when at least one voice is active.
    float nextNoise() noexcept {
        return nextRng() * 2.0f - 1.0f;
    }
};
