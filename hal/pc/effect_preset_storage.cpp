// ─────────────────────────────────────────────────────────
// FILE: hal/pc/effect_preset_storage.cpp
// BRIEF: JSON save/load for effect chain presets
// ─────────────────────────────────────────────────────────
#include "effect_preset_storage.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using json   = nlohmann::json;

void EffectPresetStorage::setDirectory(const std::string& dir) noexcept {
    dir_ = dir;
    fs::create_directories(dir_);
}

bool EffectPresetStorage::save(const EffectPreset& preset,
                                const std::string& filename) {
    json j;
    j["name"]     = preset.name;
    j["category"] = preset.category;

    const EffectChainConfig& cfg = preset.chain;
    j["numSlots"] = cfg.numSlots;

    json slots = json::array();
    for (int s = 0; s < cfg.numSlots; ++s) {
        const EffectSlotConfig& slot = cfg.slots[s];
        json sj;
        sj["type"]    = static_cast<int>(slot.type);
        sj["enabled"] = slot.enabled;
        json params = json::array();
        for (int p = 0; p < 8; ++p)
            params.push_back(slot.params[p]);
        sj["params"] = params;
        slots.push_back(sj);
    }
    j["slots"] = slots;

    const std::string path = dir_ + "/" + filename;
    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        lastError_ = "Cannot write: " + path;
        return false;
    }
    ofs << j.dump(2);
    lastError_.clear();
    return true;
}

bool EffectPresetStorage::load(const std::string& filename, EffectPreset& out) {
    const std::string path = dir_ + "/" + filename;
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        lastError_ = "Cannot open: " + path;
        return false;
    }

    json j;
    try {
        ifs >> j;
    } catch (json::exception& e) {
        lastError_ = std::string("JSON error: ") + e.what();
        return false;
    }

    out.name     = j.value("name",     filename);
    out.category = j.value("category", "User");
    out.chain    = EffectChainConfig{};
    out.chain.numSlots = j.value("numSlots", 0);
    if (out.chain.numSlots > 16) out.chain.numSlots = 16;

    if (j.contains("slots") && j["slots"].is_array()) {
        int idx = 0;
        for (const auto& sj : j["slots"]) {
            if (idx >= out.chain.numSlots) break;
            EffectSlotConfig& slot = out.chain.slots[idx];
            slot.type    = static_cast<EffectType>(sj.value("type",    -1));
            slot.enabled = sj.value("enabled", true);
            if (sj.contains("params") && sj["params"].is_array()) {
                int pi = 0;
                for (const auto& pv : sj["params"]) {
                    if (pi >= 8) break;
                    slot.params[pi++] = pv.get<float>();
                }
            }
            ++idx;
        }
    }

    lastError_.clear();
    return true;
}

std::vector<std::string> EffectPresetStorage::list() const {
    std::vector<std::string> result;
    if (!fs::exists(dir_)) return result;
    for (const auto& entry : fs::directory_iterator(dir_)) {
        if (entry.path().extension() == ".json")
            result.push_back(entry.path().filename().string());
    }
    std::sort(result.begin(), result.end());
    return result;
}
