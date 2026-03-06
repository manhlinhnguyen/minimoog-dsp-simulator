// ─────────────────────────────────────────────────────────
// FILE: core/dsp/glide.cpp
// BRIEF: Portamento / glide implementation
// ─────────────────────────────────────────────────────────
#include "glide.h"

void GlideProcessor::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    updateCoeff();
}

void GlideProcessor::setGlideTime(ms_t ms) noexcept {
    glideMs_ = ms < 0.0f ? 0.0f : ms;
    updateCoeff();
}

void GlideProcessor::setEnabled(bool on) noexcept {
    enabled_ = on;
}

void GlideProcessor::updateCoeff() noexcept {
    if (glideMs_ < 0.1f) {
        coeff_ = 0.0f;  // instant
        return;
    }
    coeff_ = std::exp(-1.0f / (glideMs_ * 0.001f * sampleRate_));
}

void GlideProcessor::setTarget(hz_t hz) noexcept {
    targetHz_  = hz;
    logTarget_ = hzToLog(hz);
    if (!enabled_ || glideMs_ < 0.1f)
        jumpTo(hz);
}

void GlideProcessor::jumpTo(hz_t hz) noexcept {
    currentHz_  = hz;
    targetHz_   = hz;
    logCurrent_ = hzToLog(hz);
    logTarget_  = logCurrent_;
}

bool GlideProcessor::isGliding() const noexcept {
    return std::abs(logCurrent_ - logTarget_) > 0.0001f;
}

hz_t GlideProcessor::tick() noexcept {  // [RT-SAFE]
    if (!enabled_ || coeff_ == 0.0f) {
        currentHz_  = targetHz_;
        logCurrent_ = logTarget_;
        return currentHz_;
    }

    // Exponential approach in log domain
    logCurrent_ = logTarget_ + (logCurrent_ - logTarget_) * coeff_;

    // Snap when close enough (avoids asymptotic tailing)
    if (std::abs(logCurrent_ - logTarget_) < 0.00001f)
        logCurrent_ = logTarget_;

    currentHz_ = logToHz(logCurrent_);
    return currentHz_;
}
