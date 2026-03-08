// ─────────────────────────────────────────────────────────
// FILE: hal/pc/engine_preset_storage.h
// BRIEF: Generic JSON preset load/save for any IEngine.
//        All engines use integer-key format:
//          { "engine": "...", "name": "...",
//            "params": { "0": 1.0, "1": 0.5, ... } }
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/iengine.h"
#include <string>
#include <vector>

class EnginePresetStorage {
public:
    void setDirectory(const std::string& dir) noexcept;

    // List preset filenames in directory (sorted alphabetically)
    std::vector<std::string> list() const;

    // Load preset from file into engine via setParam().
    // Returns false on file error or engine-name mismatch.
    bool load(const std::string& filename, IEngine& engine) const;

    // Save current engine params to file.
    bool save(const std::string& filename,
              const std::string& presetName,
              const IEngine&     engine) const;

    // Convenience: load by index from list()
    bool loadByIndex(int idx, IEngine& engine) const;

    const std::string& getDirectory() const noexcept { return dir_; }
    std::string        getLastError() const noexcept { return lastError_; }

private:
    std::string         dir_;
    mutable std::string lastError_;
};
