// ─────────────────────────────────────────────────────────
// FILE: core/effects/effect_chain.cpp
// BRIEF: Effect chain implementation
// ─────────────────────────────────────────────────────────
#include "effect_chain.h"
#include "gain_effect.h"
#include "chorus_effect.h"
#include "flanger_effect.h"
#include "phaser_effect.h"
#include "tremolo_effect.h"
#include "delay_effect.h"
#include "reverb_effect.h"
#include "eq_effect.h"
#include <algorithm>

// ─────────────────────────────────────────────────────────
// FACTORY
// ─────────────────────────────────────────────────────────

std::unique_ptr<IEffect> createEffect(EffectType t, float sampleRate) {
    std::unique_ptr<IEffect> e;
    switch (t) {
        case EffectType::Gain:    e = std::make_unique<GainEffect>();    break;
        case EffectType::Chorus:  e = std::make_unique<ChorusEffect>();  break;
        case EffectType::Flanger: e = std::make_unique<FlangerEffect>(); break;
        case EffectType::Phaser:  e = std::make_unique<PhaserEffect>();  break;
        case EffectType::Tremolo: e = std::make_unique<TremoloEffect>(); break;
        case EffectType::Delay:   e = std::make_unique<DelayEffect>();   break;
        case EffectType::Reverb:     e = std::make_unique<ReverbEffect>();  break;
        case EffectType::Equalizer:  e = std::make_unique<EqEffect>();      break;
        default: return nullptr;
    }
    e->init(sampleRate);
    return e;
}

// ─────────────────────────────────────────────────────────
// INIT
// ─────────────────────────────────────────────────────────

