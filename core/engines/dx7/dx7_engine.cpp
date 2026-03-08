// ─────────────────────────────────────────────────────────
// FILE: core/engines/dx7/dx7_engine.cpp
// BRIEF: DX7 6-operator FM engine implementation
// ─────────────────────────────────────────────────────────
#include "dx7_engine.h"
#include <cmath>
#include <algorithm>

// ── Static metadata ───────────────────────────────────────

const char* const DX7Engine::PARAM_NAMES[DX7_PARAM_COUNT] = {
    "Algorithm",
    "Op1 Ratio","Op2 Ratio","Op3 Ratio","Op4 Ratio","Op5 Ratio","Op6 Ratio",
    "Op1 Level","Op2 Level","Op3 Level","Op4 Level","Op5 Level","Op6 Level",
    "Op1 Attack","Op2 Attack","Op3 Attack","Op4 Attack","Op5 Attack","Op6 Attack",
    "Op1 Decay","Op2 Decay","Op3 Decay","Op4 Decay","Op5 Decay","Op6 Decay",
    "Op1 Sustain","Op2 Sustain","Op3 Sustain","Op4 Sustain","Op5 Sustain","Op6 Sustain",
    "Op1 Release","Op2 Release","Op3 Release","Op4 Release","Op5 Release","Op6 Release",
    "Op1 VelSens","Op2 VelSens","Op3 VelSens","Op4 VelSens","Op5 VelSens","Op6 VelSens",
    "Op1 Fixed Mode","Op2 Fixed Mode","Op3 Fixed Mode","Op4 Fixed Mode","Op5 Fixed Mode","Op6 Fixed Mode",
    "Op1 Fixed Hz","Op2 Fixed Hz","Op3 Fixed Hz","Op4 Fixed Hz","Op5 Fixed Hz","Op6 Fixed Hz",
    "Op1 KbdRate","Op2 KbdRate","Op3 KbdRate","Op4 KbdRate","Op5 KbdRate","Op6 KbdRate",
    "Feedback",
    "Master Volume"
};

static const float _dx7min[DX7_PARAM_COUNT] = {
    1,  // algorithm 1..32
    0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,  // ratio
    0,0,0,0,0,0,                      // level
    0,0,0,0,0,0,                      // attack
    0,0,0,0,0,0,                      // decay
    0,0,0,0,0,0,                      // sustain
    0,0,0,0,0,0,                      // release
    0,0,0,0,0,0,                      // velSens
    0,0,0,0,0,0,                      // fixed mode
    20,20,20,20,20,20,                // fixed Hz
    0,0,0,0,0,0,                      // kbd rate scale
    0,                                 // feedback
    0                                  // masterVol
};
static const float _dx7max[DX7_PARAM_COUNT] = {
    32,
    8,8,8,8,8,8,
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    1,1,1,1,1,1,
    8000,8000,8000,8000,8000,8000,
    1,1,1,1,1,1,                      // kbd rate scale
    1,
    1
};
static const float _dx7def[DX7_PARAM_COUNT] = {
    1,
    1,1,1,1,1,1,
    1,1,0.5f,0.5f,0.3f,0.3f,
    0.01f,0.01f,0.01f,0.01f,0.01f,0.01f,
    0.3f,0.3f,0.5f,0.5f,0.5f,0.5f,
    0.7f,0.7f,0.5f,0.5f,0.5f,0.5f,
    0.2f,0.2f,0.3f,0.3f,0.3f,0.3f,
    0.5f,0.5f,0.5f,0.5f,0.5f,0.5f,
    0,0,0,0,0,0,
    440,440,440,440,440,440,
    0,0,0,0,0,0,                      // kbd rate scale (default = off)
    0.0f,
    0.8f
};


// ── DX7Operator ───────────────────────────────────────────

