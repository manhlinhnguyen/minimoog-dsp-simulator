
// ═══════════════════════════════════════════════════════════════════════════
// FILE: core/engines/rhodes/rhodes_engine.cpp
// BRIEF: Rhodes Mark I — engine implementation
// ═══════════════════════════════════════════════════════════════════════════
#include "rhodes_engine.h"
#include <cmath>
#include <algorithm>
#include <limits>

// ──────────────────────────────────────────────────────────────────────────
// Param tables
// ──────────────────────────────────────────────────────────────────────────
const char* const RhodesEngine::PARAM_NAMES[RHODES_PARAM_COUNT] = {
    "Decay", "Tone", "Vel Sensitivity",
    "Stereo Spread", "Release",
    "Tremolo Rate", "Tremolo Depth",
    "Vibrato Rate", "Vibrato Depth",
    "Drive", "Master Volume"
};
const float RhodesEngine::PARAM_MIN[RHODES_PARAM_COUNT] = {
    0.0f, 0.0f, 0.0f, 0.0f,  50.0f,
    0.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 0.0f
};
const float RhodesEngine::PARAM_MAX[RHODES_PARAM_COUNT] = {
    1.0f, 1.0f, 1.0f, 1.0f, 2000.0f,
    10.0f, 1.0f,
    12.0f, 1.0f,
    1.0f, 1.0f
};
const float RhodesEngine::PARAM_DEF[RHODES_PARAM_COUNT] = {
//  Decay  Tone  VelSens  Spread  Release  TremRate  TremDepth  VibRate VibDepth  Drive  Vol
    0.4f,  0.5f,  0.7f,   0.6f,  300.0f,   5.0f,     0.0f,      5.2f,   0.0f,   0.3f,  0.8f
};

// ──────────────────────────────────────────────────────────────────────────
// Constructor / Init
// ──────────────────────────────────────────────────────────────────────────
RhodesEngine::RhodesEngine() {
    for (int i = 0; i < RHODES_PARAM_COUNT; ++i)
        params_[i].store(PARAM_DEF[i], std::memory_order_relaxed);

    for (int i = 0; i < MAX_VOICES; ++i)
        voiceAge_[i] = 0;
}

void RhodesEngine::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    tremoloPhase_ = 0.0f;
    dcX1L_ = dcY1L_ = dcX1R_ = dcY1R_ = 0.0f;
    decaySmoother_.init(sampleRate_, 5.0f);
    toneSmoother_.init(sampleRate_, 5.0f);
    driveSmoother_.init(sampleRate_, 5.0f);
    decaySmoother_.snapTo(decayParam_);
    toneSmoother_.snapTo(toneParam_);
    driveSmoother_.snapTo(driveParam_);
    computeCabinetFilters(sampleRate);
}

void RhodesEngine::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    decaySmoother_.init(sr, 5.0f);
    toneSmoother_.init(sr, 5.0f);
    driveSmoother_.init(sr, 5.0f);
    computeCabinetFilters(sr);
}

// ──────────────────────────────────────────────────────────────────────────
// beginBlock — cache params một lần mỗi block (tránh atomic/sample)
// ──────────────────────────────────────────────────────────────────────────
void RhodesEngine::beginBlock(int /*nFrames*/) noexcept {
    decayParam_   = params_[RP_DECAY].load(std::memory_order_relaxed);
    toneParam_    = params_[RP_TONE].load(std::memory_order_relaxed);
    velSens_      = params_[RP_VEL_SENS].load(std::memory_order_relaxed);
    stereoSpread_ = params_[RP_STEREO_SPREAD].load(std::memory_order_relaxed);
    releaseMs_    = params_[RP_RELEASE].load(std::memory_order_relaxed);
    tremoloRate_  = params_[RP_TREMOLO_RATE].load(std::memory_order_relaxed);
    tremoloDepth_ = params_[RP_TREMOLO_DEPTH].load(std::memory_order_relaxed);
    vibratoRate_  = params_[RP_VIBRATO_RATE].load(std::memory_order_relaxed);
    vibratoDepth_ = params_[RP_VIBRATO_DEPTH].load(std::memory_order_relaxed);
    driveParam_   = params_[RP_DRIVE].load(std::memory_order_relaxed);
    masterVol_    = params_[RP_MASTER_VOL].load(std::memory_order_relaxed);

    decaySmoother_.setTarget(decayParam_);
    toneSmoother_.setTarget(toneParam_);
    driveSmoother_.setTarget(driveParam_);
}

