// ─────────────────────────────────────────────────────────
// FILE: core/engines/hammond/hammond_engine.cpp
// BRIEF: Hammond B-3 engine implementation
// ─────────────────────────────────────────────────────────
#include "hammond_engine.h"
#include <cmath>
#include <algorithm>

// ── Static metadata ───────────────────────────────────────
const char* const HammondEngine::PARAM_NAMES[HAMMOND_PARAM_COUNT] = {
    "Drawbar 1 (16')", "Drawbar 2 (5⅓')", "Drawbar 3 (8')",
    "Drawbar 4 (4')",  "Drawbar 5 (2⅔')", "Drawbar 6 (2')",
    "Drawbar 7 (1⅗')", "Drawbar 8 (1⅓')", "Drawbar 9 (1')",
    "Percussion On", "Perc Harmonic", "Perc Decay",
    "Perc Level",    "Vib/Chorus Mode", "Vib Depth",
    "Leslie On", "Leslie Speed", "Leslie Mix", "Leslie Spread",
    "Click Level",   "Overdrive",   "Master Volume"
};
const float HammondEngine::PARAM_MIN[HAMMOND_PARAM_COUNT] = {
    0,0,0,0,0,0,0,0,0, 0,0,0, 0,0, 0,
    0,0,0,0,
    0,0,0
};
const float HammondEngine::PARAM_MAX[HAMMOND_PARAM_COUNT] = {
    8,8,8,8,8,8,8,8,8, 1,1,1, 1,6, 1,
    1,1,1,1,
    1,1,1
};
const float HammondEngine::PARAM_DEF[HAMMOND_PARAM_COUNT] = {
    8,8,8,0,0,0,0,0,0, 0,0,0, 0.8f,0, 0.5f,
    0,0,0.65f,0.75f,
    0.3f, 0.0f, 0.8f
};

// ── HammondVoice ──────────────────────────────────────────

static float midiToHz(int note) noexcept {
    return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
}

void HammondVoice::trigger(int midiNote, float sampleRate) noexcept {
    note      = midiNote;
    baseFreq  = midiToHz(midiNote);
    active    = true;
    for (int h = 0; h < 9; ++h) {
        phases[h]   = 0.0f;
        phaseInc[h] = (TWO_PI * baseFreq * HAMMOND_RATIOS[h]) / sampleRate;
    }
    percPhase    = 0.0f;
    percPhaseInc = (TWO_PI * baseFreq * 2.0f) / sampleRate; // 2nd harmonic default
    percEnv      = 1.0f;
    clickEnv     = 1.0f;
}

void HammondVoice::release() noexcept {
    active = false;
}

void HammondVoice::tick(const float drawbars[9],
                         bool  percOn,
                         int   percHarm,
                         float percDecayRate,
                         float percLevel,
                         float clickLevel,
                         float pitchFactor,
                         float& outL, float& outR) noexcept {
    if (!active) { outL = outR = 0.0f; return; }

    float sum = 0.0f;

    // 9 tonewheel partials
    for (int h = 0; h < 9; ++h) {
        if (drawbars[h] > 0.0f) {
            sum += (drawbars[h] / 8.0f) * std::sin(phases[h]);
        }
        phases[h] += phaseInc[h] * pitchFactor;  // real-time pitch bend
        if (phases[h] >= TWO_PI) phases[h] -= TWO_PI;
    }

    // Percussion
    if (percOn && percEnv > 0.0001f) {
        const float percFreqRatio = (percHarm == 0) ? 2.0f : 3.0f;
        percPhaseInc = (TWO_PI * baseFreq * percFreqRatio) /
                       (TWO_PI / phaseInc[2] * percFreqRatio); // reuse via phaseInc[2]
        // Simpler: directly compute
        float pInc = phaseInc[2] * percFreqRatio;
        sum += percEnv * percLevel * std::sin(percPhase);
        percPhase += pInc;
        if (percPhase >= TWO_PI) percPhase -= TWO_PI;
        percEnv *= percDecayRate;
    }

    // Key click (short white noise burst)
    if (clickEnv > 0.0001f) {
        sum += clickLevel * clickEnv * noise_.tick();
        clickEnv *= 0.93f;  // ~5ms at 44.1kHz
    }

    // Normalise: 9 drawbars each at max 1.0 → scale for headroom
    sum *= (1.0f / 9.0f);

    outL = outR = sum;
}

