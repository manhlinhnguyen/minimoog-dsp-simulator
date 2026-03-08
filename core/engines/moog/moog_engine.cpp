// ─────────────────────────────────────────────────────────
// FILE: core/engines/moog/moog_engine.cpp
// BRIEF: MoogEngine implementation
// ─────────────────────────────────────────────────────────
#include "moog_engine.h"
#include <algorithm>
#include <cmath>

MoogEngine::MoogEngine() {
    for (int i = 0; i < MOOG_PARAM_COUNT; ++i)
        params_[i].store(MOOG_PARAM_META[i].defaultVal, std::memory_order_relaxed);
}

void MoogEngine::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    voicePool_.init(sampleRate_);
}

void MoogEngine::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    voicePool_.init(sr);
}

void MoogEngine::beginBlock(int /*nFrames*/) noexcept { // [RT-SAFE]
    for (int i = 0; i < MOOG_PARAM_COUNT; ++i)
        paramCache_[i] = params_[i].load(std::memory_order_relaxed);
    paramCachePtr_ = paramCache_;
    voicePool_.applyConfig(paramCachePtr_);
}

void MoogEngine::tickSample(float& outL, float& outR) noexcept { // [RT-SAFE]
    float l = 0.0f, r = 0.0f;
    voicePool_.tick(paramCachePtr_, l, r);

    // P2: gentle post-VCA limiter to control stacked-voice peaks.
    constexpr float kLimiterDrive = 1.25f;
    outL = std::tanh(l * kLimiterDrive) / kLimiterDrive;
    outR = std::tanh(r * kLimiterDrive) / kLimiterDrive;

    oscBuf_[oscWritePos_] = outL;
    oscWritePos_ = (oscWritePos_ + 1) & (OSC_BUF_SIZE - 1);
}

void MoogEngine::noteOn (int note, int vel) noexcept { voicePool_.noteOn(note, vel); }
void MoogEngine::noteOff(int note)          noexcept { voicePool_.noteOff(note); }
void MoogEngine::allNotesOff()              noexcept { voicePool_.allNotesOff(); }
void MoogEngine::allSoundOff()              noexcept { voicePool_.allSoundOff(); }

void MoogEngine::pitchBend(int bend) noexcept {
    const float semi = std::clamp((bend / 8192.0f) * 2.0f,
        MOOG_PARAM_META[MP_MASTER_TUNE].minVal, MOOG_PARAM_META[MP_MASTER_TUNE].maxVal);
    setParam(MP_MASTER_TUNE, semi);
}

void MoogEngine::controlChange(int cc, int val) noexcept {
    switch (cc) {
        case 1:  setParam(MP_MOD_MIX,    val / 127.0f); break;
        case 7:  setParam(MP_MASTER_VOL, val / 127.0f); break;
        case 64: if (val < 64) voicePool_.allNotesOff();  break;
        case 120: case 123: voicePool_.allSoundOff();      break;
        default: break;
    }
}

void MoogEngine::setParam(int id, float v) noexcept {
    if (id < 0 || id >= MOOG_PARAM_COUNT) return;
    v = std::clamp(v, MOOG_PARAM_META[id].minVal, MOOG_PARAM_META[id].maxVal);
    params_[id].store(v, std::memory_order_relaxed);
}

float MoogEngine::getParam(int id) const noexcept {
    if (id < 0 || id >= MOOG_PARAM_COUNT) return 0.0f;
    return params_[id].load(std::memory_order_relaxed);
}

const char* MoogEngine::getParamName(int id) const noexcept {
    if (id < 0 || id >= MOOG_PARAM_COUNT) return "";
    return MOOG_PARAM_META[id].name;
}
float MoogEngine::getParamMin(int id) const noexcept {
    if (id < 0 || id >= MOOG_PARAM_COUNT) return 0.0f;
    return MOOG_PARAM_META[id].minVal;
}
float MoogEngine::getParamMax(int id) const noexcept {
    if (id < 0 || id >= MOOG_PARAM_COUNT) return 1.0f;
    return MOOG_PARAM_META[id].maxVal;
}
float MoogEngine::getParamDefault(int id) const noexcept {
    if (id < 0 || id >= MOOG_PARAM_COUNT) return 0.0f;
    return MOOG_PARAM_META[id].defaultVal;
}

int MoogEngine::getActiveVoices() const noexcept {
    return voicePool_.getActiveCount();
}

void MoogEngine::getOscBuffer(float out[OSC_BUF_SIZE], int& writePos) const noexcept {
    writePos = oscWritePos_;
    for (int i = 0; i < OSC_BUF_SIZE; ++i) out[i] = oscBuf_[i];
}
