// ─────────────────────────────────────────────────────────
// FILE: hal/pc/pattern_storage.h
// BRIEF: JSON save/load for step-sequencer patterns
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/music/sequencer.h"
#include <string>
#include <vector>

struct SeqPattern {
    std::string name;
    std::string category;
    std::string description;
    int   stepCount = 16;
    int   rateIdx   = 4;     // index into rate table: 0=1/1 1=1/2 2=1/4 3=1/8 4=1/16 5=3/8 6=3/16 7=1/4T
    float gate      = 0.8f;
    float swing     = 0.0f;
    SeqStep steps[StepSequencer::MAX_STEPS];
};

class PatternStorage {
public:
    void setDirectory(const std::string& dir) noexcept;

    bool savePattern(const SeqPattern& pattern, const std::string& filename);
    bool loadPattern(const std::string& filename, SeqPattern& out);

    std::vector<std::string> listPatterns() const;

    std::string getLastError() const { return lastError_; }

private:
    std::string dir_       = "./patterns";
    std::string lastError_;
};
