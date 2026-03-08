// ─────────────────────────────────────────────────────────
// FILE: core/engines/drums/drum_engine.cpp
// BRIEF: Hybrid drum machine implementation
// ─────────────────────────────────────────────────────────
#include "drum_engine.h"
#include <cmath>
#include <algorithm>

// ── DrumPadDsp ────────────────────────────────────────────

void DrumPadDsp::trigger(int velocity) noexcept {
    active  = true;
    ampEnv  = velocity / 127.0f;

    // Map pitch 0..1 to freq range per type
    switch (type) {
        case DspDrumType::Kick: {
            freq1 = 40.0f + pitch * 30.0f;    // 40–70 Hz start
            // Sweep depth: 0=no sweep (stay at freq1), 1=full sweep (freq1*4)
            const float sweepTarget = freq1 * (1.0f - sweepDepth)
                                    + 40.0f * sweepDepth;
            // Sweep time: 10ms..200ms
            const float sweepMs = 0.01f + sweepTime * 0.19f;
            freqDecayRate = (sweepTarget < freq1)
                ? std::pow(sweepTarget / freq1, 1.0f / (sweepMs * sampleRate))
                : 1.0f;
            decayRate = std::pow(0.001f,
                1.0f / ((0.05f + decay * 0.95f) * sampleRate));
            break;
        }
        case DspDrumType::Snare:
            freq1 = 150.0f + pitch * 100.0f;
            freqDecayRate = 1.0f;
            decayRate = std::pow(0.001f,
                1.0f / ((0.05f + decay * 0.25f) * sampleRate));
            break;
        case DspDrumType::HiHatC:
            freq1 = 8000.0f + pitch * 2000.0f;
            freqDecayRate = 1.0f;
            decayRate = std::pow(0.001f,
                1.0f / ((0.01f + decay * 0.08f) * sampleRate));
            break;
        case DspDrumType::HiHatO:
            freq1 = 8000.0f + pitch * 2000.0f;
            freqDecayRate = 1.0f;
            decayRate = std::pow(0.001f,
                1.0f / ((0.1f + decay * 0.6f) * sampleRate));
            break;
        case DspDrumType::Clap:
            decayRate = std::pow(0.001f,
                1.0f / ((0.05f + decay * 0.2f) * sampleRate));
            freqDecayRate = 1.0f;
            break;
        case DspDrumType::TomLow:
            freq1 = 60.0f + pitch * 40.0f;
            freqDecayRate = std::pow(freq1 * 0.3f / freq1,
                                     1.0f / (0.15f * sampleRate));
            decayRate = std::pow(0.001f,
                1.0f / ((0.1f + decay * 0.5f) * sampleRate));
            break;
        case DspDrumType::TomMid:
            freq1 = 100.0f + pitch * 60.0f;
            freqDecayRate = std::pow(0.5f, 1.0f / (0.1f * sampleRate));
            decayRate = std::pow(0.001f,
                1.0f / ((0.08f + decay * 0.4f) * sampleRate));
            break;
        case DspDrumType::Rimshot:
            freq1 = 400.0f + pitch * 200.0f;
            freqDecayRate = 1.0f;
            decayRate = std::pow(0.001f,
                1.0f / ((0.01f + decay * 0.1f) * sampleRate));
            break;
        default: break;
    }
    phase1 = 0.0f;
    phase2 = 0.0f;

    // Velocity → decay: harder hit → slightly longer decay (more energy imparted)
    // velScale: 0.8 (soft) … 1.2 (hard) → exponent < 1 lengthens, > 1 shortens
    const float velScale = 0.8f + 0.4f * (static_cast<float>(velocity) / 127.0f);
    decayRate = std::pow(decayRate, 1.0f / velScale);
}

