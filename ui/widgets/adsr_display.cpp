// ─────────────────────────────────────────────────────────
// FILE: ui/widgets/adsr_display.cpp
// BRIEF: ADSR shape visualizer
// ─────────────────────────────────────────────────────────
#include "adsr_display.h"

void AdsrDisplay::draw(float a, float d, float s, float r,
                        ImVec2 size) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 p  = ImGui::GetCursorScreenPos();
    const float  W  = size.x;
    const float  H  = size.y;
    const float  x0 = p.x;
    const float  y0 = p.y;
    const float  yB = y0 + H;
    const float  yT = y0;

    dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + W, yB),
                      IM_COL32(20, 20, 28, 200), 3.f);

    const float seg = W * 0.22f;
    const float xA  = x0 + a * seg;
    const float xD  = xA + d * seg;
    const float xS  = xD + seg * 0.5f;
    const float xR  = x0 + W - r * seg * 0.8f;
    const float yS  = yB - s * H;

    const ImU32 col = IM_COL32(240, 140, 20, 255);
    const float th  = 2.0f;
    dl->AddLine(ImVec2(x0,  yB), ImVec2(xA, yT), col, th);
    dl->AddLine(ImVec2(xA,  yT), ImVec2(xD, yS), col, th);
    dl->AddLine(ImVec2(xD,  yS), ImVec2(xS, yS), col, th);
    dl->AddLine(ImVec2(xS,  yS), ImVec2(xR, yB), col, th);

    dl->AddText(ImVec2(x0 + 2, yB - 12),
                IM_COL32(180, 120, 40, 200), "A");
    dl->AddText(ImVec2(xA + 2, yB - 12),
                IM_COL32(180, 120, 40, 200), "D");
    dl->AddText(ImVec2(xD + 2, yB - 12),
                IM_COL32(180, 120, 40, 200), "S");
    dl->AddText(ImVec2(xS + 2, yB - 12),
                IM_COL32(180, 120, 40, 200), "R");

    ImGui::Dummy(size);
}
