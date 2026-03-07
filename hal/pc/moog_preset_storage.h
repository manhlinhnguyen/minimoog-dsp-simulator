// ─────────────────────────────────────────────────────────
// FILE: hal/pc/moog_preset_storage.h
// BRIEF: JSON preset save/load via nlohmann/json
// ─────────────────────────────────────────────────────────
#pragma once
#include "shared/params.h"
#include "shared/interfaces.h"
#include "core/music/sequencer.h"
#include <string>
#include <vector>

struct Preset {
    std::string name;
    std::string category;
    float       params[PARAM_COUNT];
    SeqStep     seqSteps[StepSequencer::MAX_STEPS];
};

class PresetStorage {
public:
    void setDirectory(const std::string& dir) noexcept;

    bool savePreset(const Preset&      preset,
                    const std::string& filename);
    bool loadPreset(const std::string& filename,
                    Preset&            out);

    void applyPreset(const Preset&     preset,
                     AtomicParamStore& store,
                     StepSequencer&    seq) noexcept;

    Preset capturePreset(const AtomicParamStore& store,
                         const StepSequencer&    seq,
                         const std::string&      name,
                         const std::string&      category);

    std::vector<std::string> listPresets() const;

    std::string getLastError() const { return lastError_; }

private:
    std::string dir_       = "./moog_presets";
    std::string lastError_;
};
