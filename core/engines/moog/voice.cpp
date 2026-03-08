// ─────────────────────────────────────────────────────────
// FILE: core/engines/moog/voice.cpp
// BRIEF: Voice implementation — full synthesis path per note
// ─────────────────────────────────────────────────────────
#include "voice.h"
#include "core/util/math_utils.h"
#include <cmath>

void Voice::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    for (auto& o : osc_) o.setSampleRate(sampleRate);
    filter_.setSampleRate(sampleRate);
    filterEnv_.setSampleRate(sampleRate);
    ampEnv_.setSampleRate(sampleRate);
    glide_.setSampleRate(sampleRate);
    cutoffSmoother_.init(sampleRate, 5.0f);
    resSmoother_.init(sampleRate, 5.0f);
    for (auto& s : mixSmoother_) s.init(sampleRate, 5.0f);
}

void Voice::noteOn(int midiNote, int velocity,
                   float unisonDetuneCents) noexcept {
    note_        = midiNote;
    velocity_    = velocity;
    velAmp_      = velocity / 127.0f;
    detuneCents_ = unisonDetuneCents;
    age_         = 0;

    const hz_t hz = noteToHz(midiNote, detuneCents_ / 100.0f);
    glide_.setTarget(hz);

    filterEnv_.noteOn();
    ampEnv_.noteOn();
}

void Voice::noteOff() noexcept {
    filterEnv_.noteOff();
    ampEnv_.noteOff();
}

void Voice::forceOff() noexcept {
    filterEnv_.reset();
    ampEnv_.reset();
}

void Voice::reset() noexcept {
    forceOff();
    for (auto& o : osc_) o.reset();
    filter_.reset();
    glide_.jumpTo(440.0f);
    lastLevel_ = 0.0f;
    age_       = 0;
}

bool Voice::isActive() const noexcept {
    return ampEnv_.isActive();
}

bool Voice::isReleasing() const noexcept {
    return ampEnv_.getStage() == ControlEnvelope::Stage::Release;
}

hz_t Voice::noteToHz(int midiNote,
                     float semitoneOffset) const noexcept {
    return A4_HZ * std::pow(2.0f,
           (midiNote - A4_MIDI + semitoneOffset) / 12.0f);
}

float Voice::kbdTrackOffset(const float p[]) const noexcept {
    // Keyboard tracking: cutoff follows note pitch
    // 0 = no tracking, 1 = 1/3 octave/octave, 2 = 2/3
    const int   mode       = static_cast<int>(p[MP_FILTER_KBD_TRACK]);
    constexpr float factors[3] = { 0.0f, 1.0f / 3.0f, 2.0f / 3.0f };
    const float factor     = factors[clamp(mode, 0, 2)];
    // Semitones relative to C4 (MIDI 60)
    return (note_ - 60) * factor;
}

void Voice::applyOscParams(const float p[]) noexcept {
    for (int i = 0; i < 3; ++i) {
        // Params are laid out in groups of 4: ON, RANGE, FREQ, WAVE
        osc_[i].setWaveShape(static_cast<WaveShape>(
            static_cast<int>(p[MP_OSC1_WAVE + i * 4])));
        osc_[i].setRange(static_cast<OscRange>(
            static_cast<int>(p[MP_OSC1_RANGE + i * 4])));
    }
    noise_.setColor(
        p[MP_NOISE_COLOR] > 0.5f
        ? NoiseGenerator::Color::Pink
        : NoiseGenerator::Color::White);
}

void Voice::applyFilterParams(const float p[]) noexcept {
    // Cutoff and resonance handled via smoothers in tick()
    (void)p;
}

void Voice::applyEnvParams(const float p[]) noexcept {
    // Filter envelope
    filterEnv_.setAttack (normToEnvMs(p[MP_FENV_ATTACK]));
    filterEnv_.setDecay  (normToEnvMs(p[MP_FENV_DECAY]));
    filterEnv_.setSustain(p[MP_FENV_SUSTAIN]);
    filterEnv_.setRelease(normToEnvMs(p[MP_FENV_RELEASE]));

    // Amp envelope
    ampEnv_.setAttack (normToEnvMs(p[MP_AENV_ATTACK]));
    ampEnv_.setDecay  (normToEnvMs(p[MP_AENV_DECAY]));
    ampEnv_.setSustain(p[MP_AENV_SUSTAIN]);
    ampEnv_.setRelease(normToEnvMs(p[MP_AENV_RELEASE]));
}

