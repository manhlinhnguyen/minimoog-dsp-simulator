// ─────────────────────────────────────────────────────────
// FILE: core/engines/mellotron/mellotron_engine.cpp
// BRIEF: Mellotron M400 engine implementation
// ─────────────────────────────────────────────────────────
#include "mellotron_engine.h"
#include <cmath>
#include <algorithm>

// Global wavetable instance (definition)
MellotronTables g_MelloTables;

// ── Static metadata ───────────────────────────────────────
const char* const MellotronEngine::PARAM_NAMES[MELLOTRON_PARAM_COUNT] = {
    "Tape", "Volume", "Pitch Spread", "Tape Speed",
    "Runout Time", "Attack", "Release",
    "Wow Depth", "Wow Rate", "Flutter Depth", "Flutter Rate"
};
const float MellotronEngine::PARAM_MIN[MELLOTRON_PARAM_COUNT] = {
    0, 0, 0, 0.8f, 2.0f, 0, 0,
    0, 0, 0, 0
};
const float MellotronEngine::PARAM_MAX[MELLOTRON_PARAM_COUNT] = {
    3, 1, 1, 1.2f, 12.0f, 1, 1,
    1, 1, 1, 1
};
const float MellotronEngine::PARAM_DEF[MELLOTRON_PARAM_COUNT] = {
    0, 0.8f, 0, 1.0f, 8.0f, 0.05f, 0.2f,
    0.15f, 0.30f, 0.10f, 0.35f
};

// ── MellotronVoice ────────────────────────────────────────

static float mMidiToHz(int note) noexcept {
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

void MellotronVoice::trigger(int midiNote, float sampleRate,
                              float tapeSpeedFactor,
                              float attackMs, float releaseMs,
                              float runoutTimeS) noexcept {
    note        = midiNote;
    active      = true;
    releasing   = false;
    phase       = 0.0f;
    heldSamples = 0.0f;

    const float freq = mMidiToHz(midiNote) * tapeSpeedFactor;
    // Phase 0..1 covers all MELLO_CYCLES cycles of the multi-cycle table
    phaseInc    = freq / (sampleRate * MELLO_CYCLES);
    pitchOffset = tapeSpeedFactor;
    runoutSamps = runoutTimeS * sampleRate;

    attackRate  = 1.0f / (attackMs  * 0.001f * sampleRate);
    releaseRate = 1.0f / (releaseMs * 0.001f * sampleRate);
    envVal      = 0.0f;
}

void MellotronVoice::tick(MellotronTape tape, float pitchFactor, float& outL, float& outR) noexcept {
    if (!active) { outL = outR = 0.0f; return; }

    // Envelope
    if (!releasing) {
        envVal = std::min(envVal + attackRate, 1.0f);
    } else {
        envVal -= releaseRate;
        if (envVal <= 0.0f) { envVal = 0.0f; active = false; }
    }

    // Tape runout (8 s hold → fade out)
    float runoutEnv = 1.0f;
    if (!releasing) {
        heldSamples += 1.0f;
        if (heldSamples > runoutSamps) {
            const float excess = heldSamples - runoutSamps;
            runoutEnv = 1.0f - std::min(excess / (runoutSamps * 0.3f), 1.0f);
        }
    }

    const float sample = g_MelloTables.read(tape, phase) * envVal * runoutEnv;
    outL = outR = sample;

    phase += phaseInc * pitchFactor;  // real-time pitch bend
    if (phase >= 1.0f) phase -= 1.0f;
}

// ── MellotronEngine ───────────────────────────────────────

MellotronEngine::MellotronEngine() {
    for (int i = 0; i < MELLOTRON_PARAM_COUNT; ++i)
        params_[i].store(PARAM_DEF[i], std::memory_order_relaxed);
}

void MellotronEngine::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    g_MelloTables.build();
}

void MellotronEngine::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
}

void MellotronEngine::beginBlock(int /*nFrames*/) noexcept {
    const int ti = std::clamp(
        static_cast<int>(params_[MP_TAPE].load(std::memory_order_relaxed)),
        0, static_cast<int>(MellotronTape::COUNT) - 1);
    tape_           = static_cast<MellotronTape>(ti);
    volume_         = params_[MP_VOLUME].load(std::memory_order_relaxed);
    pitchSpread_    = params_[MP_PITCH_SPREAD].load(std::memory_order_relaxed);
    tapeSpeed_      = params_[MP_TAPE_SPEED].load(std::memory_order_relaxed);
    runoutTimeSec_  = params_[MP_RUNOUT_TIME].load(std::memory_order_relaxed);
    attackMs_       = 5.0f + params_[MP_ATTACK].load(std::memory_order_relaxed) * 295.0f;
    releaseMs_      = 20.0f + params_[MP_RELEASE].load(std::memory_order_relaxed) * 980.0f;

    const float wowDepthRaw     = params_[MP_WOW_DEPTH].load(std::memory_order_relaxed);
    const float wowRateRaw      = params_[MP_WOW_RATE].load(std::memory_order_relaxed);
    const float flutterDepthRaw = params_[MP_FLUTTER_DEPTH].load(std::memory_order_relaxed);
    const float flutterRateRaw  = params_[MP_FLUTTER_RATE].load(std::memory_order_relaxed);

    // Depth expressed as speed percent (1.0 = 100% speed)
    wowDepthPct_     = wowDepthRaw * 0.020f;      // 0..2.0%
    wowRateHz_       = 0.1f + wowRateRaw * 2.9f;  // 0.1..3.0 Hz
    flutterDepthPct_ = flutterDepthRaw * 0.006f;  // 0..0.6%
    flutterRateHz_   = 6.0f + flutterRateRaw * 12.0f; // 6..18 Hz
}

