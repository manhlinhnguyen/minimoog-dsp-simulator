// ═══════════════════════════════════════════════════════════════════════════
// FILE: core/engines/rhodes/rhodes_voice.h
// BRIEF: Rhodes Mark I — 4-Mode Modal Resonator + Pickup Nonlinearity
//
// Kiến trúc tín hiệu:
//
//   [Hammer Exciter]
//         │  velocity-shaped impulse burst (1.5–8 ms)
//         │  + key click noise burst (2 ms broadband)    ← P1.3
//         ▼
//   [4 Modal Resonators — biquad IIR]
//    mode0: fundamental (1.000×)       → decay dài nhất (sustain)
//    mode1: 2nd partial  (2.045×)      → decay 40%      (body/warmth)
//    mode2: bell partial (5.979×)      → decay 12%      (chime attack)
//    mode3: high partial (9.210×)      → decay  3%      (attack transient)
//         │  x(t) = tổng displacement
//         ▼
//   [Pickup Polynomial Waveshaper]
//    y = x + α·x² + β·x³
//    α, β phụ thuộc cả Tone lẫn Velocity                ← P1.1
//    x² → even harmonics (bell chime)
//    x³ → odd  harmonics (warmth/bark)
//         │  normalized × VOICE_NORM
//         ▼
//   [Key Click Noise Burst] (2 ms, mixed vào output)    ← P1.3
//         ▼
//   [Release Damper Envelope] → kill khi quiet
//         │
//         ▼
//    stereo equal-power pan → outL / outR
//
// KHÔNG có soft saturator ở tầng voice. Chỉ một tầng tanh nhẹ ở engine
// output để handle clipping khi nhiều voice đồng thời.
//
// Spectral ratios theo: Tolonen (1998) + thực nghiệm Rhodes Mark I.
// ═══════════════════════════════════════════════════════════════════════════
#pragma once
#include "shared/types.h"
#include "core/dsp/noise.h"
#include <cmath>
#include <algorithm>

namespace rhodes_detail {

// Velocity → hammer impulse width (samples): vel cao → burst ngắn → brighter
inline float hammerWidthSamples(int vel, float sr) noexcept {
    const float ms = 8.0f - 6.5f * (vel / 127.0f);  // 8ms → 1.5ms
    return ms * 0.001f * sr;
}

// Smoothstep velocity curve — tránh linear response cứng nhắc
inline float velCurve(int vel) noexcept {
    const float v = vel / 127.0f;
    return v * v * (3.0f - 2.0f * v);
}

// Inharmonic offset (semitones) cho từng mode — từ đo spectrum Rhodes thực
inline float inharmonicOffset(int midiNote, int modeIdx) noexcept {
    // mode0: không offset (fundamental exact)
    // mode1: 2nd partial hơi sharp so với 2× (stiffness tine)
    // mode2: bell partial hơi flat so với 6× lý thuyết
    // mode3: high partial hơi sharp (nonlinear stiffness tăng theo freq)
    constexpr float offsets[4] = { 0.0f, +0.035f, -0.040f, +0.060f };
    // Piano-string stretch: high notes hơi sharp hơn lý thuyết
    const float stretch = (midiNote - 60) * 0.0008f;
    return offsets[modeIdx] + stretch * static_cast<float>(modeIdx);
}

// Per-note decay scaling: high notes decay faster — range 0.70..1.30
// (tránh range 0.6..1.6 cũ quá rộng)
inline float decayScaleForNote(int midiNote) noexcept {
    return 0.70f + 0.60f * ((midiNote - 36) / 48.0f);
}

} // namespace rhodes_detail

// ══════════════════════════════════════════════════════════════════════════
// ModalMode — 2nd-order decaying IIR resonator
//
// Lossless resonator:  y[n] = 2·cos(ω)·y[n-1] − y[n-2]
// Với decay:           y[n] = 2·cos(ω)·r·y[n-1] − r²·y[n-2]
// r = pow(0.001, 1/(decayMs*sr)) → -60dB sau decayMs milliseconds
// ══════════════════════════════════════════════════════════════════════════
struct ModalMode {
    float y0   = 0.0f;   // y[n]
    float y1   = 0.0f;   // y[n-1]
    float coef = 0.0f;   // 2·cos(ω)
    float r    = 0.0f;   // per-sample decay factor
    float amp  = 0.0f;   // relative amplitude weight

