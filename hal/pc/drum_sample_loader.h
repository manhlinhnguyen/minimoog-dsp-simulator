// ─────────────────────────────────────────────────────────
// FILE: hal/pc/drum_sample_loader.h
// BRIEF: WAV loader for drum sample pads (header-only, dr_wav)
// ─────────────────────────────────────────────────────────
#pragma once
#include "core/engines/drums/drum_engine.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>

// ════════════════════════════════════════════════════════
// Minimal dr_wav-style WAV reader (header-only, no deps).
// Only supports PCM-16 and IEEE-float WAV files.
// ════════════════════════════════════════════════════════

namespace DrumSampleLoader {

struct WavResult {
    std::vector<float> audioL;
    std::vector<float> audioR;
    float sampleRate = 0.0f;
    bool  ok         = false;
    std::string error;
};

// Read a 4-byte LE integer from raw bytes
inline uint32_t read32LE(const uint8_t* p) noexcept {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8)  |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}
inline uint16_t read16LE(const uint8_t* p) noexcept {
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

inline WavResult loadWav(const std::string& path) {
    WavResult res;
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) { res.error = "Cannot open: " + path; return res; }

    uint8_t hdr[44];
    if (fread(hdr, 1, 44, f) < 44) {
        fclose(f); res.error = "Too short"; return res;
    }

    if (memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0) {
        fclose(f); res.error = "Not a WAV file"; return res;
    }

    const uint16_t audioFmt  = read16LE(hdr + 20);
    const uint16_t nChannels = read16LE(hdr + 22);
    const uint32_t sr        = read32LE(hdr + 24);
    const uint16_t bitDepth  = read16LE(hdr + 34);

    // Find 'data' chunk (may not be at fixed offset 36)
    // Back up and scan for 'data' marker
    fseek(f, 36, SEEK_SET);
    uint8_t tag[4];
    uint32_t chunkSize = 0;
    while (fread(tag, 1, 4, f) == 4) {
        uint8_t sz[4];
        if (fread(sz, 1, 4, f) < 4) break;
        chunkSize = read32LE(sz);
        if (memcmp(tag, "data", 4) == 0) break;
        fseek(f, static_cast<long>(chunkSize), SEEK_CUR);
    }

    if (memcmp(tag, "data", 4) != 0) {
        fclose(f); res.error = "No data chunk"; return res;
    }

    res.sampleRate = static_cast<float>(sr);
    const int totalSamples = static_cast<int>(chunkSize)
                           / (nChannels * (bitDepth / 8));

    res.audioL.resize(totalSamples);
    if (nChannels > 1) res.audioR.resize(totalSamples);

    if (audioFmt == 1 && bitDepth == 16) {
        // PCM-16
        for (int i = 0; i < totalSamples; ++i) {
            int16_t s;
            if (fread(&s, 2, 1, f) < 1) break;
            res.audioL[i] = s / 32768.0f;
            if (nChannels > 1) {
                if (fread(&s, 2, 1, f) < 1) break;
                res.audioR[i] = s / 32768.0f;
            }
        }
    } else if (audioFmt == 3 && bitDepth == 32) {
        // IEEE float
        for (int i = 0; i < totalSamples; ++i) {
            float s;
            if (fread(&s, 4, 1, f) < 1) break;
            res.audioL[i] = s;
            if (nChannels > 1) {
                if (fread(&s, 4, 1, f) < 1) break;
                res.audioR[i] = s;
            }
        }
    } else {
        fclose(f);
        res.error = "Unsupported WAV format (need PCM-16 or float-32)";
        return res;
    }

    fclose(f);
    res.ok = true;
    return res;
}

// Load default drum kit from assets/drum_kits/<kitName>/
// Expects files: pad09.wav .. pad16.wav (or any .wav, alphabetically assigned)
inline void loadKitIntoEngine(DrumEngine& engine,
                               const std::string& kitDir) {
    // Map standard filenames for 8 sample slots
    static const char* const DEFAULT_FILES[SAMPLE_PADS] = {
        "pad09.wav","pad10.wav","pad11.wav","pad12.wav",
        "pad13.wav","pad14.wav","pad15.wav","pad16.wav"
    };
    for (int i = 0; i < SAMPLE_PADS; ++i) {
        const std::string path = kitDir + "/" + DEFAULT_FILES[i];
        auto res = loadWav(path);
        if (res.ok) {
            engine.loadSamplePad(i,
                std::move(res.audioL),
                std::move(res.audioR),
                res.sampleRate,
                DEFAULT_FILES[i]);
        }
        // Silently skip missing files — pad stays silent
    }
}

} // namespace DrumSampleLoader
