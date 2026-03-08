// ─────────────────────────────────────────────────────────
// FILE: hal/pc/global_preset_storage.h
// BRIEF: JSON save/load for global presets (engine + effect chain)
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/engine_manager.h"
#include "core/effects/effect_chain.h"
#include <map>
#include <string>
#include <vector>

// ════════════════════════════════════════════════════════
// GlobalPreset — captures one engine + one effect chain.
//
// engine_params always uses integer string keys ("0", "1", ...)
//
// JSON on disk:
//   {
//     "name": "...", "category": "...", "engine": "...",
//     "engine_params": { "0": 0.5, "1": 0.8, ... },
//     "effects": { "numSlots": N, "slots": [...] }
//   }
// ════════════════════════════════════════════════════════

struct GlobalPreset {
    std::string name;
    std::string category;
    std::string engineName;
    std::map<std::string, float> engineParams;  // always integer string keys
    EffectChainConfig effects;
};

class GlobalPresetStorage {
public:
    void setDirectory(const std::string& dir) noexcept;

    std::vector<std::string> list() const;

    bool load(const std::string& filename, GlobalPreset& out) const;
    bool save(const std::string& filename, const GlobalPreset& preset) const;

    // Capture current state into a GlobalPreset struct
    GlobalPreset capture(const std::string& name,
                         const std::string& category,
                         EngineManager&     mgr,
                         const EffectChain& chain) const;

    // Apply a GlobalPreset: switch engine, load params, apply effects
    void apply(const GlobalPreset& preset,
               EngineManager&      mgr,
               EffectChain&        chain) const noexcept;

    std::string getLastError() const noexcept { return lastError_; }

private:
    std::string         dir_;
    mutable std::string lastError_;
};