void DrumPadDsp::tick(float& outL, float& outR) noexcept {
    if (!active) { outL = outR = 0.0f; return; }

    float sample = 0.0f;

    switch (type) {
        case DspDrumType::Kick: {
            sample = ampEnv * std::sin(phase1);
            phase1 += (TWO_PI * freq1) / sampleRate;
            freq1  *= freqDecayRate;
            if (freq1 < 30.0f) freq1 = 30.0f;
            break;
        }
        case DspDrumType::Snare: {
            const float osc   = 0.5f * std::sin(phase1);
            const float noise = 0.5f * nextNoise();
            sample = ampEnv * (osc + noise);
            phase1 += (TWO_PI * freq1) / sampleRate;
            break;
        }
        case DspDrumType::HiHatC:
        case DspDrumType::HiHatO: {
            // P3: 808-style metallic hi-hat — 6 oscillators at non-harmonic ratios
            // Ring-modulated in 3 pairs → inharmonic metallic spectrum
            static constexpr float RATIOS[6] = {
                1.0f, 1.4471f, 1.6818f, 1.9545f, 2.2727f, 2.6364f
            };
            float rm = 0.0f;
            for (int p = 0; p < 3; ++p) {
                rm += std::sin(phase1 * RATIOS[p * 2])
                    * std::sin(phase1 * RATIOS[p * 2 + 1]);
            }
            // Mix ring mod with noise for upper shimmer
            rm = rm * 0.7f + nextNoise() * 0.3f;
            sample = ampEnv * rm * 0.2f;
            phase1 += (TWO_PI * freq1) / sampleRate;
            break;
        }
        case DspDrumType::Clap: {
            // Short noise bursts with slight pre-delay envelope
            const float noise = nextNoise();
            sample = ampEnv * noise * 0.8f;
            break;
        }
        case DspDrumType::TomLow:
        case DspDrumType::TomMid: {
            sample = ampEnv * std::sin(phase1);
            phase1 += (TWO_PI * freq1) / sampleRate;
            freq1  *= freqDecayRate;
            break;
        }
        case DspDrumType::Rimshot: {
            // Short click + ring
            const float click = (ampEnv > 0.8f) ? nextNoise() * 0.5f : 0.0f;
            const float ring  = 0.5f * std::sin(phase1);
            sample = ampEnv * (click + ring);
            phase1 += (TWO_PI * freq1) / sampleRate;
            break;
        }
        default: break;
    }

    ampEnv *= decayRate;
    if (ampEnv < 0.0001f) { ampEnv = 0.0f; active = false; }

    const float panL = std::sqrt(1.0f - pan) * volume;
    const float panR = std::sqrt(pan)         * volume;
    outL = sample * panL;
    outR = sample * panR;
}

// ── DrumPadSample ─────────────────────────────────────────

void DrumPadSample::trigger(int velocity) noexcept {
    if (!loaded || audioL.empty()) return;
    playing  = true;
    readPos  = 0.0f;
    velScale = velocity / 127.0f;
    ampEnv   = 1.0f;
}

float DrumPadSample::readSampleL(int idx) const noexcept {
    if (idx < 0 || idx >= static_cast<int>(audioL.size())) return 0.0f;
    return audioL[idx];
}

float DrumPadSample::readSampleR(int idx) const noexcept {
    if (!audioR.empty() && idx < static_cast<int>(audioR.size()))
        return audioR[idx];
    return readSampleL(idx);  // mono fallback
}

void DrumPadSample::tick(float& outL, float& outR) noexcept {
    if (!playing || !loaded || audioL.empty()) {
        outL = outR = 0.0f;
        return;
    }

    const int i0 = static_cast<int>(readPos);
    const int i1 = i0 + 1;
    const float fr = readPos - static_cast<float>(i0);

    const float sL = readSampleL(i0) + fr * (readSampleL(i1) - readSampleL(i0));
    const float sR = readSampleR(i0) + fr * (readSampleR(i1) - readSampleR(i0));

    const float pL = std::sqrt(1.0f - pan) * volume * velScale;
    const float pR = std::sqrt(pan)         * volume * velScale;
    outL = sL * pL;
    outR = sR * pR;

    readPos += pitch;  // pitch multiplier
    if (readPos >= static_cast<float>(audioL.size())) {
        playing = false;
    }
}