void EffectChain::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    for (int s = 0; s < MAX_SLOTS; ++s) {
        for (int p = 0; p < MAX_PARAMS; ++p)
            atomicParams_[s][p].store(0.f, std::memory_order_relaxed);
        atomicEnabled_[s].store(true, std::memory_order_relaxed);
    }
    activeConfig_ = EffectChainConfig{};
    configDirty_.store(false, std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────
// UI-THREAD API
// ─────────────────────────────────────────────────────────

void EffectChain::setConfig(const EffectChainConfig& cfg) {
    // Build new effects (alloc OK — we're on UI thread)
    std::unique_ptr<IEffect> newEffects[MAX_SLOTS];
    const int n = std::clamp(cfg.numSlots, 0, MAX_SLOTS);
    for (int s = 0; s < n; ++s) {
        if (cfg.slots[s].type != EffectType::None) {
            newEffects[s] = createEffect(cfg.slots[s].type, sampleRate_);
            if (newEffects[s]) {
                const int pc = newEffects[s]->paramCount();
                for (int p = 0; p < pc && p < MAX_PARAMS; ++p) {
                    newEffects[s]->setParam(p, cfg.slots[s].params[p]);
                    atomicParams_[s][p].store(cfg.slots[s].params[p],
                                              std::memory_order_relaxed);
                }
            }
        }
        atomicEnabled_[s].store(cfg.slots[s].enabled, std::memory_order_relaxed);
    }

    std::lock_guard<std::mutex> lk(configMutex_);
    pendingConfig_ = cfg;
    pendingConfig_.numSlots = n;
    // Move new effects into pending slots via activeConfig_ swap
    // We'll swap them in applyPendingConfig (audio thread)
    // But we can't safely move unique_ptrs cross-thread — instead we store them
    // temporarily. The audio thread applies the config and recreates from scratch.
    // (The effect objects allocated here are discarded; audio thread re-creates.)
    configDirty_.store(true, std::memory_order_release);
}

EffectChainConfig EffectChain::getConfig() const {
    std::lock_guard<std::mutex> lk(configMutex_);
    // Return activeConfig_ snapshotted with current atomic param values
    EffectChainConfig cfg = activeConfig_;
    for (int s = 0; s < cfg.numSlots; ++s) {
        cfg.slots[s].enabled = atomicEnabled_[s].load(std::memory_order_relaxed);
        if (effects_[s]) {
            const int pc = effects_[s]->paramCount();
            for (int p = 0; p < pc && p < MAX_PARAMS; ++p)
                cfg.slots[s].params[p] =
                    atomicParams_[s][p].load(std::memory_order_relaxed);
        }
    }
    return cfg;
}

void EffectChain::setSlotParam(int slot, int param, float v) noexcept {
    if (slot < 0 || slot >= MAX_SLOTS) return;
    if (param < 0 || param >= MAX_PARAMS) return;
    if (effects_[slot]) {
        effects_[slot]->setParam(param, v);
        // Store the clamped value the effect accepted, not the raw input,
        // so serialized presets always contain valid values.
        atomicParams_[slot][param].store(
            effects_[slot]->getParam(param), std::memory_order_relaxed);
    } else {
        atomicParams_[slot][param].store(v, std::memory_order_relaxed);
    }
}

void EffectChain::setSlotEnabled(int slot, bool enabled) noexcept {
    if (slot < 0 || slot >= MAX_SLOTS) return;
    atomicEnabled_[slot].store(enabled, std::memory_order_relaxed);
}

int EffectChain::slotCount() const noexcept {
    return activeConfig_.numSlots;
}

EffectType EffectChain::slotType(int slot) const noexcept {
    if (slot < 0 || slot >= activeConfig_.numSlots) return EffectType::None;
    return activeConfig_.slots[slot].type;
}

const char* EffectChain::slotTypeName(int slot) const noexcept {
    return effectTypeName(slotType(slot));
}

bool EffectChain::slotEnabled(int slot) const noexcept {
    if (slot < 0 || slot >= MAX_SLOTS) return false;
    return atomicEnabled_[slot].load(std::memory_order_relaxed);
}

float EffectChain::slotParam(int slot, int param) const noexcept {
    if (slot < 0 || slot >= MAX_SLOTS) return 0.f;
    if (param < 0 || param >= MAX_PARAMS) return 0.f;
    return atomicParams_[slot][param].load(std::memory_order_relaxed);
}

int EffectChain::slotParamCount(int slot) const noexcept {
    if (slot < 0 || slot >= activeConfig_.numSlots) return 0;
    if (!effects_[slot]) return 0;
    return effects_[slot]->paramCount();
}

const char* EffectChain::slotParamName(int slot, int param) const noexcept {
    if (slot < 0 || slot >= activeConfig_.numSlots) return "";
    if (!effects_[slot]) return "";
    return effects_[slot]->paramName(param);
}

float EffectChain::slotParamMin(int slot, int param) const noexcept {
    if (slot < 0 || slot >= activeConfig_.numSlots || !effects_[slot]) return 0.f;
    return effects_[slot]->paramMin(param);
}

float EffectChain::slotParamMax(int slot, int param) const noexcept {
    if (slot < 0 || slot >= activeConfig_.numSlots || !effects_[slot]) return 1.f;
    return effects_[slot]->paramMax(param);
}

// ─────────────────────────────────────────────────────────
// AUDIO-THREAD PRIVATE
// ─────────────────────────────────────────────────────────

void EffectChain::applyPendingConfig() {
    // Re-create effects for each slot from pending config
    const EffectChainConfig& cfg = pendingConfig_;
    const int n = cfg.numSlots;

    // Destroy old effects beyond new count
    for (int s = n; s < MAX_SLOTS; ++s)
        effects_[s].reset();

    for (int s = 0; s < n; ++s) {
        const EffectSlotConfig& slot = cfg.slots[s];
        // Only recreate if type changed
        if (!effects_[s] || effects_[s]->type() != slot.type) {
            effects_[s] = createEffect(slot.type, sampleRate_);
        }
        if (effects_[s]) {
            const int pc = effects_[s]->paramCount();
            for (int p = 0; p < pc && p < MAX_PARAMS; ++p) {
                const float v = atomicParams_[s][p].load(std::memory_order_relaxed);
                effects_[s]->setParam(p, v);
            }
        }
        atomicEnabled_[s].store(slot.enabled, std::memory_order_relaxed);
    }

    activeConfig_ = cfg;
    activeConfig_.numSlots = n;
}

void EffectChain::syncAtomicsToEffect(int slot) noexcept {
    if (!effects_[slot]) return;
    const int pc = effects_[slot]->paramCount();
    for (int p = 0; p < pc && p < MAX_PARAMS; ++p)
        effects_[slot]->setParam(p,
            atomicParams_[slot][p].load(std::memory_order_relaxed));
}

// [RT-SAFE]
void EffectChain::processBlock(float* L, float* R, int n,
                               const IEffect::EffectContext& ctx) noexcept {
    // Try to apply pending structural config (skip if UI thread holds mutex)
    if (configDirty_.load(std::memory_order_acquire)) {
        if (configMutex_.try_lock()) {
            applyPendingConfig();
            configDirty_.store(false, std::memory_order_release);
            configMutex_.unlock();
        }
        // If try_lock failed, we'll retry next block (max ~5ms delay)
    }

    const int numSlots = activeConfig_.numSlots;
    for (int s = 0; s < numSlots; ++s) {
        if (!effects_[s]) continue;
        if (!atomicEnabled_[s].load(std::memory_order_relaxed)) continue;
        effects_[s]->processBlock(L, R, n, ctx);
    }
}
