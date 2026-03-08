// ─────────────────────────────────────────────────────────
// FILE: hal/pc/rtaudio_backend.cpp
// BRIEF: RtAudio 6.x audio backend implementation
// ─────────────────────────────────────────────────────────
#include "rtaudio_backend.h"
#include "core/engines/engine_manager.h"
#include <cstring>
#include <iostream>

RtAudioBackend::RtAudioBackend(IAudioProcessor& processor)
    : processor_(processor)
{
    dac_.setErrorCallback(&RtAudioBackend::errorCallback);
}

RtAudioBackend::~RtAudioBackend() {
    stop();
    close();
}

bool RtAudioBackend::open(Config cfg, EffectChain* effectChain) {
    cfg_         = cfg;
    effectChain_ = effectChain;

    const std::vector<unsigned int> ids = dac_.getDeviceIds();
    if (ids.empty()) {
        lastError_ = "No audio devices found";
        return false;
    }

    RtAudio::StreamParameters params;
    params.deviceId    = (cfg_.deviceIdx < 0)
                       ? dac_.getDefaultOutputDevice()
                       : static_cast<unsigned int>(cfg_.deviceIdx);
    params.nChannels   = 2;
    params.firstChannel= 0;

    RtAudio::StreamOptions opts;
    opts.flags = RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_MINIMIZE_LATENCY;

    unsigned int bufSize = cfg_.bufferSize;

    RtAudioErrorType err = dac_.openStream(
        &params, nullptr, RTAUDIO_FLOAT32,
        cfg_.sampleRate, &bufSize,
        &RtAudioBackend::audioCallback,
        static_cast<void*>(this), &opts);

    if (err != RTAUDIO_NO_ERROR) {
        lastError_ = dac_.getErrorText();
        return false;
    }

    cfg_.bufferSize = bufSize;
    processor_.setSampleRate(static_cast<float>(cfg_.sampleRate));
    processor_.setBlockSize(static_cast<int>(cfg_.bufferSize));
    return true;
}

bool RtAudioBackend::start() {
    RtAudioErrorType err = dac_.startStream();
    if (err != RTAUDIO_NO_ERROR) {
        lastError_ = dac_.getErrorText();
        return false;
    }
    running_ = true;
    return true;
}

void RtAudioBackend::stop() {
    if (!running_) return;
    dac_.stopStream();
    running_ = false;
}

void RtAudioBackend::close() {
    if (dac_.isStreamOpen())
        dac_.closeStream();
}

std::vector<std::string> RtAudioBackend::listDevices() {
    std::vector<std::string> names;
    for (unsigned int id : dac_.getDeviceIds()) {
        RtAudio::DeviceInfo info = dac_.getDeviceInfo(id);
        if (info.outputChannels > 0)
            names.push_back(info.name);
    }
    return names;
}

// ─────────────────────────────────────────────────────────
// AUDIO CALLBACK — real-time thread, no alloc, no lock
// ─────────────────────────────────────────────────────────

int RtAudioBackend::audioCallback(void*        outputBuffer,
                                   void*        /*inputBuffer*/,
                                   unsigned int  nFrames,
                                   double        /*streamTime*/,
                                   RtAudioStreamStatus /*status*/,
                                   void*         userData) {
    auto* self = static_cast<RtAudioBackend*>(userData);
    auto* out  = static_cast<float*>(outputBuffer);

    constexpr int MAX_BUF = 1024;
    float outL[MAX_BUF];
    float outR[MAX_BUF];

    const int n = (nFrames > MAX_BUF)
                ? MAX_BUF
                : static_cast<int>(nFrames);

    self->processor_.processBlock(outL, outR, n);

    if (self->effectChain_) {
        IEffect::EffectContext fxCtx;
        if (auto* mgr = dynamic_cast<EngineManager*>(&self->processor_))
            fxCtx.bpm = mgr->getBPM();
        self->effectChain_->processBlock(outL, outR, n, fxCtx);
    }

    for (int i = 0; i < n; ++i) {
        out[i * 2 + 0] = outL[i];
        out[i * 2 + 1] = outR[i];
    }
    return 0;
}

void RtAudioBackend::errorCallback(RtAudioErrorType /*type*/,
                                    const std::string& msg) {
    std::cerr << "[RtAudio] " << msg << "\n";
}