    void reset() noexcept { y0 = y1 = 0.0f; }

    void init(float freqHz, float decayMs, float amplitude, float sr) noexcept {
        const float omega = TWO_PI * freqHz / sr;
        coef = 2.0f * std::cos(omega);
        r    = std::pow(0.001f, 1.0f / (decayMs * 0.001f * sr));
        amp  = amplitude;
        reset();
    }

    // [RT-SAFE] Update frequency without resetting resonator state (for pitch bend)
    void updateFreq(float freqHz, float sr) noexcept {
        const float omega = TWO_PI * freqHz / sr;
        coef = 2.0f * std::cos(omega);
        // r (decay) and amp are unchanged
    }

    // Inject hammer energy — khởi động resonator từ đầu
    void excite(float energy) noexcept {
        y1 = energy * amp;
        y0 = energy * amp * r;  // cos(0) = 1, bắt đầu đúng pha
    }

    // [RT-SAFE] advance one sample, return displacement
    inline float tick() noexcept {
        const float yn = coef * r * y0 - r * r * y1;
        y1 = y0;
        y0 = yn;
        return yn;
    }

    inline bool isAlive(float threshold = 1e-7f) const noexcept {
        return std::abs(y0) > threshold;
    }
};

// ══════════════════════════════════════════════════════════════════════════
// RhodesVoice — single polyphonic voice
// ══════════════════════════════════════════════════════════════════════════
struct RhodesVoice {

    bool  active     = false;
    int   note       = 60;
    int   velocity   = 100;
    float sampleRate = 44100.0f;
    float baseHz_    = 261.63f;   // base frequency at trigger (incl. micro-detune)

    // 4 modal resonators (ascending frequency)
    static constexpr int NUM_MODES = 4;
    ModalMode modes[NUM_MODES];

    // P3.1: Torsional mode — parallel bending/torsion → characteristic "wobble"
    // Offset ~6 cents from fundamental → 0.5–0.8 Hz AM beat in piano range
    ModalMode torsionMode_;

    float xPrev = 0.0f;  // previous displacement

    // Hammer burst state
    int   hammerSamplesLeft = 0;
    float hammerEnergy      = 0.0f;
    float hammerDecayCoef   = 0.0f;

    // Key click transient — P1.3: broadband noise burst at attack (hammer-tine impact)
    int   clickSamplesLeft = 0;
    float clickAmp         = 0.0f;
    float clickDecayCoef   = 1.0f;
    NoiseGenerator noise_;         // white noise for click burst

    // Damper release envelope — P3.3: 2-stage (fast initial felt contact + slow decay)
    enum class State : uint8_t { Silent, Playing, Releasing };
    State voiceState           = State::Silent;
    float releaseCoef          = 0.0f;  // Stage 2: user-set slow decay coef
    float releaseMul           = 1.0f;
    int   damperStage1Samples_ = 0;     // Stage 1 countdown (fast initial damp)
    float damperFastCoef_      = 1.0f;  // Stage 1: -6dB over 30ms

    // Equal-power stereo pan (set at trigger)
    float panL = 0.7071f;
    float panR = 0.7071f;

    // Pickup polynomial coefficients (set at trigger — P1.1: vel + tone)
    float pickupAlpha = 0.0f;  // x² — even harmonics (bell chime)
    float pickupBeta  = 0.0f;  // x³ — odd  harmonics (warmth)

    // ── API ─────────────────────────────────────────────────────────────

    void trigger(int midiNote, int vel,
                 float sampleRateHz,
                 float decayParam,
                 float toneParam,
                 float velSens,
                 float stereoSpread,
                 float microDetuneCents = 0.0f) noexcept;  // P1.2: analog detuning

