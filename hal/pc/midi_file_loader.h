// ─────────────────────────────────────────────────────────
// FILE: hal/pc/midi_file_loader.h
// BRIEF: Scan directory for .mid files; read bytes into memory
// ─────────────────────────────────────────────────────────
#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

namespace MidiFileLoader {

// Returns sorted list of .mid file paths in dir.
inline std::vector<std::string> scan(const std::string& dir) {
    std::vector<std::string> result;
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) return result;

    for (const auto& entry :
         std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        const auto& p = entry.path();
        const auto ext = p.extension().string();
        if (ext == ".mid" || ext == ".MID" || ext == ".midi")
            result.push_back(p.string());
    }
    std::sort(result.begin(), result.end());
    return result;
}

// Reads a file into a byte vector. Returns empty on failure.
inline std::vector<uint8_t> read(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto sz = f.tellg();
    if (sz <= 0) return {};
    f.seekg(0);
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    return buf;
}

// Returns just the stem (filename without extension).
inline std::string stem(const std::string& path) {
    return std::filesystem::path(path).stem().string();
}

} // namespace MidiFileLoader
