// ─────────────────────────────────────────────────────────
// FILE: core/engines/drums/drum_engine.h
// BRIEF: Hybrid drum machine — 8 DSP pads + 8 sample pads
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/iengine.h"
#include "drum_pad_dsp.h"
#include "drum_pad_sample.h"
#include <atomic>
#include <array>
#include <string>

// ════════════════════════════════════════════════════════
// Per-pad params: 4 per pad × 16 pads = 64 params
// Plus: global volume, reserved → 66 total
//
// Pad layout:
//   Pad 0-7  : DSP synthesis (MIDI notes 36-43)
//   Pad 8-15 : Sample playback (MIDI notes 44-51)
//   Follows General MIDI drum map starting at note 36 (Kick)
// ════════════════════════════════════════════════════════

constexpr int DRUM_PADS     = 16;
constexpr int DSP_PADS      = 8;
constexpr int SAMPLE_PADS   = 8;

// Param IDs: [padIdx*4 + paramType] for pads 0-15, then global
enum DrumParamType : int {
    DP_VOLUME = 0,
    DP_PITCH  = 1,
    DP_DECAY  = 2,
    DP_PAN    = 3,
};
constexpr int DRUM_PAD_PARAMS     = 4;
constexpr int DRUM_GLOBAL_VOL_ID  = DRUM_PADS * DRUM_PAD_PARAMS;  // = 64
constexpr int DRUM_KICK_SWEEP_DEPTH_ID = DRUM_GLOBAL_VOL_ID + 1;  // = 65
constexpr int DRUM_KICK_SWEEP_TIME_ID  = DRUM_GLOBAL_VOL_ID + 2;  // = 66
constexpr int DRUM_PARAM_COUNT    = DRUM_GLOBAL_VOL_ID + 3;        // = 67

inline int drumParamId(int pad, DrumParamType type) noexcept {
    return pad * DRUM_PAD_PARAMS + static_cast<int>(type);
}

static const char* const DSP_PAD_NAMES[DSP_PADS] = {
    "Kick", "Snare", "Hi-Hat C", "Hi-Hat O",
    "Clap", "Tom Lo", "Tom Mid", "Rimshot"
};
static const int DSP_MIDI_NOTES[DSP_PADS] = { 36,38,42,46,39,41,43,37 };
static const int SAMPLE_MIDI_NOTES[SAMPLE_PADS] = { 44,45,47,48,49,50,51,52 };

// ════════════════════════════════════════════════════════
class DrumEngine final : public IEngine {
public:
    DrumEngine();
    ~DrumEngine() override = default;

    void init(float sampleRate) noexcept override;
    void setSampleRate(float sr) noexcept override;
    void beginBlock(int nFrames) noexcept override;
    void tickSample(float& outL, float& outR) noexcept override;

    void noteOn    (int note, int vel)  noexcept override;
    void noteOff   (int /*note*/)       noexcept override {}  // drum = one-shot
    void allNotesOff()                  noexcept override;
    void allSoundOff()                  noexcept override;
    void pitchBend (int /*bend*/)       noexcept override {}
    void controlChange(int cc, int val) noexcept override;

    const char* getName()     const noexcept override { return "Hybrid Drum Machine"; }
    const char* getCategory() const noexcept override { return "Drums"; }

    int   getParamCount()   const noexcept override { return DRUM_PARAM_COUNT; }
    void  setParam(int id, float v) noexcept override;
    float getParam(int id)  const noexcept override;
    const char* getParamName   (int id) const noexcept override;
    float       getParamMin    (int id) const noexcept override;
    float       getParamMax    (int id) const noexcept override;
    float       getParamDefault(int id) const noexcept override;

    int getActiveVoices() const noexcept override;

    // ── HAL interface — called once before audio starts ───
    // Load a WAV into a sample pad slot (pad 8..15).
    // Ownership of audioL/audioR data is transferred.
    void loadSamplePad(int padIdx,
                       std::vector<float> audioL,
                       std::vector<float> audioR,
                       float srHz,
                       const std::string& name) noexcept;

    // For UI: get sample pad name
    const std::string& getSampleName(int samplePadIdx) const noexcept;

private:
    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    float globalVol_  = 0.8f;
    float kickSweepDepth_ = 0.5f;  // 0..1 → sweep intensity
    float kickSweepTime_  = 0.2f;  // 0..1 → 10ms..200ms

    std::array<DrumPadDsp,    DSP_PADS>    dspPads_;
    std::array<DrumPadSample, SAMPLE_PADS> samplePads_;

    // Cached per-pad params (updated in beginBlock)
    struct PadCache { float vol, pitch, decay, pan; };
    std::array<PadCache, DRUM_PADS> padCache_;

    std::atomic<float> params_[DRUM_PARAM_COUNT];

    // Trigger by pad index (0..15)
    void triggerPad(int padIdx, int velocity) noexcept;
};
