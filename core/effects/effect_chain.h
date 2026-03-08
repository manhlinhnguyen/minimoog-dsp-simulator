// ─────────────────────────────────────────────────────────
// FILE: core/effects/effect_chain.h
// BRIEF: Post-synth effect chain — up to 16 serial slots
// Thread safety: atomic params for RT knob updates;
//   structural changes via mutex + try_lock (max ~5ms delay)
// ─────────────────────────────────────────────────────────
#pragma once
#include "effect_base.h"
#include <atomic>
#include <memory>
#include <mutex>

// ════════════════════════════════════════════════════════
// DATA STRUCTURES (plain POD — serializable to JSON)
// ════════════════════════════════════════════════════════

struct EffectSlotConfig {
    EffectType type    = EffectType::None;
    bool       enabled = true;
    float      params[8] = {0.f,0.f,0.f,0.f,0.f,0.f,0.f,0.f};
};

struct EffectChainConfig {
    int            numSlots = 0;
    EffectSlotConfig slots[16];
};

// ════════════════════════════════════════════════════════
// EFFECT CHAIN
// ════════════════════════════════════════════════════════

class EffectChain {
public:
    static constexpr int MAX_SLOTS  = 16;
    static constexpr int MAX_PARAMS = 8;

    void init(float sampleRate) noexcept;  // [RT-UNSAFE] — call once before audio starts

    // [RT-SAFE] — called from audio thread
    void processBlock(float* L, float* R, int n,
                      const IEffect::EffectContext& ctx) noexcept;

    // ── UI-thread API (all [RT-UNSAFE]) ─────────────────

    // Replace entire chain (add/remove/reorder effects)
    void setConfig(const EffectChainConfig& cfg);

    // Read current active config for serialization
    EffectChainConfig getConfig() const;

    // Fast knob update — bypasses mutex via atomics [RT-SAFE from UI thread]
    void setSlotParam(int slot, int param, float v) noexcept;
    void setSlotEnabled(int slot, bool enabled) noexcept;

    // Convenience helpers for the UI panel
    int  slotCount() const noexcept;
    EffectType slotType(int slot) const noexcept;
    const char* slotTypeName(int slot) const noexcept;
    bool slotEnabled(int slot) const noexcept;
    float slotParam(int slot, int param) const noexcept;
    int   slotParamCount(int slot) const noexcept;
    const char* slotParamName(int slot, int param) const noexcept;
    float slotParamMin(int slot, int param) const noexcept;
    float slotParamMax(int slot, int param) const noexcept;

private:
    float sampleRate_ = 44100.f;

    // Structural config — protected by mutex
    mutable std::mutex  configMutex_;
    EffectChainConfig   pendingConfig_;
    std::atomic<bool>   configDirty_{false};

    // Active state (owned by audio thread after applyPendingConfig)
    EffectChainConfig           activeConfig_;
    std::unique_ptr<IEffect>    effects_[MAX_SLOTS];

    // Per-param atomics for RT-safe knob updates
    std::atomic<float>  atomicParams_[MAX_SLOTS][MAX_PARAMS];
    std::atomic<bool>   atomicEnabled_[MAX_SLOTS];

    void applyPendingConfig();  // called from audio thread inside try_lock
    void syncAtomicsToEffect(int slot) noexcept;  // push atomics → effect params
};