// ──────────────────────────────────────────────────────────────────────────
// Voice management helpers
// ──────────────────────────────────────────────────────────────────────────
int RhodesEngine::findFreeVoice() noexcept {
    // 1. Tìm voice không active
    for (int i = 0; i < MAX_VOICES; ++i)
        if (!voices_[i].active) return i;

    // 2. Steal voice cũ nhất (oldest active)
    int oldest = 0;
    int minAge = voiceAge_[0];
    for (int i = 1; i < MAX_VOICES; ++i) {
        if (voiceAge_[i] < minAge) {
            minAge  = voiceAge_[i];
            oldest  = i;
        }
    }
    return oldest;
}

int RhodesEngine::findVoiceByNote(int note) noexcept {
    for (int i = 0; i < MAX_VOICES; ++i)
        if (voices_[i].active && voices_[i].note == note) return i;
    return -1;
}

// ──────────────────────────────────────────────────────────────────────────
// tickSample — hot path [RT-SAFE]
// ──────────────────────────────────────────────────────────────────────────
void RhodesEngine::tickSample(float& outL, float& outR) noexcept {
    const float decaySmoothed = decaySmoother_.tick();
    const float toneSmoothed  = toneSmoother_.tick();
    const float driveSmoothed = driveSmoother_.tick();
    decayParam_ = decaySmoothed;
    toneParam_  = toneSmoothed;

    float pitchModSemis = pitchBendSemis_;
    if (vibratoDepth_ > 0.0001f) {
        vibratoPhase_ += (TWO_PI * vibratoRate_) / sampleRate_;
        if (vibratoPhase_ >= TWO_PI) vibratoPhase_ -= TWO_PI;
        // Max vibrato depth: +-35 cents
        pitchModSemis += std::sin(vibratoPhase_) * (vibratoDepth_ * 0.35f);
    }

    // Real-time pitch modulation: bend + vibrato (coefficient update, no state reset)
    if (pitchModSemis != prevPitchModSemis_) {
        for (auto& v : voices_)
            if (v.active) v.updatePitchFactor(pitchModSemis, sampleRate_);
        prevPitchModSemis_ = pitchModSemis;
    }

    float sumL = 0.0f, sumR = 0.0f;

    for (auto& v : voices_) {
        if (!v.active) continue;
        float vL, vR;
        v.tick(vL, vR);
        sumL += vL;
        sumR += vR;
    }

    // DC block — xử lý riêng biệt L và R
    sumL = dcBlock(sumL, dcX1L_, dcY1L_);
    sumR = dcBlock(sumR, dcX1R_, dcY1R_);

    // Tremolo stereo autopan (suitcase style)
    applyTremolo(sumL, sumR);

    // Polyphony normalization — bù đắp khi nhiều voices cùng lúc
    // 1/sqrt(MAX_VOICES) = 1/3.46 — equal-power summing
    constexpr float POLY_NORM = 0.2887f;  // 1/sqrt(12)
    sumL *= POLY_NORM;
    sumR *= POLY_NORM;

    // Gentle output saturation — tanh soft limiter
    const float drive = 1.0f + driveSmoothed * 1.5f;  // 1.0..2.5x
    sumL = outputSaturate(sumL, drive);
    sumR = outputSaturate(sumR, drive);

    // P3.4: Cabinet/amp simulation (low shelf +2dB@200Hz, high shelf -8dB@8kHz)
    applyCabinet(sumL, sumR);

    outL = masterVol_ * sumL;
    outR = masterVol_ * sumR;
}