// ── HammondEngine ─────────────────────────────────────────

HammondEngine::HammondEngine() {
    for (int i = 0; i < HAMMOND_PARAM_COUNT; ++i)
        params_[i].store(PARAM_DEF[i], std::memory_order_relaxed);
}

void HammondEngine::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    for (int i = 0; i < 9; ++i) {
        drawbarSmooth_[i].init(sampleRate_, 5.0f);
        drawbarSmooth_[i].snapTo(drawbars_[i]);
        drawbarsSmoothed_[i] = drawbars_[i];
    }
    overdriveSmoother_.init(sampleRate_, 5.0f);
    overdriveSmoother_.snapTo(overdrive_);
}

void HammondEngine::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    for (int i = 0; i < 9; ++i)
        drawbarSmooth_[i].init(sr, 5.0f);
    overdriveSmoother_.init(sr, 5.0f);
    // Re-trigger all active voices with new sample rate
    for (auto& v : voices_)
        if (v.active) v.trigger(v.note, sr);
}

void HammondEngine::beginBlock(int /*nFrames*/) noexcept {
    // Snapshot params
    for (int h = 0; h < 9; ++h) {
        drawbars_[h] = params_[HP_DRAWBAR_1 + h].load(std::memory_order_relaxed);
        drawbarSmooth_[h].setTarget(drawbars_[h]);  // set smooth target (0..8)
    }
    percOn_     = params_[HP_PERC_ON].load(std::memory_order_relaxed) > 0.5f;
    percHarm_   = static_cast<int>(params_[HP_PERC_HARM].load(std::memory_order_relaxed));
    {
        // Decay: 0=fast(600ms)  1=slow(1500ms)
        const float decayMs = (params_[HP_PERC_DECAY].load(std::memory_order_relaxed) > 0.5f)
                             ? 1500.0f : 600.0f;
        percDecayRate_ = std::pow(0.001f, 1.0f / (decayMs * 0.001f * sampleRate_));
    }
    percLevel_  = params_[HP_PERC_LEVEL].load(std::memory_order_relaxed);
    vibMode_    = static_cast<int>(params_[HP_VIB_MODE].load(std::memory_order_relaxed));
    vibDepth_   = params_[HP_VIB_DEPTH].load(std::memory_order_relaxed);
    leslieOn_   = params_[HP_LESLIE_ON].load(std::memory_order_relaxed) > 0.5f;
    leslieFast_ = params_[HP_LESLIE_SPEED].load(std::memory_order_relaxed) > 0.5f;
    leslieMix_  = params_[HP_LESLIE_MIX].load(std::memory_order_relaxed);
    leslieSpread_ = params_[HP_LESLIE_SPREAD].load(std::memory_order_relaxed);
    clickLevel_ = params_[HP_CLICK_LEVEL].load(std::memory_order_relaxed);
    overdrive_  = params_[HP_OVERDRIVE].load(std::memory_order_relaxed);
    masterVol_  = params_[HP_MASTER_VOL].load(std::memory_order_relaxed);

    overdriveSmoother_.setTarget(overdrive_);

    hornTargetHz_ = leslieFast_ ? 6.2f : 0.8f;
    drumTargetHz_ = leslieFast_ ? 5.6f : 0.67f;
}