// ── DrumEngine ────────────────────────────────────────────

DrumEngine::DrumEngine() {
    // Init default params
    for (int p = 0; p < DRUM_PADS; ++p) {
        params_[drumParamId(p, DP_VOLUME)].store(0.8f, std::memory_order_relaxed);
        params_[drumParamId(p, DP_PITCH )].store(0.5f, std::memory_order_relaxed);
        params_[drumParamId(p, DP_DECAY )].store(0.5f, std::memory_order_relaxed);
        params_[drumParamId(p, DP_PAN   )].store(0.5f, std::memory_order_relaxed);
    }
    params_[DRUM_GLOBAL_VOL_ID].store(0.8f, std::memory_order_relaxed);
    params_[DRUM_KICK_SWEEP_DEPTH_ID].store(0.5f, std::memory_order_relaxed);
    params_[DRUM_KICK_SWEEP_TIME_ID].store(0.2f, std::memory_order_relaxed);

    // Assign DSP pad types
    for (int i = 0; i < DSP_PADS; ++i)
        dspPads_[i].type = static_cast<DspDrumType>(i);
}

void DrumEngine::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    for (auto& p : dspPads_) p.setSampleRate(sampleRate);
}

void DrumEngine::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    for (auto& p : dspPads_) p.setSampleRate(sr);
}

void DrumEngine::beginBlock(int /*nFrames*/) noexcept {
    globalVol_ = params_[DRUM_GLOBAL_VOL_ID].load(std::memory_order_relaxed);
    kickSweepDepth_ = params_[DRUM_KICK_SWEEP_DEPTH_ID].load(std::memory_order_relaxed);
    kickSweepTime_  = params_[DRUM_KICK_SWEEP_TIME_ID].load(std::memory_order_relaxed);
    for (int p = 0; p < DRUM_PADS; ++p) {
        padCache_[p].vol   = params_[drumParamId(p, DP_VOLUME)].load(std::memory_order_relaxed);
        padCache_[p].pitch = params_[drumParamId(p, DP_PITCH )].load(std::memory_order_relaxed);
        padCache_[p].decay = params_[drumParamId(p, DP_DECAY )].load(std::memory_order_relaxed);
        padCache_[p].pan   = params_[drumParamId(p, DP_PAN   )].load(std::memory_order_relaxed);
    }
    // Apply cached params to pads
    for (int i = 0; i < DSP_PADS; ++i) {
        dspPads_[i].volume = padCache_[i].vol;
        dspPads_[i].pitch  = padCache_[i].pitch;
        dspPads_[i].decay  = padCache_[i].decay;
        dspPads_[i].pan    = padCache_[i].pan;
    }
    // Apply kick sweep params to pad 0 (Kick)
    dspPads_[0].sweepDepth = kickSweepDepth_;
    dspPads_[0].sweepTime  = kickSweepTime_;
    for (int i = 0; i < SAMPLE_PADS; ++i) {
        const int pi = DSP_PADS + i;
        samplePads_[i].volume = padCache_[pi].vol;
        samplePads_[i].pitch  = 0.5f + padCache_[pi].pitch; // 0..1 → 0.5x..1.5x rate
        samplePads_[i].pan    = padCache_[pi].pan;
    }
}

void DrumEngine::tickSample(float& outL, float& outR) noexcept {
    float sumL = 0.0f, sumR = 0.0f;

    for (auto& p : dspPads_) {
        if (!p.active) continue;
        float pL, pR;
        p.tick(pL, pR);
        sumL += pL; sumR += pR;
    }
    for (auto& p : samplePads_) {
        if (!p.playing) continue;
        float pL, pR;
        p.tick(pL, pR);
        sumL += pL; sumR += pR;
    }

    outL = globalVol_ * sumL;
    outR = globalVol_ * sumR;
}