// ──────────────────────────────────────────────────────────────────────────
// Tremolo — stereo autopan kiểu Rhodes Suitcase
// L và R 180° out of phase → tạo cảm giác xoay vòng thực hơn
// ──────────────────────────────────────────────────────────────────────────
void RhodesEngine::applyTremolo(float& L, float& R) noexcept {
    if (tremoloDepth_ < 0.001f) return;

    tremoloPhase_ += (TWO_PI * tremoloRate_) / sampleRate_;
    if (tremoloPhase_ >= TWO_PI) tremoloPhase_ -= TWO_PI;

    // Equal-power stereo tremolo:
    // gainL và gainR đối pha → khi L sáng thì R tối và ngược lại
    const float lfo    = std::sin(tremoloPhase_);
    const float depth  = tremoloDepth_ * 0.5f;
    const float gainL  = 1.0f - depth * (1.0f + lfo);   // 1→(1-depth) khi lfo=+1
    const float gainR  = 1.0f - depth * (1.0f - lfo);   // 1→(1-depth) khi lfo=-1

    L *= gainL;
    R *= gainR;
}

// ──────────────────────────────────────────────────────────────────────────
// DC blocker — single-pole highpass (~5 Hz)
// y[n] = x[n] - x[n-1] + R·y[n-1],  R = 1 - (2π·fc/sr)
// ──────────────────────────────────────────────────────────────────────────
float RhodesEngine::dcBlock(float x, float& x1, float& y1) noexcept {
    // R gần 1.0 → cutoff rất thấp ~5Hz, trong suốt với audio
    constexpr float R = 0.9997f;
    const float y = x - x1 + R * y1;
    x1 = x;
    y1 = y;
    return y;
}

// ──────────────────────────────────────────────────────────────────────────
// Output saturator — tanh soft limiter
//
// tanh(x·drive)/drive:
//   - Tín hiệu nhỏ: output ≈ x (hoàn toàn tuyến tính, trong suốt)
//   - Tín hiệu lớn: saturates mượt mà tại ±1/drive
//   - Không có phase discontinuity hay hard knee → nghe sạch hơn cubic clip
//   - drive=1.0: max output = ±1.0,   drive=2.5: max = ±0.40
//
// Với POLY_NORM và VOICE_NORM đã scale, single voice ≈ 0.5 peak
// → chỉ clip nhẹ khi ≥4 voices full velocity đồng thời.
// ──────────────────────────────────────────────────────────────────────────
float RhodesEngine::outputSaturate(float x, float drive) noexcept {
    return std::tanh(x * drive) / drive;
}

// ──────────────────────────────────────────────────────────────────────────
// Note events
// ──────────────────────────────────────────────────────────────────────────
void RhodesEngine::noteOn(int note, int vel) noexcept {
    if (vel == 0) { noteOff(note); return; }

    // P1.4: retrigger clears any pedal-held state for this note
    noteHeld_[note] = false;

    // Nếu note đang active → retrigger trực tiếp (legato hit)
    int idx = findVoiceByNote(note);
    if (idx < 0) idx = findFreeVoice();

    // P1.2: compute per-trigger micro-detuning ±2 cents (analog pitch variation)
    detuneSeed_ = detuneSeed_ * 1664525u + 1013904223u;
    const int   detuneRaw   = static_cast<int>(detuneSeed_ >> 16) % 9 - 4;  // -4..+4
    const float detuneCents = detuneRaw * 0.5f;                              // ±2 cents

    voiceAge_[idx] = ++voiceAgeCounter_;
    voices_[idx].trigger(note, vel, sampleRate_,
                         decayParam_, toneParam_,
                         velSens_, stereoSpread_, detuneCents);

    // P2.3: sympathetic resonance — nudge harmonically-related sustaining voices
    injectSympatheticEnergy(note, vel / 127.0f * 0.4f);
}

