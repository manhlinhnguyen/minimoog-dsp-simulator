// ─────────────────────────────────────────────────────────
// FILE: core/effects/effect_base.h
// BRIEF: Effect interface and factory for the post-synth effect chain
// ─────────────────────────────────────────────────────────
#pragma once
#include <memory>

// ════════════════════════════════════════════════════════
// EFFECT TYPE ENUM
// ════════════════════════════════════════════════════════

enum class EffectType : int {
    None     = -1,
    Gain     = 0,
    Chorus   = 1,
    Flanger  = 2,
    Phaser   = 3,
    Tremolo  = 4,
    Delay    = 5,
    Reverb     = 6,
    Equalizer  = 7,
    COUNT      = 8
};

// ════════════════════════════════════════════════════════
// IEFFECT — pure interface, zero platform dependency
// All methods callable from the audio thread must be RT-SAFE.
// ════════════════════════════════════════════════════════

struct IEffect {
    virtual ~IEffect() = default;

    virtual void  init(float sampleRate)                  noexcept = 0;
    virtual void  reset()                                 noexcept = 0;
    virtual void  setParam(int id, float v)               noexcept = 0;  // [RT-SAFE]
    virtual float getParam(int id)              const     noexcept = 0;
    virtual int   paramCount()                  const     noexcept = 0;
    virtual const char* paramName(int id)       const     noexcept = 0;
    virtual float paramMin(int id)              const     noexcept = 0;
    virtual float paramMax(int id)              const     noexcept = 0;
    virtual float paramDefault(int id)          const     noexcept = 0;

    // [RT-SAFE] — no alloc, no lock, no throw
    // EffectContext is immutable for one block and provided by the host.
    struct EffectContext {
        float bpm = 120.0f;
    };
    virtual void  processBlock(float* L, float* R, int n,
                               const EffectContext& ctx) noexcept = 0;

    virtual EffectType  type()     const noexcept = 0;
    virtual const char* typeName() const noexcept = 0;
};

// Factory — [RT-UNSAFE] (allocates)
std::unique_ptr<IEffect> createEffect(EffectType t, float sampleRate);

// Helper: type name string
inline const char* effectTypeName(EffectType t) {
    switch (t) {
        case EffectType::Gain:    return "Gain";
        case EffectType::Chorus:  return "Chorus";
        case EffectType::Flanger: return "Flanger";
        case EffectType::Phaser:  return "Phaser";
        case EffectType::Tremolo: return "Tremolo";
        case EffectType::Delay:   return "Delay";
        case EffectType::Reverb:     return "Reverb";
        case EffectType::Equalizer:  return "Equalizer";
        default:                     return "None";
    }
}
