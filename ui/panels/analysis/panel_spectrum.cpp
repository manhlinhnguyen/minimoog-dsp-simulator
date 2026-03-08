// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_spectrum.cpp
// BRIEF: FFT Spectrum Analyzer — radix-2 FFT, log-freq axis, dB scale
// ─────────────────────────────────────────────────────────
#include "panel_spectrum.h"
#include "imgui.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdio>

namespace PanelSpectrum {

static constexpr int   FFT_N    = 512;
static constexpr int   FFT_BINS = FFT_N / 2;   // 256 usable bins
static constexpr float TWO_PI   = 6.28318530718f;
static constexpr float SR       = 44100.0f;

// In-place radix-2 DIT FFT (real input, imag initially 0)
static void fft512(float re[FFT_N], float im[FFT_N]) {
    // Bit-reversal permutation
    for (int i = 1, j = 0; i < FFT_N; ++i) {
        int bit = FFT_N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            float t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    // Butterfly stages
    for (int len = 2; len <= FFT_N; len <<= 1) {
        const float ang = -TWO_PI / static_cast<float>(len);
        const float wRe = cosf(ang), wIm = sinf(ang);
        for (int i = 0; i < FFT_N; i += len) {
            float curRe = 1.0f, curIm = 0.0f;
            for (int j = 0; j < len / 2; ++j) {
                const float uRe = re[i+j];
                const float uIm = im[i+j];
                const float vRe = re[i+j+len/2] * curRe - im[i+j+len/2] * curIm;
                const float vIm = re[i+j+len/2] * curIm + im[i+j+len/2] * curRe;
                re[i+j]       = uRe + vRe;  im[i+j]       = uIm + vIm;
                re[i+j+len/2] = uRe - vRe;  im[i+j+len/2] = uIm - vIm;
                const float nr = curRe*wRe - curIm*wIm;
                curIm = curRe*wIm + curIm*wRe;
                curRe = nr;
            }
        }
    }
}

void render(EngineManager& mgr) {
    ImGui::SetNextWindowSize(ImVec2(580, 240), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Spectrum Analyzer")) { ImGui::End(); return; }

    static float s_ring[EngineManager::OSC_BUF_SIZE];
    static float s_mag  [FFT_BINS] = {};   // smoothed magnitude in dB
    static float s_re   [FFT_N],   s_im[FFT_N];
    static float s_hann [FFT_N];
    static bool  s_ready = false;
    if (!s_ready) {
        for (int i = 0; i < FFT_N; ++i)
            s_hann[i] = 0.5f * (1.0f - cosf(TWO_PI * i / (FFT_N - 1)));
        s_ready = true;
    }

    int writePos = 0;
    mgr.getOscBuffer(s_ring, writePos);

    // Fill FFT input with windowed samples
    for (int i = 0; i < FFT_N; ++i) {
        const int idx = (writePos - FFT_N + i + EngineManager::OSC_BUF_SIZE)
                        & (EngineManager::OSC_BUF_SIZE - 1);
        s_re[i] = s_ring[idx] * s_hann[i];
        s_im[i] = 0.0f;
    }
    fft512(s_re, s_im);

    // Magnitude → dB, temporal smoothing
    static constexpr float SMOOTH = 0.2f;
    for (int k = 1; k < FFT_BINS; ++k) {
        const float mag = sqrtf(s_re[k]*s_re[k] + s_im[k]*s_im[k]) / (FFT_N * 0.5f);
        const float dB  = std::max(-80.0f, 20.0f * log10f(mag + 1e-9f));
        s_mag[k] = s_mag[k] * (1.0f - SMOOTH) + dB * SMOOTH;
    }

    // Draw
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float  pw    = avail.x;
    const float  ph    = std::max(avail.y - 32.0f, 80.0f);
    const ImVec2 p0    = ImGui::GetCursorScreenPos();
    ImDrawList*  dl    = ImGui::GetWindowDrawList();

    dl->AddRectFilled(p0, ImVec2(p0.x + pw, p0.y + ph), IM_COL32(10, 12, 16, 255));

    // dB grid lines: 0, -20, -40, -60, -80
    for (int db = 0; db >= -80; db -= 20) {
        const float y = p0.y + ph * (1.0f - (db + 80.0f) / 80.0f);
        dl->AddLine(ImVec2(p0.x, y), ImVec2(p0.x + pw, y), IM_COL32(35, 50, 35, 200));
        char lbl[8]; snprintf(lbl, sizeof(lbl), "%d", db);
        dl->AddText(ImVec2(p0.x + 3, y - 13), IM_COL32(55, 80, 55, 200), lbl);
    }

    // Log frequency axis
    const float freqMin  = 20.0f, freqMax = SR * 0.5f;
    const float logMin   = log10f(freqMin);
    const float logRange = log10f(freqMax) - logMin;

    // Draw frequency markers
    const float fMarkers[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    const char* fLabels [] = { "50","100","200","500","1k","2k","5k","10k","20k" };
    for (int m = 0; m < 9; ++m) {
        const float t  = (log10f(fMarkers[m]) - logMin) / logRange;
        const float gx = p0.x + t * pw;
        dl->AddLine(ImVec2(gx, p0.y), ImVec2(gx, p0.y + ph), IM_COL32(35, 50, 35, 180));
        dl->AddText(ImVec2(gx + 2, p0.y + ph - 14), IM_COL32(65, 90, 65, 220), fLabels[m]);
    }

    // Filled spectrum
    float prevX = -1.0f, prevY = p0.y + ph;
    for (int k = 1; k < FFT_BINS; ++k) {
        const float freq = k * SR / FFT_N;
        if (freq < freqMin || freq > freqMax) continue;
        const float t    = (log10f(freq) - logMin) / logRange;
        const float x    = p0.x + t * pw;
        const float norm = std::clamp((s_mag[k] + 80.0f) / 80.0f, 0.0f, 1.0f);
        const float y    = p0.y + ph * (1.0f - norm);
        const ImU32 col  = norm > 0.85f ? IM_COL32(255, 220, 60, 220) :
                           norm > 0.60f ? IM_COL32(80, 220, 80, 200) :
                                          IM_COL32(30, 150, 55, 190);
        if (prevX >= p0.x)
            dl->AddRectFilled(ImVec2(prevX, std::min(prevY, y)),
                              ImVec2(x + 0.5f, p0.y + ph), col);
        prevX = x; prevY = y;
    }

    ImGui::Dummy(ImVec2(pw, ph));
    ImGui::End();
}

} // namespace PanelSpectrum
