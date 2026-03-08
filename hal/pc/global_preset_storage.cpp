// ─────────────────────────────────────────────────────────
// FILE: hal/pc/global_preset_storage.cpp
// BRIEF: GlobalPresetStorage implementation
// ─────────────────────────────────────────────────────────
#include "global_preset_storage.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;
namespace fs = std::filesystem;

void GlobalPresetStorage::setDirectory(const std::string& dir) noexcept {
    dir_ = dir;
}

std::vector<std::string> GlobalPresetStorage::list() const {
    std::vector<std::string> names;
    if (dir_.empty()) return names;
    try {
        for (const auto& entry : fs::directory_iterator(dir_)) {
            if (entry.path().extension() == ".json")
                names.push_back(entry.path().filename().string());
        }
        std::sort(names.begin(), names.end());
    } catch (...) {}
    return names;
}

// ── Helpers: EffectChainConfig ↔ JSON ────────────────────

static json effectsToJson(const EffectChainConfig& cfg) {
    json j;
    j["numSlots"] = cfg.numSlots;
    json slots = json::array();
    for (int s = 0; s < cfg.numSlots; ++s) {
        const EffectSlotConfig& slot = cfg.slots[s];
        json sj;
        sj["type"]    = static_cast<int>(slot.type);
        sj["enabled"] = slot.enabled;
        json params = json::array();
        for (int p = 0; p < 8; ++p) params.push_back(slot.params[p]);
        sj["params"] = params;
        slots.push_back(sj);
    }
    j["slots"] = slots;
    return j;
}

static EffectChainConfig effectsFromJson(const json& j) {
    EffectChainConfig cfg{};
    cfg.numSlots = std::min(j.value("numSlots", 0), 16);
    if (!j.contains("slots") || !j["slots"].is_array()) return cfg;
    int idx = 0;
    for (const auto& sj : j["slots"]) {
        if (idx >= cfg.numSlots) break;
        EffectSlotConfig& slot = cfg.slots[idx];
        slot.type    = static_cast<EffectType>(sj.value("type", -1));
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
    return cfg;
}

// ── load / save ───────────────────────────────────────────

bool GlobalPresetStorage::load(const std::string& filename,
                                GlobalPreset& out) const {
    const std::string path = dir_ + "/" + filename;
    std::ifstream f(path);
    if (!f.is_open()) {
        lastError_ = "Cannot open: " + path;
        return false;
    }
    try {
        json j;
        f >> j;
        out.name       = j.value("name",     filename);
        out.category   = j.value("category", "User");
        out.engineName = j.value("engine",   "");
        out.engineParams.clear();

        if (j.contains("engine_params") && j["engine_params"].is_object())
            for (auto it = j["engine_params"].begin();
                      it != j["engine_params"].end(); ++it)
                out.engineParams[it.key()] = it.value().get<float>();

        if (j.contains("effects") && j["effects"].is_object())
            out.effects = effectsFromJson(j["effects"]);
        else
            out.effects = EffectChainConfig{};

        lastError_.clear();
        return true;
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return false;
    }
}

bool GlobalPresetStorage::save(const std::string& filename,
                                const GlobalPreset& preset) const {
    try { fs::create_directories(dir_); } catch (...) {}

    const std::string path = dir_ + "/" + filename;
    try {
        json j;
        j["name"]     = preset.name;
        j["category"] = preset.category;
        j["engine"]   = preset.engineName;

        json ep = json::object();
        for (const auto& [k, v] : preset.engineParams)
            ep[k] = v;
        j["engine_params"] = ep;
        j["effects"]       = effectsToJson(preset.effects);

        std::ofstream f(path);
        if (!f.is_open()) {
            lastError_ = "Cannot write: " + path;
            return false;
        }
        f << j.dump(2);
        lastError_.clear();
        return true;
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return false;
    }
}

// ── capture ───────────────────────────────────────────────

GlobalPreset GlobalPresetStorage::capture(const std::string& name,
                                           const std::string& category,
                                           EngineManager&     mgr,
                                           const EffectChain& chain) const {
    GlobalPreset p;
    p.name     = name;
    p.category = category;
    p.effects  = chain.getConfig();

    const IEngine* active = mgr.getActiveEngine();
    p.engineName = active ? active->getName() : "";

    if (active) {
        for (int i = 0; i < active->getParamCount(); ++i)
            p.engineParams[std::to_string(i)] = active->getParam(i);
    }
    return p;
}

// ── apply ─────────────────────────────────────────────────

void GlobalPresetStorage::apply(const GlobalPreset& preset,
                                 EngineManager&      mgr,
                                 EffectChain&        chain) const noexcept {
    // 1. Switch to matching engine
    int targetIdx = -1;
    for (int i = 0; i < mgr.getEngineCount(); ++i) {
        const IEngine* e = mgr.getEngine(i);
        if (e && preset.engineName == e->getName()) {
            targetIdx = i;
            break;
        }
    }
    if (targetIdx >= 0)
        mgr.switchEngine(targetIdx);

    // 2. Apply engine params (integer string keys for all engines)
    IEngine* active = mgr.getActiveEngine();
    if (active) {
        for (int i = 0; i < active->getParamCount(); ++i)
            active->setParam(i, active->getParamDefault(i));
        for (const auto& [k, v] : preset.engineParams) {
            try {
                const int id = std::stoi(k);
                active->setParam(id, v);
            } catch (...) {}
        }
    }

    // 3. Apply effect chain
    chain.setConfig(preset.effects);
}