void HammondEngine::tickSample(float& outL, float& outR) noexcept {
    // Advance drawbar smoothers once per sample
    for (int h = 0; h < 9; ++h)
        drawbarsSmoothed_[h] = drawbarSmooth_[h].tick();

    float sumL = 0.0f, sumR = 0.0f;
    activeCount_ = 0;

    for (auto& v : voices_) {
        if (!v.active) continue;
        ++activeCount_;
        float vL, vR;
        v.tick(drawbarsSmoothed_, percOn_, percHarm_,
               percDecayRate_, percLevel_, clickLevel_,
               pitchBendCache_,
               vL, vR);
        sumL += vL;
        sumR += vR;
    }

    // P3: Tube overdrive — soft tanh saturation (pre-Leslie, post-mix)
    // drive range 1x–5x; at drive=1 output is transparent
    {
        const float od = overdriveSmoother_.tick();
        if (od > 0.001f) {
            const float drive = 1.0f + od * 4.0f;
            sumL = std::tanh(sumL * drive) / drive;
            sumR = std::tanh(sumR * drive) / drive;
        }
    }

    // True Leslie path (dual rotor + Doppler + AM)
    if (leslieOn_) {
        const float inMono = 0.5f * (sumL + sumR);
        float wetL = 0.0f, wetR = 0.0f;
        processLeslie(inMono, wetL, wetR);

        const float dry = 1.0f - leslieMix_;
        outL = masterVol_ * (dry * inMono + leslieMix_ * wetL);
        outR = masterVol_ * (dry * inMono + leslieMix_ * wetR);
        return;
    }

    // Legacy Vibrato/Chorus (scanner approximation)
    if (vibMode_ > 0) {
        vibPhase_ += (TWO_PI * vibRate_) / sampleRate_;
        if (vibPhase_ >= TWO_PI) vibPhase_ -= TWO_PI;
        const float vibLFO = std::sin(vibPhase_);
        if (vibMode_ <= 3) {
            // Vibrato: pitch modulation
            const float mod = 1.0f + vibDepth_ * 0.01f * vibLFO;
            sumL *= mod; sumR *= mod;  // simplified
        }
        // Chorus (C1-C3): dry + delayed; approximate with slight pan spread
        if (vibMode_ >= 4) {
            const float spread = vibDepth_ * 0.1f * vibLFO;
            outL = masterVol_ * (sumL * (1.0f + spread));
            outR = masterVol_ * (sumR * (1.0f - spread));
            return;
        }
    }

    outL = masterVol_ * sumL;
    outR = masterVol_ * sumR;
}

float HammondEngine::readDelay(float delaySamples) noexcept {
    const float rp = static_cast<float>(leslieWritePos_) - delaySamples;
    float wrapped = rp;
    while (wrapped < 0.0f) wrapped += static_cast<float>(LESLIE_DELAY_SAMPLES);
    while (wrapped >= static_cast<float>(LESLIE_DELAY_SAMPLES))
        wrapped -= static_cast<float>(LESLIE_DELAY_SAMPLES);

    const int i0 = static_cast<int>(wrapped);
    const int i1 = (i0 + 1) & (LESLIE_DELAY_SAMPLES - 1);
    const float frac = wrapped - static_cast<float>(i0);
    return leslieDelay_[i0] + (leslieDelay_[i1] - leslieDelay_[i0]) * frac;
}

void HammondEngine::processLeslie(float inMono, float& outL, float& outR) noexcept {
    // Speed ramp (mechanical inertia).
    hornRateHz_ += (hornTargetHz_ - hornRateHz_) * 0.0009f;
    drumRateHz_ += (drumTargetHz_ - drumRateHz_) * 0.0007f;

    hornPhase_ += (TWO_PI * hornRateHz_) / sampleRate_;
    drumPhase_ += (TWO_PI * drumRateHz_) / sampleRate_;
    if (hornPhase_ >= TWO_PI) hornPhase_ -= TWO_PI;
    if (drumPhase_ >= TWO_PI) drumPhase_ -= TWO_PI;

    // Simple crossover: low rotor (<~700 Hz), horn (>~700 Hz).
    const float lpfCoef = 0.07f;
    lpState_ += lpfCoef * (inMono - lpState_);
    const float lowBand = lpState_;
    hpState_ = inMono - lpState_;
    const float highBand = hpState_;

    // Write mono source into shared delay line.
    leslieDelay_[leslieWritePos_] = inMono;
    leslieWritePos_ = (leslieWritePos_ + 1) & (LESLIE_DELAY_SAMPLES - 1);

    // Delay modulation depths (samples) for Doppler impression.
    const float hornBase = 1.8f + 0.8f * leslieSpread_;
    const float drumBase = 3.4f + 0.9f * leslieSpread_;
    const float hornDepth = 1.4f + 1.1f * leslieSpread_;
    const float drumDepth = 0.8f + 0.7f * leslieSpread_;

    const float hornSin = std::sin(hornPhase_);
    const float drumSin = std::sin(drumPhase_);

    const float hornDelayL = hornBase + hornDepth * hornSin;
    const float hornDelayR = hornBase - hornDepth * hornSin;
    const float drumDelayL = drumBase + drumDepth * drumSin;
    const float drumDelayR = drumBase - drumDepth * drumSin;

    const float dopL = readDelay(hornDelayL) * highBand + readDelay(drumDelayL) * lowBand;
    const float dopR = readDelay(hornDelayR) * highBand + readDelay(drumDelayR) * lowBand;

    // Rotor AM shading; horn stronger than drum.
    constexpr float HALF_TURN = 3.14159265359f;
    const float hornAmL = 0.60f + 0.40f * (0.5f + 0.5f * std::cos(hornPhase_));
    const float hornAmR = 0.60f + 0.40f * (0.5f + 0.5f * std::cos(hornPhase_ + HALF_TURN));
    const float drumAmL = 0.75f + 0.25f * (0.5f + 0.5f * std::cos(drumPhase_));
    const float drumAmR = 0.75f + 0.25f * (0.5f + 0.5f * std::cos(drumPhase_ + HALF_TURN));

    outL = dopL * (0.65f * hornAmL + 0.35f * drumAmL);
    outR = dopR * (0.65f * hornAmR + 0.35f * drumAmR);
}