static float dx7MidiToHz(int note) noexcept {
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

void DX7Operator::trigger(float baseFreq, float sampleRate,
                           float r, float lv, float vs,
                           bool  isFixedMode, float hzFixed,
                           float atk, float dec, float sus, float rel,
                           int velocity, float kbdRateScale,
                           int midiNote) noexcept {
    ratio    = r;
    level    = lv;
    velSens  = vs;
    phase    = 0.0f;
    fixedMode = isFixedMode;
    fixedHz   = std::clamp(hzFixed, 20.0f, 8000.0f);
    const float freqHz = fixedMode ? fixedHz : (baseFreq * ratio);
    phaseInc = (TWO_PI * freqHz) / sampleRate;
    output   = 0.0f;

    const float velScale = 1.0f - vs + vs * (velocity / 127.0f);
    sustainLvl = sus * level * velScale;

    // Keyboard rate scaling: higher notes → faster envelopes
    // kbdRateScale 0..1 → scale factor 1.0..4.0 per octave from C4 (note 60)
    // scaleFactor = kbdScaleBase ^ ((note - 60) / 12)
    float kbdFactor = 1.0f;
    if (kbdRateScale > 0.001f) {
        const float kbdScaleBase = 1.0f + kbdRateScale * 3.0f; // 1..4 per octave
        kbdFactor = std::pow(kbdScaleBase, (midiNote - 60) / 12.0f);
    }

    // Rate -> per-sample exponential coefficients (DX-style contour)
    const float atkMs = (1.0f + atk * 2999.0f) / kbdFactor;
    attackRate  = std::pow(0.001f, 1.0f / (atkMs * 0.001f * sampleRate));

    const float decMs = (1.0f + dec * 4999.0f) / kbdFactor;
    decayRate   = std::pow(0.001f, 1.0f / (decMs * 0.001f * sampleRate));

    const float relMs = (1.0f + rel * 2999.0f) / kbdFactor;
    releaseRate = std::pow(0.001f, 1.0f / (relMs * 0.001f * sampleRate));

    envVal = 0.0f;
    stage  = Stage::Attack;
}

void DX7Operator::release() noexcept {
    if (stage != Stage::Off) stage = Stage::Release;
}

float DX7Operator::tick(float pitchFactor) noexcept {
    // Envelope
    switch (stage) {
        case Stage::Attack:
            envVal = 1.0f - (1.0f - envVal) * attackRate;
            if (envVal >= 0.9995f) { envVal = 1.0f; stage = Stage::Decay; }
            break;
        case Stage::Decay:
            envVal = sustainLvl + (envVal - sustainLvl) * decayRate;
            if (envVal <= sustainLvl + 0.0001f) { envVal = sustainLvl; stage = Stage::Sustain; }
            break;
        case Stage::Sustain:
            envVal = sustainLvl;
            break;
        case Stage::Release:
            envVal *= releaseRate;
            if (envVal < 0.00001f) { envVal = 0.0f; stage = Stage::Off; }
            break;
        case Stage::Off:
            output = 0.0f;
            return 0.0f;
    }
    output = envVal * level * std::sin(phase);
    phase += phaseInc * (fixedMode ? 1.0f : pitchFactor);
    if (phase >= TWO_PI) phase -= TWO_PI;
    return output;
}

// ── DX7Voice ──────────────────────────────────────────────

void DX7Voice::trigger(int midiNote, int vel,
                        float sampleRate,
                        int   algorithm,
                        const float ratios[NUM_OPS],
                        const bool  fixedMode[NUM_OPS],
                        const float fixedHz[NUM_OPS],
                        const float levels[NUM_OPS],
                        const float vs[NUM_OPS],
                        const float attacks[NUM_OPS],
                        const float decays[NUM_OPS],
                        const float sustains[NUM_OPS],
                        const float releases[NUM_OPS],
                        float feedbackAmount,
                        const float kbdRate[NUM_OPS]) noexcept {
    note        = midiNote;
    velocity    = vel;
    active      = true;
    feedback    = feedbackAmount;
    feedbackBuf = 0.0f;

    const float baseFreq = dx7MidiToHz(midiNote);
    for (int i = 0; i < NUM_OPS; ++i) {
        const float kr = kbdRate ? kbdRate[i] : 0.0f;
        ops[i].trigger(baseFreq, sampleRate,
                       ratios[i], levels[i], vs[i],
                       fixedMode[i], fixedHz[i],
                       attacks[i], decays[i], sustains[i], releases[i],
                       vel, kr, midiNote);
    }
    (void)algorithm;
}

void DX7Voice::release() noexcept {
    for (auto& op : ops) op.release();
}

void DX7Voice::tick(int algorithm, float pitchFactor, float& outL, float& outR) noexcept {
    if (!active) { outL = outR = 0.0f; return; }

    const DX7Algorithm& alg = DX7_ALGORITHMS[algorithm];

    // Compute modulator signals
    float modSignal[NUM_OPS] = {};

    // Feedback on op 0
    modSignal[0] += feedback * feedbackBuf;

    // Accumulate mod sources
    for (int op = 0; op < NUM_OPS; ++op) {
        const int src = alg.mod_src[op];
        for (int s = 0; s < NUM_OPS; ++s) {
            if (src & (1 << s)) modSignal[op] += ops[s].output;
        }
    }

    // Tick each operator with its mod input
    float carrier_sum = 0.0f;
    bool allOff = true;
    for (int op = 0; op < NUM_OPS; ++op) {
        // Apply modulation to phase before tick
        ops[op].phase += modSignal[op];
        const float out = ops[op].tick(pitchFactor);  // pass pitch factor here
        if (ops[op].stage != DX7Operator::Stage::Off) allOff = false;

        if (alg.carriers & (1 << op)) carrier_sum += out;
    }

    feedbackBuf = ops[0].output;

    if (allOff) active = false;

    outL = outR = carrier_sum * 0.25f; // scale for headroom
}

// ── DX7Engine ─────────────────────────────────────────────

DX7Engine::DX7Engine() {
    for (int i = 0; i < DX7_PARAM_COUNT; ++i)
        params_[i].store(_dx7def[i], std::memory_order_relaxed);
}

void DX7Engine::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
}

void DX7Engine::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
}

