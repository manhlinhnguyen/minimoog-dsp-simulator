// ─────────────────────────────────────────────────────────
// FILE: hal/pc/effect_preset_storage.h
// BRIEF: JSON save/load for effect chain presets
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/effects/effect_chain.h"
#include <string>
#include <vector>

struct EffectPreset {
    std::string       name;
    std::string       category;
    EffectChainConfig chain;
};

class EffectPresetStorage {
public:
    void setDirectory(const std::string& dir) noexcept;

    bool save(const EffectPreset& preset, const std::string& filename);
    bool load(const std::string& filename, EffectPreset& out);

    std::vector<std::string> list() const;

    std::string getLastError() const { return lastError_; }

private:
    std::string dir_       = "./effect_presets";
    std::string lastError_;
};