    // [RT-SAFE] Update all modal resonator frequencies for real-time pitch bend.
    // Adjusts biquad coef without resetting state — safe to call mid-note.
    void updatePitchFactor(float pitchBendSemis, float sr) noexcept {
        if (!active) return;
        using namespace rhodes_detail;
        constexpr float ratios[NUM_MODES] = { 1.000f, 2.045f, 5.979f, 9.210f };
        const float factor = std::pow(2.0f, pitchBendSemis / 12.0f);
        const float bentHz = baseHz_ * factor;
        for (int m = 0; m < NUM_MODES; ++m) {
            const float offset  = inharmonicOffset(note, m);
            const float newFreq = bentHz * ratios[m] * std::pow(2.0f, offset / 12.0f);
            modes[m].updateFreq(newFreq, sr);
        }
        // Torsional mode: ~6 cents above fundamental
        torsionMode_.updateFreq(bentHz * std::pow(2.0f, 6.0f / 1200.0f), sr);
    }

    // Damper lift: 2-stage release (P3.3)
    // Stage 1: felt makes initial contact → fast -6dB in 30ms (thud of damper landing)
    // Stage 2: full felt compression → slow exponential per user releaseMs
    void release(float releaseMs = 200.0f) noexcept {
        if (voiceState == State::Playing) {
            voiceState  = State::Releasing;
            releaseMul  = 1.0f;
            // Stage 1 — 30ms fast initial damp: pow(0.5, 1/N) where N = 30ms samples
            const int stage1N        = static_cast<int>(0.030f * sampleRate);
            damperStage1Samples_     = (stage1N > 0) ? stage1N : 1;
            damperFastCoef_          = std::pow(0.5f, 1.0f / static_cast<float>(damperStage1Samples_));
            // Stage 2 — user-set slow decay
            releaseCoef = std::pow(0.001f, 1.0f / (releaseMs * 0.001f * sampleRate));
        }
    }

    void kill() noexcept {
        active                = false;
        voiceState            = State::Silent;
        for (auto& m : modes) m.reset();
        torsionMode_.reset();
        xPrev                 = 0.0f;
        releaseMul            = 1.0f;
        damperStage1Samples_  = 0;
        clickSamplesLeft      = 0;
        clickAmp              = 0.0f;
        hammerSamplesLeft     = 0;
    }

    // [RT-SAFE]
    void tick(float& outL, float& outR) noexcept;

    float getAmplitude() const noexcept { return std::abs(modes[0].y0); }

private:
    // Pickup waveshaper: displacement-based polynomial
    // Frequency-independent (không như velocity dx/dt tỉ lệ với ω)
    // α·x²: asymmetric → even harmonics → bell chime đặc trưng Rhodes
    // β·x³: symmetric  → odd  harmonics → warmth, bark
    inline float pickupModel(float x) const noexcept {
        return x + pickupAlpha * x * x + pickupBeta * x * x * x;
    }
};