void DX7Engine::beginBlock(int /*nFrames*/) noexcept {
    algorithm_ = std::clamp(static_cast<int>(
        params_[DX7P_ALGORITHM].load(std::memory_order_relaxed)) - 1, 0, 31);

    for (int i = 0; i < 6; ++i) {
        ratios_[i]  = params_[DX7P_OP0_RATIO  + i].load(std::memory_order_relaxed);
        levels_[i]  = params_[DX7P_OP0_LEVEL  + i].load(std::memory_order_relaxed);
        attacks_[i] = params_[DX7P_OP0_ATK    + i].load(std::memory_order_relaxed);
        decays_[i]  = params_[DX7P_OP0_DEC    + i].load(std::memory_order_relaxed);
        sustains_[i]= params_[DX7P_OP0_SUS    + i].load(std::memory_order_relaxed);
        releases_[i]= params_[DX7P_OP0_REL    + i].load(std::memory_order_relaxed);
        velSens_[i] = params_[DX7P_OP0_VEL    + i].load(std::memory_order_relaxed);
        fixedMode_[i] = params_[DX7P_OP0_FIXED_MODE + i].load(std::memory_order_relaxed) > 0.5f;
        fixedHz_[i]   = params_[DX7P_OP0_FIXED_HZ   + i].load(std::memory_order_relaxed);
        kbdRate_[i]   = params_[DX7P_OP0_KBD_RATE    + i].load(std::memory_order_relaxed);
    }
    feedback_  = params_[DX7P_FEEDBACK].load(std::memory_order_relaxed);
    masterVol_ = params_[DX7P_MASTER_VOL].load(std::memory_order_relaxed);
}

void DX7Engine::tickSample(float& outL, float& outR) noexcept {
    float sumL = 0.0f, sumR = 0.0f;
    for (auto& v : voices_) {
        if (!v.active) continue;
        float vL, vR;
        v.tick(algorithm_, pitchBendCache_, vL, vR);
        sumL += vL; sumR += vR;
    }
    outL = masterVol_ * sumL;
    outR = masterVol_ * sumR;
}

void DX7Engine::noteOn(int note, int vel) noexcept {
    // Oldest-active voice stealing (replaces round-robin)
    int best = -1;
    for (int i = 0; i < MAX_VOICES; ++i)
        if (!voices_[i].active) { best = i; break; }
    if (best < 0) {
        // Steal oldest: find voice with the smallest (earliest) age
        int minAge = voiceAge_[0];
        best = 0;
        for (int i = 1; i < MAX_VOICES; ++i) {
            if (voiceAge_[i] < minAge) { minAge = voiceAge_[i]; best = i; }
        }
    }
    voiceAge_[best] = ++voiceAgeCounter_;

    voices_[best].trigger(note, vel, sampleRate_,
                          algorithm_,
                          ratios_, fixedMode_, fixedHz_,
                          levels_, velSens_,
                          attacks_, decays_, sustains_, releases_,
                          feedback_ * 4.0f,
                          kbdRate_);
}

void DX7Engine::noteOff(int note) noexcept {
    for (auto& v : voices_)
        if (v.active && v.note == note) { v.release(); return; }
}

void DX7Engine::allNotesOff() noexcept {
    for (auto& v : voices_) v.release();
}

void DX7Engine::allSoundOff() noexcept {
    for (auto& v : voices_) v.active = false;
}

void DX7Engine::controlChange(int cc, int val) noexcept {
    if (cc == 7) setParam(DX7P_MASTER_VOL, val / 127.0f);
}

void DX7Engine::pitchBend(int bend) noexcept {
    pitchBendSemis_ = std::clamp((bend / 8192.0f) * 2.0f, -2.0f, 2.0f);
    pitchBendCache_ = std::pow(2.0f, pitchBendSemis_ / 12.0f);
}

void DX7Engine::setParam(int id, float v) noexcept {
    if (id < 0 || id >= DX7_PARAM_COUNT) return;
    params_[id].store(std::clamp(v, _dx7min[id], _dx7max[id]),
                      std::memory_order_relaxed);
}

float DX7Engine::getParam(int id) const noexcept {
    if (id < 0 || id >= DX7_PARAM_COUNT) return 0.0f;
    return params_[id].load(std::memory_order_relaxed);
}

const char* DX7Engine::getParamName(int id) const noexcept {
    if (id < 0 || id >= DX7_PARAM_COUNT) return "";
    return PARAM_NAMES[id];
}
float DX7Engine::getParamMin(int id) const noexcept {
    if (id < 0 || id >= DX7_PARAM_COUNT) return 0.0f;
    return _dx7min[id];
}
float DX7Engine::getParamMax(int id) const noexcept {
    if (id < 0 || id >= DX7_PARAM_COUNT) return 1.0f;
    return _dx7max[id];
}
float DX7Engine::getParamDefault(int id) const noexcept {
    if (id < 0 || id >= DX7_PARAM_COUNT) return 0.0f;
    return _dx7def[id];
}

int DX7Engine::getActiveVoices() const noexcept {
    int n = 0;
    for (const auto& v : voices_) n += v.active ? 1 : 0;
    return n;
}
