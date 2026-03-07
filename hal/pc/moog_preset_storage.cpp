// ─────────────────────────────────────────────────────────
// FILE: hal/pc/moog_preset_storage.cpp
// BRIEF: JSON preset persistence
// ─────────────────────────────────────────────────────────
#include "moog_preset_storage.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using json   = nlohmann::json;

void PresetStorage::setDirectory(const std::string& dir) noexcept {
    dir_ = dir;
    fs::create_directories(dir_);
}

bool PresetStorage::savePreset(const Preset&      preset,
                                const std::string& filename) {
    json j;
    j["name"]     = preset.name;
    j["category"] = preset.category;

    // Only save synth params — music params (arp/seq/chord/scale) and
    // seq_steps are managed separately by the Music panel / pattern storage.
    json pj = json::object();
    for (int i = 0; i < PARAM_COUNT; ++i)
        if (!isMusicParam(i))
            pj[PARAM_META[i].jsonKey] = preset.params[i];
    j["params"] = pj;

    const std::string path = dir_ + "/" + filename;
    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        lastError_ = "Cannot write: " + path;
        return false;
    }
    ofs << j.dump(2);
    return true;
}

bool PresetStorage::loadPreset(const std::string& filename,
                                Preset&            out) {
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
        lastError_ = std::string("JSON parse error: ") + e.what();
        return false;
    }

    out.name     = j.value("name",     filename);
    out.category = j.value("category", "User");

    for (int i = 0; i < PARAM_COUNT; ++i)
        out.params[i] = PARAM_META[i].defaultVal;

    if (j.contains("params") && j["params"].is_object()) {
        for (int i = 0; i < PARAM_COUNT; ++i) {
            const char* key = PARAM_META[i].jsonKey;
            if (j["params"].contains(key))
                out.params[i] = j["params"][key].get<float>();
        }
    }

    if (j.contains("seq_steps") && j["seq_steps"].is_array()) {
        int idx = 0;
        for (const auto& s : j["seq_steps"]) {
            if (idx >= StepSequencer::MAX_STEPS) break;
            out.seqSteps[idx].note     = s.value("note",     60);
            out.seqSteps[idx].velocity = s.value("velocity", 100);
            out.seqSteps[idx].gate     = s.value("gate",     0.8f);
            out.seqSteps[idx].active   = s.value("active",   true);
            out.seqSteps[idx].tie      = s.value("tie",      false);
            ++idx;
        }
    }
    return true;
}

void PresetStorage::applyPreset(const Preset&     preset,
                                 AtomicParamStore& store,
                                 StepSequencer&    seq) noexcept {
    for (int i = 0; i < PARAM_COUNT; ++i)
        store.set(i, preset.params[i]);
    for (int i = 0; i < StepSequencer::MAX_STEPS; ++i)
        seq.setStep(i, preset.seqSteps[i]);
}

Preset PresetStorage::capturePreset(
        const AtomicParamStore& store,
        const StepSequencer&    seq,
        const std::string&      name,
        const std::string&      category) {
    Preset p;
    p.name     = name;
    p.category = category;
    store.snapshot(p.params);
    for (int i = 0; i < StepSequencer::MAX_STEPS; ++i)
        p.seqSteps[i] = seq.getStep(i);
    return p;
}

std::vector<std::string> PresetStorage::listPresets() const {
    std::vector<std::string> list;
    if (!fs::exists(dir_)) return list;
    for (const auto& entry : fs::directory_iterator(dir_)) {
        if (entry.path().extension() == ".json")
            list.push_back(entry.path().filename().string());
    }
    std::sort(list.begin(), list.end());
    return list;
}
