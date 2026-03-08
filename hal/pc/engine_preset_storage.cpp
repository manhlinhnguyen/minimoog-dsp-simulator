// ─────────────────────────────────────────────────────────
// FILE: hal/pc/engine_preset_storage.cpp
// BRIEF: EnginePresetStorage implementation
// ─────────────────────────────────────────────────────────
#include "engine_preset_storage.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

using json = nlohmann::json;
namespace fs = std::filesystem;

void EnginePresetStorage::setDirectory(const std::string& dir) noexcept {
    dir_ = dir;
}

std::vector<std::string> EnginePresetStorage::list() const {
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

bool EnginePresetStorage::load(const std::string& filename,
                                IEngine& engine) const {
    const std::string path = dir_ + "/" + filename;
    std::ifstream f(path);
    if (!f.is_open()) {
        lastError_ = "Cannot open: " + path;
        return false;
    }
    try {
        json j;
        f >> j;

        if (j.contains("engine")) {
            const std::string eName = j["engine"].get<std::string>();
            if (eName != engine.getName()) {
                lastError_ = "Engine mismatch: preset is for '" + eName + "'";
                return false;
            }
        }

        if (!j.contains("params")) {
            lastError_ = "No 'params' key in preset";
            return false;
        }

        const json& params = j["params"];
        for (auto it = params.begin(); it != params.end(); ++it) {
            try {
                const int   id  = std::stoi(it.key());
                const float val = it.value().get<float>();
                engine.setParam(id, val);
            } catch (...) {
                // Skip non-integer keys (e.g., named keys in legacy files)
            }
        }
        lastError_.clear();
        return true;
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return false;
    }
}

bool EnginePresetStorage::save(const std::string& filename,
                                const std::string& presetName,
                                const IEngine&     engine) const {
    try { fs::create_directories(dir_); } catch (...) {}

    const std::string path = dir_ + "/" + filename;
    try {
        json j;
        j["engine"] = engine.getName();
        j["name"]   = presetName;

        json params;
        for (int i = 0; i < engine.getParamCount(); ++i)
            params[std::to_string(i)] = engine.getParam(i);
        j["params"] = params;

        std::ofstream f(path);
        if (!f.is_open()) {
            lastError_ = "Cannot write: " + path;
            return false;
        }
        f << j.dump(4);
        lastError_.clear();
        return true;
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return false;
    }
}

bool EnginePresetStorage::loadByIndex(int idx, IEngine& engine) const {
    const auto names = list();
    if (idx < 0 || idx >= static_cast<int>(names.size())) return false;
    return load(names[idx], engine);
}
