// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_correlation.cpp
// BRIEF: Stereo Correlation Meter — Pearson ρ(L,R) over 512 samples
//   +1 = mono (identical)  |  0 = decorrelated stereo  |  -1 = anti-phase
// ─────────────────────────────────────────────────────────
#include "panel_correlation.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace PanelCorrelation {

static constexpr int   CORR_N   = 512;
static constexpr float SMOOTH   = 0.15f;  // temporal smoothing

static float computeCorr(const float* L, const float* R, int writePos) {
    const int BUF = EngineManager::OSC_BUF_SIZE;
    float sumLR = 0.0f, sumL2 = 0.0f, sumR2 = 0.0f;
    for (int i = 0; i < CORR_N; ++i) {
        const int idx = (writePos - CORR_N + i + BUF) & (BUF - 1);
        const float l = L[idx], r = R[idx];
        sumLR += l * r;
        sumL2 += l * l;
        sumR2 += r * r;
    }
    const float denom = sqrtf(sumL2 * sumR2);
    return denom > 1e-9f ? std::clamp(sumLR / denom, -1.0f, 1.0f) : 0.0f;
}

void render(EngineManager& mgr) {
    ImGui::SetNextWindowSize(ImVec2(400, 90), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Correlation Meter")) { ImGui::End(); return; }

    static float s_L[EngineManager::OSC_BUF_SIZE];
    static float s_R[EngineManager::OSC_BUF_SIZE];
    static float s_corr = 0.0f;

    int writePos = 0;
    mgr.getOscBufferStereo(s_L, s_R, writePos);

    const float raw   = computeCorr(s_L, s_R, writePos);
    s_corr = s_corr * (1.0f - SMOOTH) + raw * SMOOTH;

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float  pw    = avail.x;
    const float  ph    = std::max(40.0f, avail.y - 20.0f);
    const ImVec2 p0    = ImGui::GetCursorScreenPos();
    ImDrawList*  dl    = ImGui::GetWindowDrawList();

    // Background bar
    dl->AddRectFilled(p0, ImVec2(p0.x + pw, p0.y + ph), IM_COL32(18, 20, 24, 255));

    // Center line (0 = decorrelated stereo)
    const float cx = p0.x + pw * 0.5f;
    dl->AddLine(ImVec2(cx, p0.y), ImVec2(cx, p0.y + ph), IM_COL32(55, 65, 55, 255));

    // Filled correlation bar
    const float corrX = p0.x + (s_corr + 1.0f) * 0.5f * pw;
    const ImU32 col   = s_corr < -0.1f ? IM_COL32(220, 60, 40, 230)  // anti-phase = red
                      : s_corr <  0.3f ? IM_COL32(220, 190, 40, 230) // wide stereo = yellow
                                       : IM_COL32(50, 200, 70, 230);  // mono/normal = green
    if (s_corr >= 0.0f)
        dl->AddRectFilled(ImVec2(cx, p0.y + 4), ImVec2(corrX, p0.y + ph - 4), col);
    else
        dl->AddRectFilled(ImVec2(corrX, p0.y + 4), ImVec2(cx, p0.y + ph - 4), col);

    // Tick marks at -1, -0.5, 0, +0.5, +1
    for (int t = 0; t <= 4; ++t) {
        const float tv = -1.0f + t * 0.5f;
        const float tx = p0.x + (tv + 1.0f) * 0.5f * pw;
        dl->AddLine(ImVec2(tx, p0.y), ImVec2(tx, p0.y + 6), IM_COL32(70, 70, 70, 200));
        dl->AddLine(ImVec2(tx, p0.y + ph - 6), ImVec2(tx, p0.y + ph), IM_COL32(70, 70, 70, 200));
    }

    // Labels: -1, 0, +1
    dl->AddText(ImVec2(p0.x + 2,        p0.y + ph/2 - 6), IM_COL32(160, 80, 60, 255), "-1");
    dl->AddText(ImVec2(cx - 3,           p0.y + ph/2 - 6), IM_COL32(140, 140, 100, 255), "0");
    dl->AddText(ImVec2(p0.x + pw - 14,  p0.y + ph/2 - 6), IM_COL32(80, 180, 80, 255), "+1");

    ImGui::Dummy(ImVec2(pw, ph));

    // Numerical readout + interpretation
    const char* interp = s_corr > 0.8f  ? "MONO"
                       : s_corr > 0.3f  ? "Narrow stereo"
                       : s_corr > -0.1f ? "Wide stereo"
                       : s_corr > -0.5f ? "Partial anti-phase"
                                        : "ANTI-PHASE  [!]";
    char buf[48];
    snprintf(buf, sizeof(buf), "ρ = %+.3f   %s", s_corr, interp);
    ImGui::TextDisabled("%s", buf);

    ImGui::End();
}

} // namespace PanelCorrelation