void RhodesEngine::noteOff(int note) noexcept {
    // P1.4: if sustain pedal is held, defer the release
    if (pedalDown_) {
        noteHeld_[note] = true;
        return;
    }
    const int idx = findVoiceByNote(note);
    if (idx >= 0)
        voices_[idx].release(releaseMs_);
}

void RhodesEngine::allNotesOff() noexcept {
    for (auto& v : voices_)
        if (v.active) v.release(200.0f);
}

void RhodesEngine::allSoundOff() noexcept {
    for (auto& v : voices_) v.kill();
    dcX1L_ = dcY1L_ = dcX1R_ = dcY1R_ = 0.0f;
    cabinetLow_.reset();
    cabinetHigh_.reset();
    pedalDown_ = false;
    for (auto& h : noteHeld_) h = false;
}

void RhodesEngine::controlChange(int cc, int val) noexcept {
    switch (cc) {
        case 7:  setParam(RP_MASTER_VOL,    val / 127.0f);         break;
        case 11: setParam(RP_MASTER_VOL,    val / 127.0f);         break;  // expression
        case 1:  setParam(RP_TREMOLO_DEPTH, val / 127.0f);         break;  // mod wheel → tremolo
        case 64:  // P1.4: sustain pedal
            pedalDown_ = (val >= 64);
            if (!pedalDown_) {
                // Pedal released: trigger deferred note-offs
                for (int n = 0; n < 128; ++n) {
                    if (noteHeld_[n]) {
                        const int idx = findVoiceByNote(n);
                        if (idx >= 0) voices_[idx].release(releaseMs_);
                        noteHeld_[n] = false;
                    }
                }
            }
            break;
        default: break;
    }
}

void RhodesEngine::pitchBend(int bend) noexcept {
    pitchBendSemis_ = std::clamp((bend / 8192.0f) * 2.0f, -2.0f, 2.0f);
}

// ──────────────────────────────────────────────────────────────────────────
// Param interface
// ──────────────────────────────────────────────────────────────────────────
void RhodesEngine::setParam(int id, float v) noexcept {
    if (id < 0 || id >= RHODES_PARAM_COUNT) return;
    params_[id].store(std::clamp(v, PARAM_MIN[id], PARAM_MAX[id]),
                      std::memory_order_relaxed);
}

float RhodesEngine::getParam(int id) const noexcept {
    if (id < 0 || id >= RHODES_PARAM_COUNT) return 0.0f;
    return params_[id].load(std::memory_order_relaxed);
}

const char* RhodesEngine::getParamName(int id) const noexcept {
    if (id < 0 || id >= RHODES_PARAM_COUNT) return "";
    return PARAM_NAMES[id];
}

float RhodesEngine::getParamMin(int id) const noexcept {
    if (id < 0 || id >= RHODES_PARAM_COUNT) return 0.0f;
    return PARAM_MIN[id];
}

float RhodesEngine::getParamMax(int id) const noexcept {
    if (id < 0 || id >= RHODES_PARAM_COUNT) return 1.0f;
    return PARAM_MAX[id];
}

float RhodesEngine::getParamDefault(int id) const noexcept {
    if (id < 0 || id >= RHODES_PARAM_COUNT) return 0.0f;
    return PARAM_DEF[id];
}

int RhodesEngine::getActiveVoices() const noexcept {
    int n = 0;
    for (const auto& v : voices_) n += v.active ? 1 : 0;
    return n;
}

