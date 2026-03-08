// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_lissajous.cpp
// BRIEF: Lissajous Vectorscope — X=L, Y=R, fading trail
// ─────────────────────────────────────────────────────────
#include "panel_lissajous.h"
#include "imgui.h"
#include <cmath>
#include <algorithm>

namespace PanelLissajous {

void render(EngineManager& mgr) {
    ImGui::SetNextWindowSize(ImVec2(300, 330), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Lissajous / Vectorscope")) { ImGui::End(); return; }

    static float s_L[EngineManager::OSC_BUF_SIZE];
    static float s_R[EngineManager::OSC_BUF_SIZE];

    int writePos = 0;
    mgr.getOscBufferStereo(s_L, s_R, writePos);

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float  size  = std::max(40.0f, std::min(avail.x, avail.y - 20.0f));
    const ImVec2 p0    = ImGui::GetCursorScreenPos();
    const float  half  = size * 0.5f;
    const ImVec2 ctr   = ImVec2(p0.x + half, p0.y + half);
    ImDrawList*  dl    = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled(p0, ImVec2(p0.x + size, p0.y + size), IM_COL32(8, 10, 14, 255));

    // Grid: cross-hairs + diagonals
    dl->AddLine(ImVec2(ctr.x, p0.y), ImVec2(ctr.x, p0.y + size), IM_COL32(30, 45, 30, 255));
    dl->AddLine(ImVec2(p0.x, ctr.y), ImVec2(p0.x + size, ctr.y), IM_COL32(30, 45, 30, 255));
    dl->AddLine(p0, ImVec2(p0.x + size, p0.y + size),             IM_COL32(20, 35, 20, 220));
    dl->AddLine(ImVec2(p0.x + size, p0.y), ImVec2(p0.x, p0.y + size), IM_COL32(20, 35, 20, 220));

    // Border circle
    dl->AddCircle(ctr, half * 0.98f, IM_COL32(30, 45, 30, 120), 64);

    // Draw line segments with alpha fade: oldest = dim, newest = bright
    static constexpr int PLOT_N = 512;
    const float scale = half * 0.90f;

    for (int i = 0; i < PLOT_N - 1; ++i) {
        const int idx0 = (writePos - PLOT_N + i     + EngineManager::OSC_BUF_SIZE) & (EngineManager::OSC_BUF_SIZE - 1);
        const int idx1 = (writePos - PLOT_N + i + 1 + EngineManager::OSC_BUF_SIZE) & (EngineManager::OSC_BUF_SIZE - 1);
        const float x0 = ctr.x + s_L[idx0] * scale;
        const float y0 = ctr.y - s_R[idx0] * scale;
        const float x1 = ctr.x + s_L[idx1] * scale;
        const float y1 = ctr.y - s_R[idx1] * scale;
        const int   a  = 30 + (i * 200) / PLOT_N;
        dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(50, 220, 100, a), 1.3f);
    }

    ImGui::Dummy(ImVec2(size, size));

    // Labels
    ImGui::SetCursorScreenPos(ImVec2(p0.x + 2,        p0.y + half - 8));
    ImGui::TextDisabled("L");
    ImGui::SetCursorScreenPos(ImVec2(p0.x + size - 10, p0.y + half - 8));
    ImGui::TextDisabled("R");
    ImGui::SetCursorScreenPos(ImVec2(p0.x + half - 3,  p0.y + 1));
    ImGui::TextDisabled("+");
    ImGui::SetCursorScreenPos(ImVec2(p0.x + half - 3,  p0.y + size - 14));
    ImGui::TextDisabled("-");

    ImGui::SetCursorScreenPos(ImVec2(p0.x, p0.y + size + 4));
    ImGui::TextDisabled("X = Left  |  Y = Right  |  diag = Mono");

    ImGui::End();
}

} // namespace PanelLissajous