sample_t Voice::tick(const float p[]) noexcept {  // [RT-SAFE]
    // ── 1. Update envelope params from snapshot ───────────
    applyEnvParams(p);

    // ── 2. Advance envelopes ──────────────────────────────
    const float filterCV = filterEnv_.tick();
    const float ampCV    = ampEnv_.tick();

    if (!isActive()) { lastLevel_ = 0.0f; return 0.0f; }

    // ── 3. Glide ──────────────────────────────────────────
    glide_.setEnabled(p[MP_GLIDE_ON] > 0.5f);
    glide_.setGlideTime(normToGlideMs(p[MP_GLIDE_TIME]));
    const hz_t baseHz = glide_.tick();

    // ── 4. Apply OSC params ───────────────────────────────
    applyOscParams(p);

    const float tune = p[MP_MASTER_TUNE];  // ±1 semitone
    osc_[0].setFrequency(baseHz
        * semitonesToRatio(tune + normToSemitones(p[MP_OSC1_FREQ])));
    osc_[1].setFrequency(baseHz
        * semitonesToRatio(tune + normToSemitones(p[MP_OSC2_FREQ])));
    osc_[2].setFrequency(baseHz
        * semitonesToRatio(tune + normToSemitones(p[MP_OSC3_FREQ])));

    // ── 5. OSC3 LFO mode ──────────────────────────────────
    const bool osc3IsLFO = p[MP_OSC3_LFO_ON] > 0.5f;

    // ── 6. Render OSCs ────────────────────────────────────
    const sample_t s1 = osc_[0].tick();
    const sample_t s2 = osc_[1].tick();
    const sample_t s3 = osc3IsLFO ? 0.0f : osc_[2].tick();
    const sample_t sn = noise_.tick();

    // ── 7. Mixer ─────────────────────────────────────────
    mixSmoother_[0].setTarget(p[MP_MIX_OSC1]
                              * (p[MP_OSC1_ON] > 0.5f ? 1.f : 0.f));
    mixSmoother_[1].setTarget(p[MP_MIX_OSC2]
                              * (p[MP_OSC2_ON] > 0.5f ? 1.f : 0.f));
    mixSmoother_[2].setTarget(p[MP_MIX_OSC3]
                              * (p[MP_OSC3_ON] > 0.5f && !osc3IsLFO ? 1.f : 0.f));
    mixSmoother_[3].setTarget(p[MP_MIX_NOISE]);

    const float m1 = mixSmoother_[0].tick();
    const float m2 = mixSmoother_[1].tick();
    const float m3 = mixSmoother_[2].tick();
    const float mn = mixSmoother_[3].tick();

    sample_t mixed = s1 * m1 + s2 * m2 + s3 * m3 + sn * mn;
    // Soft-clip the mixed signal
    mixed = fast_tanh(mixed * 0.5f) * 2.0f;

    // ── 8. Modulation ─────────────────────────────────────
    // LFO source: OSC3 in LFO mode
    float lfoOut = 0.0f;
    if (osc3IsLFO) lfoOut = osc_[2].tick();

    const float modAmt = p[MP_MOD_MIX];

    if (p[MP_OSC_MOD_ON] > 0.5f) {
        const float pitchMod = lfoOut * modAmt * 2.0f;  // ±2 semitones
        osc_[0].setFrequency(baseHz * semitonesToRatio(pitchMod));
    }

    float filterModSemi = 0.0f;
    if (p[MP_FILTER_MOD_ON] > 0.5f)
        filterModSemi = lfoOut * modAmt * 24.0f;  // ±24 semitones

    // ── 9. Filter ─────────────────────────────────────────
    const hz_t baseCutHz  = normToCutoffHz(p[MP_FILTER_CUTOFF]);
    const float envModSemi = p[MP_FILTER_AMOUNT] * filterCV * 60.0f;
    const float kbdSemi    = kbdTrackOffset(p);
    const float totalSemi  = envModSemi + kbdSemi + filterModSemi;
    const hz_t finalCutHz  = baseCutHz * semitonesToRatio(totalSemi);

    cutoffSmoother_.setTarget(clamp(finalCutHz, 20.0f, 20000.0f));
    resSmoother_.setTarget(p[MP_FILTER_EMPHASIS]);

    filter_.setCutoff(cutoffSmoother_.tick());
    filter_.setResonance(resSmoother_.tick());

    const sample_t filtered = filter_.process(mixed);

    // ── 10. VCA ───────────────────────────────────────────
    const float amp    = ampCV * velAmp_ * p[MP_MASTER_VOL];
    const sample_t out = filtered * amp;

    lastLevel_ = std::abs(out);
    ++age_;
    return out;
}
