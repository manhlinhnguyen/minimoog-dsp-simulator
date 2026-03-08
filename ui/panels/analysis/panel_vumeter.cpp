// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_vumeter.cpp
// BRIEF: Stereo VU+Peak meter — ballistic RMS, peak hold 2s
// ─────────────────────────────────────────────────────────
#include "panel_vumeter.h"
#include "imgui.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdio>

namespace PanelVuMeter {

struct Channel {
    float vu            = 0.0f;   // ballistic RMS linear
    float peak          = 0.0f;   // peak hold linear
    int   peakHoldLeft  = 0;      // frames remaining in hold
};

static constexpr float VU_ATTACK_COEF  = 0.12f;   // ~2 frames rise
static constexpr float VU_RELEASE_COEF = 0.975f;  // ~80 frames fall
static constexpr int   PEAK_HOLD_FRAMES = 120;    // 2s @ 60fps
static constexpr float PEAK_DECAY_COEF = 0.990f;
static constexpr int   RMS_N           = 512;

void render(EngineManager& mgr) {
    ImGui::SetNextWindowSize(ImVec2(160, 280), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("VU Meter")) { ImGui::End(); return; }

    static float   s_L[EngineManager::OSC_BUF_SIZE];
    static float   s_R[EngineManager::OSC_BUF_SIZE];
    static Channel s_ch[2];

    int writePos = 0;
    mgr.getOscBufferStereo(s_L, s_R, writePos);

    // Compute RMS + peak over last RMS_N samples
    float rms[2]  = {};
    float pkNew[2]= {};
    const float* bufs[2] = { s_L, s_R };
    for (int c = 0; c < 2; ++c) {
        for (int i = 0; i < RMS_N; ++i) {
            const int idx = (writePos - RMS_N + i + EngineManager::OSC_BUF_SIZE)
                            & (EngineManager::OSC_BUF_SIZE - 1);
            const float v = bufs[c][idx];
            rms[c]  += v * v;
            const float av = v < 0.0f ? -v : v;
            if (av > pkNew[c]) pkNew[c] = av;
        }
        rms[c] = sqrtf(rms[c] / RMS_N);
    }

    // Apply ballistics
    for (int c = 0; c < 2; ++c) {
        s_ch[c].vu = rms[c] > s_ch[c].vu
            ? s_ch[c].vu * (1.0f - VU_ATTACK_COEF) + rms[c] * VU_ATTACK_COEF
            : s_ch[c].vu * VU_RELEASE_COEF;
        if (pkNew[c] >= s_ch[c].peak) {
            s_ch[c].peak         = pkNew[c];
            s_ch[c].peakHoldLeft = PEAK_HOLD_FRAMES;
        } else if (s_ch[c].peakHoldLeft > 0) {
            --s_ch[c].peakHoldLeft;
        } else {
            s_ch[c].peak *= PEAK_DECAY_COEF;
        }
    }

    // Draw
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float  barH  = std::max(60.0f, avail.y - 28.0f);
    const float  barW  = (avail.x - 24.0f) * 0.5f;
    const ImVec2 p0    = ImGui::GetCursorScreenPos();
    ImDrawList*  dl    = ImGui::GetWindowDrawList();

    const float dBMin = -60.0f, dBRange = 60.0f;

    // y_top = p0.y (0 dB), y_bot = p0.y + barH (-60 dB)
    const float y_m3  = p0.y + barH * (3.0f / dBRange);   // -3 dB
    const float y_m6  = p0.y + barH * (6.0f / dBRange);   // -6 dB

    const char* names[2] = { "L", "R" };

    for (int c = 0; c < 2; ++c) {
        const float bx   = p0.x + c * (barW + 12.0f);
        const float y_bot= p0.y + barH;

        // Background
        dl->AddRectFilled(ImVec2(bx, p0.y), ImVec2(bx + barW, y_bot),
                          IM_COL32(18, 20, 24, 255));

        // Filled VU bar
        const float vuDB   = 20.0f * log10f(s_ch[c].vu + 1e-9f);
        const float vuNorm = std::clamp((vuDB - dBMin) / dBRange, 0.0f, 1.0f);
        if (vuNorm > 0.001f) {
            const float vuTop = p0.y + barH * (1.0f - vuNorm);
            // Green zone (bottom → -6dB)
            dl->AddRectFilled(ImVec2(bx, std::max(vuTop, y_m6)),
                              ImVec2(bx + barW, y_bot), IM_COL32(35, 180, 55, 230));
            // Yellow zone (-6dB → -3dB)
            if (vuTop < y_m6)
                dl->AddRectFilled(ImVec2(bx, std::max(vuTop, y_m3)),
                                  ImVec2(bx + barW, y_m6), IM_COL32(240, 200, 40, 230));
            // Red zone (-3dB → 0dB)
            if (vuTop < y_m3)
                dl->AddRectFilled(ImVec2(bx, vuTop),
                                  ImVec2(bx + barW, y_m3), IM_COL32(240, 55, 40, 230));
        }

        // Peak hold indicator
        const float pkDB   = 20.0f * log10f(s_ch[c].peak + 1e-9f);
        const float pkNorm = std::clamp((pkDB - dBMin) / dBRange, 0.0f, 1.0f);
        if (pkNorm > 0.005f) {
            const float pkY  = p0.y + barH * (1.0f - pkNorm);
            const ImU32 pkCl = pkDB > -3.0f ? IM_COL32(255, 80, 60, 255)
                                            : IM_COL32(240, 230, 80, 255);
            dl->AddRectFilled(ImVec2(bx, pkY - 2), ImVec2(bx + barW, pkY + 2), pkCl);
        }

        // dB tick marks every 10 dB
        for (int db = 0; db >= -60; db -= 10) {
            const float ty = p0.y + barH * (-db / dBRange);
            dl->AddLine(ImVec2(bx, ty), ImVec2(bx + barW * 0.4f, ty),
                        IM_COL32(70, 70, 70, 180));
            if (db % 20 == 0) {
                char lbl[5]; snprintf(lbl, sizeof(lbl), "%d", db);
                dl->AddText(ImVec2(bx + barW * 0.45f, ty - 6),
                            IM_COL32(90, 90, 90, 200), lbl);
            }
        }

        // Channel label + dB reading
        dl->AddText(ImVec2(bx + barW * 0.3f, y_bot + 3),
                    IM_COL32(180, 160, 110, 255), names[c]);
        char dbStr[8];
        snprintf(dbStr, sizeof(dbStr), "%+.1f", vuDB < -60.f ? -60.f : vuDB);
        dl->AddText(ImVec2(bx, p0.y - 15), IM_COL32(140, 190, 130, 255), dbStr);
    }

    ImGui::Dummy(ImVec2(avail.x, barH + 22.0f));
    ImGui::End();
}

} // namespace PanelVuMeter
