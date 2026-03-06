// ─────────────────────────────────────────────────────────
// FILE: ui/widgets/seq_display.cpp
// BRIEF: Step sequencer grid widget
// ─────────────────────────────────────────────────────────
#include "seq_display.h"
#include <string>

void SeqDisplay::draw(SeqStep steps[], int stepCount,
                       int activeStep, AtomicParamStore& /*store*/) {
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();

    const float cellW = 44.f;
    const float cellH = 60.f;
    const float pad   = 3.f;

    for (int i = 0; i < stepCount; ++i) {
        SeqStep& s  = steps[i];
        const float x = pos.x + i * (cellW + pad);
        const float y = pos.y;

        // Background
        const bool active = (i == activeStep);
        const ImU32 bg = active
            ? IM_COL32(200, 120, 20, 255)
            : (s.active
                ? IM_COL32(60, 60, 80, 255)
                : IM_COL32(30, 30, 40, 255));

        dl->AddRectFilled(ImVec2(x, y),
                          ImVec2(x + cellW, y + cellH),
                          bg, 4.f);
        dl->AddRect(ImVec2(x, y),
                    ImVec2(x + cellW, y + cellH),
                    IM_COL32(80, 80, 100, 255), 4.f);

        // Note name label
        const char* noteNames[] = {
            "C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        int octave = s.note / 12;
        int semi   = s.note % 12;
        char buf[8];
        snprintf(buf, sizeof(buf), "%s%d", noteNames[semi], octave - 1);
        dl->AddText(ImVec2(x + 4, y + 4),
                    IM_COL32(200, 200, 200, 255), buf);

        // Step number
        char stepBuf[4];
        snprintf(stepBuf, sizeof(stepBuf), "%d", i + 1);
        dl->AddText(ImVec2(x + 4, y + cellH - 16),
                    IM_COL32(120, 120, 140, 255), stepBuf);

        // Tie indicator
        if (s.tie) {
            dl->AddText(ImVec2(x + cellW - 14, y + 4),
                        IM_COL32(100, 200, 100, 255), "T");
        }

        // Invisible button for interaction
        ImGui::SetCursorScreenPos(ImVec2(x, y));
        ImGui::PushID(i);
        if (ImGui::InvisibleButton("##step", ImVec2(cellW, cellH))) {
            s.active = !s.active;
        }
        if (ImGui::IsItemHovered()) {
            // Right-click: toggle tie
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                s.tie = !s.tie;
            // Scroll wheel: change note (clamp to 0..127)
            const float wheel = ImGui::GetIO().MouseWheel;
            if (wheel != 0.f) {
                s.note = s.note + static_cast<int>(wheel);
                if (s.note < 0)   s.note = 0;
                if (s.note > 127) s.note = 127;
            }
        }
        ImGui::PopID();
    }

    ImGui::SetCursorScreenPos(
        ImVec2(pos.x, pos.y + cellH + 6));
    ImGui::Dummy(ImVec2(stepCount * (cellW + pad), 0));
}
