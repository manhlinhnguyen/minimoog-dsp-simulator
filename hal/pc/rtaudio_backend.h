// ─────────────────────────────────────────────────────────
// FILE: hal/pc/rtaudio_backend.h
// BRIEF: RtAudio audio output backend
// ─────────────────────────────────────────────────────────
#pragma once
#include "RtAudio.h"
#include "shared/interfaces.h"
#include "core/effects/effect_chain.h"
#include <string>
#include <vector>

class RtAudioBackend {
public:
    struct Config {
        unsigned int sampleRate = 44100;
        unsigned int bufferSize = 256;
        int          deviceIdx  = -1;   // -1 = default
    };

    explicit RtAudioBackend(IAudioProcessor& processor);
    ~RtAudioBackend();

    bool open (Config cfg = {}, EffectChain* effectChain = nullptr);
    bool start();
    void stop();
    void close();

    std::vector<std::string> listDevices();
    unsigned int getSampleRate() const noexcept { return cfg_.sampleRate; }
    unsigned int getBufferSize() const noexcept { return cfg_.bufferSize; }
    bool         isRunning()     const noexcept { return running_; }
    std::string  getLastError()  const noexcept { return lastError_; }

private:
    IAudioProcessor& processor_;
    EffectChain*     effectChain_ = nullptr;
    RtAudio          dac_;
    Config           cfg_;
    bool             running_   = false;
    std::string      lastError_;

    static int audioCallback(void*        outputBuffer,
                             void*        inputBuffer,
                             unsigned int nFrames,
                             double       streamTime,
                             RtAudioStreamStatus status,
                             void*        userData);

    static void errorCallback(RtAudioErrorType type,
                              const std::string& msg);
};