// ──────────────────────────────────────────────────────────────────────────
// P3.4 Cabinet filter — computeCabinetFilters
//
// Audio EQ Cookbook (R. Bristow-Johnson) biquad shelf formulas.
// alpha = sin(w0)/2 * sqrt(2)  (shelf slope S=1.0, simplifies to this)
//
// Low shelf  +2dB  @ 200Hz: adds body/warmth (cabinet resonance boost)
// High shelf -8dB  @ 8kHz:  speaker rolloff (vintage amp air loss)
// ──────────────────────────────────────────────────────────────────────────
void RhodesEngine::computeCabinetFilters(float sr) noexcept {
    constexpr float SQRT2_HALF = 0.70711f;  // 1/sqrt(2) = sin(w0)/2*sqrt(2) shorthand

    // ── Low shelf +2dB @ 200Hz ───────────────────────────────────────────
    {
        const float A   = 1.12202f;  // 10^(2/40)
        const float w0  = TWO_PI * 200.0f / sr;
        const float cw  = std::cos(w0);
        const float sw  = std::sin(w0);
        const float sqA = std::sqrt(A);
        const float alp = sw * SQRT2_HALF;   // sin(w0)/2 * sqrt(2)
        const float a0  = (A+1) + (A-1)*cw + 2*sqA*alp;
        cabinetLow_.b0  =  A*((A+1) - (A-1)*cw + 2*sqA*alp) / a0;
        cabinetLow_.b1  = 2*A*((A-1) - (A+1)*cw)            / a0;
        cabinetLow_.b2  =  A*((A+1) - (A-1)*cw - 2*sqA*alp) / a0;
        cabinetLow_.a1  = -2*((A-1) + (A+1)*cw)              / a0;
        cabinetLow_.a2  =    ((A+1) + (A-1)*cw - 2*sqA*alp)  / a0;
        cabinetLow_.reset();
    }

    // ── High shelf -8dB @ 8kHz ───────────────────────────────────────────
    {
        const float A   = 0.63096f;  // 10^(-8/40)
        const float w0  = TWO_PI * 8000.0f / sr;
        const float cw  = std::cos(w0);
        const float sw  = std::sin(w0);
        const float sqA = std::sqrt(A);
        const float alp = sw * SQRT2_HALF;
        const float a0  = (A+1) - (A-1)*cw + 2*sqA*alp;
        cabinetHigh_.b0 =  A*((A+1) + (A-1)*cw + 2*sqA*alp) / a0;
        cabinetHigh_.b1 = -2*A*((A-1) + (A+1)*cw)           / a0;
        cabinetHigh_.b2 =  A*((A+1) + (A-1)*cw - 2*sqA*alp) / a0;
        cabinetHigh_.a1 =  2*((A-1) - (A+1)*cw)             / a0;
        cabinetHigh_.a2 =    ((A+1) - (A-1)*cw - 2*sqA*alp) / a0;
        cabinetHigh_.reset();
    }
}

// ──────────────────────────────────────────────────────────────────────────
// P3.4 Cabinet apply — [RT-SAFE]
// ──────────────────────────────────────────────────────────────────────────
void RhodesEngine::applyCabinet(float& L, float& R) noexcept {
    cabinetLow_.processLR(L, R);
    cabinetHigh_.processLR(L, R);
}

// ──────────────────────────────────────────────────────────────────────────
// P2.3 Sympathetic resonance — [RT-SAFE]
//
// When a new note is struck, sustaining voices whose pitch is harmonically
// related receive a small energy injection (simulating acoustic coupling
// through the keyboard and soundboard).
//
// Coupling strengths by interval (semitones mod 12):
//   Unison/Octave (0): 0.015   Fifth (7): 0.010   Fourth (5): 0.008
// ──────────────────────────────────────────────────────────────────────────
void RhodesEngine::injectSympatheticEnergy(int newNote, float energy) noexcept {
    static constexpr float COUPLING[12] = {
        0.015f, 0.0f, 0.0f, 0.0f, 0.0f, 0.008f,
        0.0f,   0.010f, 0.0f, 0.0f, 0.0f, 0.0f
    };
    for (auto& v : voices_) {
        if (!v.active) continue;
        const int interval = std::abs(v.note - newNote) % 12;
        const float coupling = COUPLING[interval];
        if (coupling > 0.0f)
            v.modes[0].y0 += energy * coupling;
    }
}
