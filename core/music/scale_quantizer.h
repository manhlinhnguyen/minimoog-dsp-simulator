// ─────────────────────────────────────────────────────────
// FILE: core/music/scale_quantizer.h
// BRIEF: Scale quantizer — snaps notes to nearest scale degree
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/types.h"

struct ScaleDef {
    const char* name;
    bool        degrees[12];  // true = note belongs to scale
};

class ScaleQuantizer {
public:
    static constexpr int SCALE_COUNT = 16;
    static const ScaleDef SCALES[SCALE_COUNT];

    void setEnabled(bool on)  noexcept;
    void setRoot   (int semi) noexcept;  // 0=C .. 11=B
    void setScale  (int idx)  noexcept;  // 0..15

    // Returns closest in-scale MIDI note
    int quantize(int midiNote) const noexcept;

    bool isEnabled()   const noexcept { return enabled_; }
    int  getRoot()     const noexcept { return root_; }
    int  getScaleIdx() const noexcept { return scaleIdx_; }

private:
    bool enabled_  = false;
    int  root_     = 0;
    int  scaleIdx_ = 1;   // major default
};