// ──────────────────────────────────────────────────────────────────────────
// RhodesVoice::trigger
// ──────────────────────────────────────────────────────────────────────────
inline void RhodesVoice::trigger(int midiNote, int vel,
                                    float sampleRateHz,
                                    float decayParam,
                                    float toneParam,
                                    float velSens,
                                    float stereoSpread,
                                    float microDetuneCents) noexcept {
    using namespace rhodes_detail;

    note       = midiNote;
    velocity   = vel;
    sampleRate = sampleRateHz;
    active     = true;
    voiceState = State::Playing;
    releaseMul = 1.0f;
    xPrev      = 0.0f;

    // P1.2: micro-detune — apply per-trigger analog pitch variation
    const float baseHz = 440.0f * std::pow(2.0f, (midiNote - 69 + microDetuneCents / 100.0f) / 12.0f);
    baseHz_ = baseHz;  // store for real-time pitch bend updates

    // ── Modal frequency ratios ──────────────────────────────────────────
    // Từ spectrum analysis Rhodes Mark I (Tolonen 1998 + thực nghiệm):
    //   mode0: 1.000× fundamental
    //   mode1: 2.045× second partial (slightly inharmonic — tine stiffness)
    //   mode2: 5.979× bell partial   (characteristic chime, most audible at attack)
    //   mode3: 9.210× high partial   (attack transient, decays trong vài ms)
    constexpr float ratios[NUM_MODES] = { 1.000f, 2.045f, 5.979f, 9.210f };

    // ── Decay times ─────────────────────────────────────────────────────
    // decayParam 0..1 → fundamental 1s..15s
    // Per-note scaling: note cao decay nhanh hơn (gentler range 0.70..1.30)
    const float noteDecayScale = decayScaleForNote(midiNote);
    const float fundamentalMs  = (1000.0f + decayParam * 14000.0f) / noteDecayScale;

    // Tỉ lệ decay mỗi mode — dựa trên đo đạc thực tế:
    //   mode1 (2×): 40% — long, contributes to sustained warmth
    //   mode2 (6×): 12% — medium, bell chime "blooms" then fades
    //   mode3 (9×): 3%  — very fast, pure attack transient
    const float decayTimes[NUM_MODES] = {
        fundamentalMs,
        fundamentalMs * 0.40f,
        fundamentalMs * 0.12f,
        fundamentalMs * 0.030f
    };

    // ── Modal amplitudes (P2.2 calibrated) ──────────────────────────────
    // toneShift centred at 1.0 when tone=0.5 → ±30% adjustment either way
    // Amplitudes at mid-tone match Tolonen (1998) Rhodes Mark I measurement:
    //   {1.000, 0.280, 0.190, 0.070} ← calibrated from literature
    const float toneShift = 0.70f + toneParam * 0.60f;  // 0.70..1.30

    const float modeAmps[NUM_MODES] = {
        1.000f,
        0.280f * toneShift,   // 2nd (2.045×): -12 dB ref → warmth/body
        0.190f * toneShift,   // bell (5.979×): -15 dB ref → chime attack
        0.070f * toneShift    // high (9.210×): -23 dB ref → attack transient
    };

    // ── Init resonators ─────────────────────────────────────────────────
    for (int m = 0; m < NUM_MODES; ++m) {
        const float offset = inharmonicOffset(midiNote, m);
        const float freq   = baseHz * ratios[m] * std::pow(2.0f, offset / 12.0f);
        modes[m].init(freq, decayTimes[m], modeAmps[m], sampleRateHz);
    }

    // ── Hammer excite (P2.1: per-mode scaling) ──────────────────────────
    // Hammer contact area vs wavelength: lower modes get more excitation.
    // High partials (short wavelength) see less effective hammer coupling.
    const float velN  = velCurve(vel);
    const float velSc = (1.0f - velSens) + velSens * velN;

    constexpr float HAMMER_EXCITE[NUM_MODES] = { 1.00f, 0.85f, 0.50f, 0.25f };
    for (int m = 0; m < NUM_MODES; ++m)
        modes[m].excite(velSc * HAMMER_EXCITE[m]);

    // P3.1: torsional mode — init at fundamental + 6 cents → subtle AM wobble
    {
        const float torsionHz = baseHz * std::pow(2.0f, 6.0f / 1200.0f);
        torsionMode_.init(torsionHz, fundamentalMs * 0.80f, 0.12f, sampleRateHz);
        torsionMode_.excite(velSc * 0.20f);
    }

    // Hammer burst: inject thêm năng lượng nhỏ trong vài ms đầu
    // (giả lập hammer contact time — tạo transient "bloom")
    hammerSamplesLeft = static_cast<int>(hammerWidthSamples(vel, sampleRateHz));
    hammerEnergy      = velSc * 0.15f;  // 15% of initial energy
    hammerDecayCoef   = std::pow(0.001f, 1.0f / hammerSamplesLeft);

    // ── Pickup nonlinearity (P1.1) ──────────────────────────────────────
    // alpha/beta phụ thuộc Tone (pickup distance) VÀ Velocity (hit strength).
    // Hard hit → pickup driven harder → richer harmonics (như Rhodes thực).
    // Tinh tế: chỉ tạo harmonic coloration, không phải distortion rõ rệt.
    // alpha range 0.03..0.16,  beta = alpha * 0.5
    const float velContrib  = velN * 0.06f;                        // 0..0.06 vel contribution
    const float pickupDrive = 0.03f + toneParam * 0.07f + velContrib;  // 0.03..0.16
    pickupAlpha = pickupDrive;
    pickupBeta  = pickupDrive * 0.5f;

    // ── Key click transient (P1.3) ──────────────────────────────────────
    // Broadband noise burst 2ms — giả lập tiếng "tick" hammer-tine impact.
    // Amplitude tỉ lệ với velocity (hard hit → louder click).
    // noise_ state advances naturally across triggers — no explicit reseed needed
    clickSamplesLeft  = static_cast<int>(0.002f * sampleRateHz);  // 2 ms
    clickAmp          = velSc * 0.06f;
    clickDecayCoef    = (clickSamplesLeft > 1)
                        ? std::pow(0.001f, 1.0f / clickSamplesLeft)
                        : 0.0f;

    // ── Stereo pan ──────────────────────────────────────────────────────
    // Equal-power, note-based: thấp→trái, cao→phải (giống Rhodes stereo field)
    const float panPos = std::clamp((midiNote - 36) / 48.0f, 0.0f, 1.0f);
    const float panAng = panPos * stereoSpread * TWO_PI * 0.25f
                       + (1.0f - stereoSpread) * TWO_PI * 0.125f;
    panL = std::cos(panAng);
    panR = std::sin(panAng);
}

