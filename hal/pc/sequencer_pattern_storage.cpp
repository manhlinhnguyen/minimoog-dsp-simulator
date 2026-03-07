// ─────────────────────────────────────────────────────────
// FILE: hal/pc/sequencer_pattern_storage.cpp
// BRIEF: JSON save/load for step-sequencer patterns
// ─────────────────────────────────────────────────────────
#include "sequencer_pattern_storage.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;
using json   = nlohmann::json;

void PatternStorage::setDirectory(const std::string& dir) noexcept {
    dir_ = dir;
    fs::create_directories(dir_);
}

bool PatternStorage::savePattern(const SeqPattern&  pattern,
                                  const std::string& filename) {
    json j;
    j["name"]        = pattern.name;
    j["category"]    = pattern.category;
    j["description"] = pattern.description;
    j["steps"]       = pattern.stepCount;
    j["rate"]        = pattern.rateIdx;
    j["gate"]        = pattern.gate;
    j["swing"]       = pattern.swing;

    json arr = json::array();
    for (int i = 0; i < pattern.stepCount; ++i) {
        const auto& s = pattern.steps[i];
        arr.push_back({
            {"note",     s.note},
            {"velocity", s.velocity},
            {"gate",     s.gate},
            {"active",   s.active},
            {"tie",      s.tie}
        });
    }
    j["pattern"] = arr;

    const std::string path = dir_ + "/" + filename;
    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        lastError_ = "Cannot write: " + path;
        return false;
    }
    ofs << j.dump(2);
    return true;
}

bool PatternStorage::loadPattern(const std::string& filename,
                                  SeqPattern&        out) {
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

    out.name        = j.value("name",        filename);
    out.category    = j.value("category",    "User");
    out.description = j.value("description", "");
    out.stepCount   = j.value("steps",       16);
    out.rateIdx     = j.value("rate",        4);
    out.gate        = j.value("gate",        0.8f);
    out.swing       = j.value("swing",       0.0f);

    // Reset all steps to defaults first
    for (int i = 0; i < StepSequencer::MAX_STEPS; ++i)
        out.steps[i] = SeqStep{};

    if (j.contains("pattern") && j["pattern"].is_array()) {
        int idx = 0;
        for (const auto& s : j["pattern"]) {
            if (idx >= StepSequencer::MAX_STEPS) break;
            out.steps[idx].note     = s.value("note",     60);
            out.steps[idx].velocity = s.value("velocity", 100);
            out.steps[idx].gate     = s.value("gate",     0.8f);
            out.steps[idx].active   = s.value("active",   true);
            out.steps[idx].tie      = s.value("tie",      false);
            ++idx;
        }
    }
    return true;
}

std::vector<std::string> PatternStorage::listPatterns() const {
    std::vector<std::string> list;
    if (!fs::exists(dir_)) return list;
    for (const auto& entry : fs::directory_iterator(dir_)) {
        if (entry.path().extension() == ".json")
            list.push_back(entry.path().filename().string());
    }
    std::sort(list.begin(), list.end());
    return list;
}