void DrumEngine::noteOn(int note, int vel) noexcept {
    // DSP pads
    for (int i = 0; i < DSP_PADS; ++i)
        if (DSP_MIDI_NOTES[i] == note) { triggerPad(i, vel); return; }
    // Sample pads
    for (int i = 0; i < SAMPLE_PADS; ++i)
        if (SAMPLE_MIDI_NOTES[i] == note) { triggerPad(DSP_PADS + i, vel); return; }
}

void DrumEngine::triggerPad(int padIdx, int velocity) noexcept {
    if (padIdx < DSP_PADS) {
        // Hi-hat choke group: HiHatC (pad 2) and HiHatO (pad 3) kill each other
        if (padIdx == 2) dspPads_[3].active = false;  // close kills open
        else if (padIdx == 3) dspPads_[2].active = false;  // open kills close
        dspPads_[padIdx].trigger(velocity);
    } else {
        samplePads_[padIdx - DSP_PADS].trigger(velocity);
    }
}

void DrumEngine::allNotesOff() noexcept {
    for (auto& p : dspPads_)    p.active  = false;
    for (auto& p : samplePads_) p.playing = false;
}

void DrumEngine::allSoundOff() noexcept { allNotesOff(); }

void DrumEngine::controlChange(int cc, int val) noexcept {
    if (cc == 7) setParam(DRUM_GLOBAL_VOL_ID, val / 127.0f);
}

void DrumEngine::setParam(int id, float v) noexcept {
    if (id < 0 || id >= DRUM_PARAM_COUNT) return;
    params_[id].store(std::clamp(v, 0.0f, 1.0f), std::memory_order_relaxed);
}

float DrumEngine::getParam(int id) const noexcept {
    if (id < 0 || id >= DRUM_PARAM_COUNT) return 0.0f;
    return params_[id].load(std::memory_order_relaxed);
}

const char* DrumEngine::getParamName(int id) const noexcept {
    if (id == DRUM_GLOBAL_VOL_ID) return "Global Volume";
    if (id == DRUM_KICK_SWEEP_DEPTH_ID) return "Kick Sweep Depth";
    if (id == DRUM_KICK_SWEEP_TIME_ID)  return "Kick Sweep Time";
    if (id < 0 || id >= DRUM_PARAM_COUNT) return "";
    const int type = id % DRUM_PAD_PARAMS;
    static const char* typeNames[4] = {"Volume","Pitch","Decay","Pan"};
    // We just return the type name since pad names are shown by UI
    return typeNames[type];
}

float DrumEngine::getParamMin(int /*id*/) const noexcept { return 0.0f; }
float DrumEngine::getParamMax(int /*id*/) const noexcept { return 1.0f; }
float DrumEngine::getParamDefault(int id) const noexcept {
    if (id == DRUM_GLOBAL_VOL_ID) return 0.8f;
    if (id == DRUM_KICK_SWEEP_DEPTH_ID) return 0.5f;
    if (id == DRUM_KICK_SWEEP_TIME_ID)  return 0.2f;
    const int type = id % DRUM_PAD_PARAMS;
    static const float defs[4] = {0.8f, 0.5f, 0.5f, 0.5f};
    return defs[type];
}

int DrumEngine::getActiveVoices() const noexcept {
    int n = 0;
    for (const auto& p : dspPads_)    n += p.active  ? 1 : 0;
    for (const auto& p : samplePads_) n += p.playing ? 1 : 0;
    return n;
}

void DrumEngine::loadSamplePad(int padIdx,
                                std::vector<float> audioL,
                                std::vector<float> audioR,
                                float srHz,
                                const std::string& name) noexcept {
    if (padIdx < 0 || padIdx >= SAMPLE_PADS) return;
    auto& pad       = samplePads_[padIdx];
    pad.audioL      = std::move(audioL);
    pad.audioR      = std::move(audioR);
    pad.sampleRate  = srHz;
    pad.name        = name;
    pad.loaded      = true;
}

const std::string& DrumEngine::getSampleName(int idx) const noexcept {
    static const std::string empty;
    if (idx < 0 || idx >= SAMPLE_PADS) return empty;
    return samplePads_[idx].name;
}
