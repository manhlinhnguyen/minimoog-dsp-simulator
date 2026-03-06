// ─────────────────────────────────────────────────────────
// FILE: ui/widgets/keyboard_display.cpp
// BRIEF: 2-octave piano keyboard widget
// ─────────────────────────────────────────────────────────
#include "keyboard_display.h"
#include <string>

void KeyboardDisplay::draw(const bool* activeNotes, int baseNote, ImVec2 size,
                            std::function<void(int, bool)> onKey) {
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float W    = size.x;
    const float H    = size.y;

    constexpr int WHITE_COUNT = 14;
    const float wW = W / WHITE_COUNT;
    const float bW = wW * 0.6f;
    const float bH = H * 0.6f;

    constexpr int WHITE_SEMI[7] = {0, 2, 4, 5, 7, 9, 11};
    constexpr int BLACK_SEMI[5] = {1, 3, 6, 8, 10};

    auto isLit = [&](int note) -> bool {
        return activeNotes && note >= 0 && note < 128 && activeNotes[note];
    };

    // White keys
    for (int oct = 0; oct < 2; ++oct) {
        for (int k = 0; k < 7; ++k) {
            const int   idx  = oct * 7 + k;
            const int   note = baseNote + oct * 12 + WHITE_SEMI[k];
            const float x    = pos.x + idx * wW;
            const float y    = pos.y;
            const ImU32 fill = isLit(note)
                ? IM_COL32(240, 160, 40, 255)   // pressed = orange
                : IM_COL32(230, 230, 230, 255);  // normal  = white
            dl->AddRectFilled(ImVec2(x + 1, y),
                              ImVec2(x + wW - 1, y + H),
                              fill, 2.f);
            dl->AddRect(ImVec2(x + 1, y),
                        ImVec2(x + wW - 1, y + H),
                        IM_COL32(60, 60, 60, 255), 2.f);

            ImGui::SetCursorScreenPos(ImVec2(x + 1, y));
            ImGui::InvisibleButton(("wk" + std::to_string(note)).c_str(),
                                   ImVec2(wW - 2, H));
            if (onKey) {
                if (ImGui::IsItemActivated())   onKey(note, true);
                if (ImGui::IsItemDeactivated()) onKey(note, false);
            }
        }
    }

    // Black keys (drawn on top)
    for (int oct = 0; oct < 2; ++oct) {
        int bIdx = 0;
        for (int k = 0; k < 5; ++k) {
            if (k == 2) bIdx = 3;
            else        bIdx = (k < 2) ? k : k + 1;

            const int   note = baseNote + oct * 12 + BLACK_SEMI[k];
            const float x    = pos.x + (oct * 7 + bIdx) * wW + wW - bW * 0.5f;
            const float y    = pos.y;
            const ImU32 fill = isLit(note)
                ? IM_COL32(200, 120, 20, 255)   // pressed = dark orange
                : IM_COL32(30, 30, 30, 255);     // normal  = black
            dl->AddRectFilled(ImVec2(x, y),
                              ImVec2(x + bW, y + bH),
                              fill, 2.f);

            ImGui::SetCursorScreenPos(ImVec2(x, y));
            ImGui::InvisibleButton(("bk" + std::to_string(note)).c_str(),
                                   ImVec2(bW, bH));
            if (onKey) {
                if (ImGui::IsItemActivated())   onKey(note, true);
                if (ImGui::IsItemDeactivated()) onKey(note, false);
            }
        }
    }

    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + H + 2));
    ImGui::Dummy(ImVec2(W, 0));
}
