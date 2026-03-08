// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_spectrogram.cpp
// BRIEF: Spectrogram waterfall — 512-pt FFT, 64 time rows, color LUT
// Time flows top→bottom. Each row = one FFT snapshot ~15/sec.
// ─────────────────────────────────────────────────────────
#include "panel_spectrogram.h"
#include "imgui.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace PanelSpectrogram {

static constexpr int   FFT_N    = 512;
static constexpr int   BINS     = 128;   // display first 128 bins (0..11.025 kHz)
static constexpr int   ROWS     = 64;    // time history
static constexpr int   FPR      = 3;     // frames-per-row: update every 3 display frames
static constexpr float TWO_PI   = 6.28318530718f;

static void fft512(float re[FFT_N], float im[FFT_N]) {
    for (int i = 1, j = 0; i < FFT_N; ++i) {
        int bit = FFT_N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            float t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    for (int len = 2; len <= FFT_N; len <<= 1) {
        const float ang = -TWO_PI / static_cast<float>(len);
        const float wRe = cosf(ang), wIm = sinf(ang);
        for (int i = 0; i < FFT_N; i += len) {
            float curRe = 1.0f, curIm = 0.0f;
            for (int j = 0; j < len / 2; ++j) {
                const float uRe = re[i+j], uIm = im[i+j];
                const float vRe = re[i+j+len/2]*curRe - im[i+j+len/2]*curIm;
                const float vIm = re[i+j+len/2]*curIm + im[i+j+len/2]*curRe;
                re[i+j]       = uRe+vRe;  im[i+j]       = uIm+vIm;
                re[i+j+len/2] = uRe-vRe;  im[i+j+len/2] = uIm-vIm;
                const float nr = curRe*wRe - curIm*wIm;
                curIm = curRe*wIm + curIm*wRe;
                curRe = nr;
            }
        }
    }
}

// Color LUT: norm 0..1 → RGBA (dark→green→yellow→white)
static ImU32 specColor(float norm) {
    if (norm <= 0.0f) return IM_COL32(8, 10, 16, 255);
    if (norm < 0.4f) {
        const float t = norm / 0.4f;
        return IM_COL32((int)(8  + t * 12),
                        (int)(10 + t * 120),
                        (int)(16 + t * 20), 255);
    }
    if (norm < 0.75f) {
        const float t = (norm - 0.4f) / 0.35f;
        return IM_COL32((int)(20  + t * 180),
                        (int)(130 + t * 80),
                        (int)(36  - t * 16), 255);
    }
    const float t = std::min((norm - 0.75f) / 0.25f, 1.0f);
    return IM_COL32((int)(200 + t * 55),
                    (int)(210 + t * 45),
                    (int)(20  + t * 200), 255);
}

void render(EngineManager& mgr) {
    ImGui::SetNextWindowSize(ImVec2(580, 220), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Spectrogram")) { ImGui::End(); return; }

    static float s_ring  [EngineManager::OSC_BUF_SIZE];
    static float s_hist  [ROWS][BINS] = {};  // history[row][bin] = norm 0..1
    static int   s_head  = 0;                // index of oldest row (scrolls)
    static int   s_frame = 0;
    static float s_hann  [FFT_N];
    static float s_re    [FFT_N], s_im[FFT_N];
    static bool  s_ready = false;
    if (!s_ready) {
        for (int i = 0; i < FFT_N; ++i)
            s_hann[i] = 0.5f * (1.0f - cosf(TWO_PI * i / (FFT_N - 1)));
        s_ready = true;
    }

    int writePos = 0;
    mgr.getOscBuffer(s_ring, writePos);

    // Update one FFT row every FPR display frames
    ++s_frame;
    if (s_frame >= FPR) {
        s_frame = 0;
        for (int i = 0; i < FFT_N; ++i) {
            const int idx = (writePos - FFT_N + i + EngineManager::OSC_BUF_SIZE)
                            & (EngineManager::OSC_BUF_SIZE - 1);
            s_re[i] = s_ring[idx] * s_hann[i];
            s_im[i] = 0.0f;
        }
        fft512(s_re, s_im);
        // Store magnitude (norm 0..1 from dB -80..0) in newest row
        const int row = s_head;
        for (int k = 0; k < BINS; ++k) {
            const float mag = sqrtf(s_re[k]*s_re[k] + s_im[k]*s_im[k]) / (FFT_N * 0.5f);
            const float dB  = std::max(-80.0f, 20.0f * log10f(mag + 1e-9f));
            s_hist[row][k] = (dB + 80.0f) / 80.0f;
        }
        s_head = (s_head + 1) % ROWS;  // advance — s_head now points to oldest
    }

    // Draw
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float  pw    = avail.x;
    const float  ph    = std::max(60.0f, avail.y - 10.0f);
    const ImVec2 p0    = ImGui::GetCursorScreenPos();
    ImDrawList*  dl    = ImGui::GetWindowDrawList();

    dl->AddRectFilled(p0, ImVec2(p0.x + pw, p0.y + ph), IM_COL32(8, 10, 16, 255));

    const float cellW = pw  / BINS;
    const float cellH = ph  / ROWS;

    // Draw from oldest (top) to newest (bottom)
    for (int r = 0; r < ROWS; ++r) {
        const int row = (s_head + r) % ROWS;  // s_head = oldest
        const float ry = p0.y + r * cellH;
        for (int k = 0; k < BINS; ++k) {
            const float kx = p0.x + k * cellW;
            dl->AddRectFilled(ImVec2(kx, ry),
                              ImVec2(kx + cellW + 0.5f, ry + cellH + 0.5f),
                              specColor(s_hist[row][k]));
        }
    }

    ImGui::Dummy(ImVec2(pw, ph));
    ImGui::End();
}

} // namespace PanelSpectrogram