void HammondEngine::noteOn(int note, int /*vel*/) noexcept {
    // Find existing or free slot
    for (auto& v : voices_) {
        if (v.active && v.note == note) return; // already pressed
    }
    // Percussion: only fires if no other notes held
    bool anyActive = false;
    for (const auto& v : voices_) anyActive |= v.active;
    percFired_ = anyActive;  // if notes already held, no perc

    // Find free slot
    for (auto& v : voices_) {
        if (!v.active) {
            v.trigger(note, sampleRate_);
            if (percFired_) v.percEnv = 0.0f; // suppress perc
            return;
        }
    }
}

void HammondEngine::noteOff(int note) noexcept {
    for (auto& v : voices_)
        if (v.active && v.note == note) { v.release(); return; }
}

void HammondEngine::allNotesOff() noexcept {
    for (auto& v : voices_) v.active = false;
    percFired_ = false;
}

void HammondEngine::allSoundOff() noexcept {
    allNotesOff();
}

void HammondEngine::controlChange(int cc, int val) noexcept {
    if (cc == 7) setParam(HP_MASTER_VOL, val / 127.0f);
}

void HammondEngine::pitchBend(int bend) noexcept {
    pitchBendSemis_ = std::clamp((bend / 8192.0f) * 2.0f, -2.0f, 2.0f);
    pitchBendCache_ = std::pow(2.0f, pitchBendSemis_ / 12.0f);
}

void HammondEngine::setParam(int id, float v) noexcept {
    if (id < 0 || id >= HAMMOND_PARAM_COUNT) return;
    params_[id].store(std::clamp(v, PARAM_MIN[id], PARAM_MAX[id]),
                      std::memory_order_relaxed);
}

float HammondEngine::getParam(int id) const noexcept {
    if (id < 0 || id >= HAMMOND_PARAM_COUNT) return 0.0f;
    return params_[id].load(std::memory_order_relaxed);
}

const char* HammondEngine::getParamName(int id) const noexcept {
    if (id < 0 || id >= HAMMOND_PARAM_COUNT) return "";
    return PARAM_NAMES[id];
}

float HammondEngine::getParamMin(int id) const noexcept {
    if (id < 0 || id >= HAMMOND_PARAM_COUNT) return 0.0f;
    return PARAM_MIN[id];
}

float HammondEngine::getParamMax(int id) const noexcept {
    if (id < 0 || id >= HAMMOND_PARAM_COUNT) return 1.0f;
    return PARAM_MAX[id];
}

float HammondEngine::getParamDefault(int id) const noexcept {
    if (id < 0 || id >= HAMMOND_PARAM_COUNT) return 0.0f;
    return PARAM_DEF[id];
}

int HammondEngine::getActiveVoices() const noexcept {
    return activeCount_;
}