void MellotronEngine::tickSample(float& outL, float& outR) noexcept {
    // Tape transport modulation shared across voices.
    wowPhase_ += (TWO_PI * wowRateHz_) / sampleRate_;
    if (wowPhase_ >= TWO_PI) wowPhase_ -= TWO_PI;

    flutterPhase_ += (TWO_PI * flutterRateHz_) / sampleRate_;
    if (flutterPhase_ >= TWO_PI) flutterPhase_ -= TWO_PI;

    const float wowMul     = 1.0f + wowDepthPct_ * std::sin(wowPhase_);
    const float flutterMul = 1.0f + flutterDepthPct_ * std::sin(flutterPhase_);
    const float tapeModMul = wowMul * flutterMul;

    float sumL = 0.0f, sumR = 0.0f;
    int activeVoices = 0;
    for (auto& v : voices_) {
        if (!v.active) continue;
        ++activeVoices;
        float vL, vR;
        v.tick(tape_, pitchBendCache_ * tapeModMul, vL, vR);
        sumL += vL; sumR += vR;
    }

    if (activeVoices > 0) {
        // Around -50 dBFS hiss floor, slightly scaled by active voices.
        const float hiss = nextNoise() * (0.0018f + 0.00015f * static_cast<float>(activeVoices));
        sumL += hiss;
        sumR += hiss;
    }

    outL = volume_ * sumL;
    outR = volume_ * sumR;
}

void MellotronEngine::noteOn(int note, int vel) noexcept {
    (void)vel;
    for (auto& v : voices_)
        if (v.active && v.note == note) return;  // already on

    for (auto& v : voices_) {
        if (!v.active) {
            // Small random detune for analog warmth
            const float detune = tapeSpeed_ + pitchSpread_ * 0.02f * (nextRng() - 0.5f);
            v.trigger(note, sampleRate_, detune,
                      attackMs_, releaseMs_, runoutTimeSec_);
            return;
        }
    }
}

void MellotronEngine::noteOff(int note) noexcept {
    for (auto& v : voices_)
        if (v.active && v.note == note) { v.release(); return; }
}

void MellotronEngine::allNotesOff() noexcept {
    for (auto& v : voices_) if (v.active) v.release();
}

void MellotronEngine::allSoundOff() noexcept {
    for (auto& v : voices_) v.active = false;
}

void MellotronEngine::controlChange(int cc, int val) noexcept {
    if (cc == 7) setParam(MP_VOLUME, val / 127.0f);
}

void MellotronEngine::pitchBend(int bend) noexcept {
    pitchBendSemis_ = std::clamp((bend / 8192.0f) * 2.0f, -2.0f, 2.0f);
    pitchBendCache_ = std::pow(2.0f, pitchBendSemis_ / 12.0f);
}

void MellotronEngine::setParam(int id, float v) noexcept {
    if (id < 0 || id >= MELLOTRON_PARAM_COUNT) return;
    params_[id].store(std::clamp(v, PARAM_MIN[id], PARAM_MAX[id]),
                      std::memory_order_relaxed);
}

float MellotronEngine::getParam(int id) const noexcept {
    if (id < 0 || id >= MELLOTRON_PARAM_COUNT) return 0.0f;
    return params_[id].load(std::memory_order_relaxed);
}

const char* MellotronEngine::getParamName(int id) const noexcept {
    if (id < 0 || id >= MELLOTRON_PARAM_COUNT) return "";
    return PARAM_NAMES[id];
}
float MellotronEngine::getParamMin(int id) const noexcept {
    if (id < 0 || id >= MELLOTRON_PARAM_COUNT) return 0.0f;
    return PARAM_MIN[id];
}
float MellotronEngine::getParamMax(int id) const noexcept {
    if (id < 0 || id >= MELLOTRON_PARAM_COUNT) return 1.0f;
    return PARAM_MAX[id];
}
float MellotronEngine::getParamDefault(int id) const noexcept {
    if (id < 0 || id >= MELLOTRON_PARAM_COUNT) return 0.0f;
    return PARAM_DEF[id];
}

int MellotronEngine::getActiveVoices() const noexcept {
    int n = 0;
    for (const auto& v : voices_) n += v.active ? 1 : 0;
    return n;
}