// ──────────────────────────────────────────────────────────────────────────
// RhodesVoice::tick — per-sample render [RT-SAFE]
// ──────────────────────────────────────────────────────────────────────────
inline void RhodesVoice::tick(float& outL, float& outR) noexcept {
    if (!active) { outL = outR = 0.0f; return; }

    // ── Hammer burst injection ──────────────────────────────────────────
    if (hammerSamplesLeft > 0) {
        hammerEnergy *= hammerDecayCoef;
        for (int m = 0; m < NUM_MODES; ++m)
            modes[m].y0 += hammerEnergy * modes[m].amp * 0.02f;
        --hammerSamplesLeft;
    }

    // ── Modal resonators → displacement sum ────────────────────────────
    float x = 0.0f;
    for (int m = 0; m < NUM_MODES; ++m)
        x += modes[m].tick();

    // P3.1: torsional AM modulation
    // Torsion mode (~6 cents sharp) modulates pickup proximity → AM effect.
    // Produces subtle "wobble" / shimmer, characteristic of tine vibration.
    x *= (1.0f + 0.08f * torsionMode_.tick());

    xPrev = x;

    // ── Pickup polynomial waveshaper ────────────────────────────────────
    float y = pickupModel(x);

    // ── Normalize voice output ─────────────────────────────────────────
    constexpr float VOICE_NORM = 0.32f;
    y *= VOICE_NORM;

    // ── Key click noise burst (P1.3) ────────────────────────────────────
    // Thêm trực tiếp vào output sau VOICE_NORM (click không qua resonator).
    // LCG noise: RT-safe, không dùng rand()/heap.
    if (clickSamplesLeft > 0) {
        y += noise_.tick() * clickAmp;
        clickAmp *= clickDecayCoef;
        --clickSamplesLeft;
    }

    // ── Release envelope (P3.3: 2-stage) ───────────────────────────────
    if (voiceState == State::Releasing) {
        if (damperStage1Samples_ > 0) {
            // Stage 1: fast initial felt contact (-6dB over 30ms)
            releaseMul *= damperFastCoef_;
            --damperStage1Samples_;
        } else {
            // Stage 2: slow continued damp (user-set releaseMs)
            releaseMul *= releaseCoef;
        }
        y *= releaseMul;
        if (releaseMul < 1e-5f) {
            kill();
            outL = outR = 0.0f;
            return;
        }
    }

    // ── Auto-silence khi resonators tắt hẳn ───────────────────────────
    if (voiceState != State::Releasing && !modes[0].isAlive(1e-7f)) {
        kill();
        outL = outR = 0.0f;
        return;
    }

    // ── Stereo output ───────────────────────────────────────────────────
    outL = y * panL;
    outR = y * panR;
}
