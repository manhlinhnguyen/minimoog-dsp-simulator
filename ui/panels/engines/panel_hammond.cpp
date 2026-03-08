// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_hammond.cpp
// BRIEF: Hammond B-3 panel — drawbars + percussion + vib
// ─────────────────────────────────────────────────────────
#include "panel_hammond.h"
#include "imgui.h"
#include <cmath>

namespace PanelHammond {

// Drawbar colours (classic Hammond colours)
static const ImVec4 DRAWBAR_COLORS[9] = {
    {0.8f,0.8f,0.8f,1}, // 16'  Brown → white
    {0.5f,0.3f,0.1f,1}, // 5⅓' Brown
    {0.8f,0.8f,0.8f,1}, // 8'  White
    {0.2f,0.2f,0.2f,1}, // 4'  Black (top)
    {0.5f,0.3f,0.1f,1}, // 2⅔' Brown
    {0.2f,0.2f,0.2f,1}, // 2'  Black
    {0.2f,0.2f,0.2f,1}, // 1⅗' Black
    {0.5f,0.3f,0.1f,1}, // 1⅓' Brown
    {0.2f,0.2f,0.2f,1}, // 1'  Black
};

// Vertical drawbar slider (reversed: top=max, bottom=0)
static bool DrawbarSlider(const char* id, float* val, ImVec4 col) {
    ImGui::PushStyleColor(ImGuiCol_SliderGrab,       col);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, col);
    const bool changed = ImGui::VSliderFloat(id, ImVec2(28, 120), val, 0.0f, 8.0f, "");
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%.0f", *val);
    return changed;
}

void renderContent(HammondEngine& engine) {
    // ── Drawbars ──────────────────────────────────────────
    ImGui::Text("DRAWBARS");
    ImGui::Separator();

    static const char* labels[9] = {
        "16'","5⅓'","8'","4'","2⅔'","2'","1⅗'","1⅓'","1'"
    };

    for (int i = 0; i < 9; ++i) {
        float v = engine.getParam(HP_DRAWBAR_1 + i);
        char id[16];
        snprintf(id, sizeof(id), "##db%d", i);
        if (DrawbarSlider(id, &v, DRAWBAR_COLORS[i]))
            engine.setParam(HP_DRAWBAR_1 + i, v);
        ImGui::SameLine();
    }
    ImGui::NewLine();

    // Labels below drawbars
    for (int i = 0; i < 9; ++i) {
        ImGui::Text("%s", labels[i]);
        if (i < 8) ImGui::SameLine(0, 12);
    }

    ImGui::Separator();

    // ── Percussion ────────────────────────────────────────
    ImGui::Text("PERCUSSION");
    {
        bool percOn = engine.getParam(HP_PERC_ON) > 0.5f;
        if (ImGui::Checkbox("Enable##perc", &percOn))
            engine.setParam(HP_PERC_ON, percOn ? 1.0f : 0.0f);

        ImGui::SameLine();
        int harm = static_cast<int>(engine.getParam(HP_PERC_HARM));
        if (ImGui::RadioButton("2nd", harm == 0)) engine.setParam(HP_PERC_HARM, 0.0f);
        ImGui::SameLine();
        if (ImGui::RadioButton("3rd", harm == 1)) engine.setParam(HP_PERC_HARM, 1.0f);

        float percDecay = engine.getParam(HP_PERC_DECAY);
        ImGui::SameLine();
        if (ImGui::RadioButton("Fast", percDecay < 0.5f)) engine.setParam(HP_PERC_DECAY, 0.0f);
        ImGui::SameLine();
        if (ImGui::RadioButton("Slow", percDecay >= 0.5f)) engine.setParam(HP_PERC_DECAY, 1.0f);

        float percLevel = engine.getParam(HP_PERC_LEVEL);
        ImGui::SetNextItemWidth(160);
        if (ImGui::SliderFloat("Level##perc", &percLevel, 0.0f, 1.0f))
            engine.setParam(HP_PERC_LEVEL, percLevel);
    }

    ImGui::Separator();

    // ── Vibrato/Chorus ────────────────────────────────────
    ImGui::Text("VIBRATO / CHORUS");
    {
        static const char* vibModes[] = {"Off","V1","V2","V3","C1","C2","C3"};
        int vibMode = static_cast<int>(engine.getParam(HP_VIB_MODE));
        ImGui::SetNextItemWidth(100);
        if (ImGui::Combo("Mode##vib", &vibMode, vibModes, 7))
            engine.setParam(HP_VIB_MODE, static_cast<float>(vibMode));

        float vibDepth = engine.getParam(HP_VIB_DEPTH);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        if (ImGui::SliderFloat("Depth##vib", &vibDepth, 0.0f, 1.0f))
            engine.setParam(HP_VIB_DEPTH, vibDepth);
    }

    ImGui::Separator();

    // ── Leslie (dual rotor) ───────────────────────────────
    ImGui::Text("LESLIE ROTARY");
    {
        bool leslieOn = engine.getParam(HP_LESLIE_ON) > 0.5f;
        if (ImGui::Checkbox("Enable Leslie", &leslieOn))
            engine.setParam(HP_LESLIE_ON, leslieOn ? 1.0f : 0.0f);

        bool fast = engine.getParam(HP_LESLIE_SPEED) > 0.5f;
        ImGui::SameLine();
        if (ImGui::Checkbox("Fast", &fast))
            engine.setParam(HP_LESLIE_SPEED, fast ? 1.0f : 0.0f);

        float mix = engine.getParam(HP_LESLIE_MIX);
        ImGui::SetNextItemWidth(160);
        if (ImGui::SliderFloat("Mix##leslie", &mix, 0.0f, 1.0f))
            engine.setParam(HP_LESLIE_MIX, mix);

        float spread = engine.getParam(HP_LESLIE_SPREAD);
        ImGui::SetNextItemWidth(160);
        if (ImGui::SliderFloat("Spread##leslie", &spread, 0.0f, 1.0f))
            engine.setParam(HP_LESLIE_SPREAD, spread);
    }

    ImGui::Separator();

    // ── Extras ────────────────────────────────────────────
    {
        float click = engine.getParam(HP_CLICK_LEVEL);
        ImGui::SetNextItemWidth(160);
        if (ImGui::SliderFloat("Key Click", &click, 0.0f, 1.0f))
            engine.setParam(HP_CLICK_LEVEL, click);

        float od = engine.getParam(HP_OVERDRIVE);
        ImGui::SetNextItemWidth(160);
        if (ImGui::SliderFloat("Overdrive", &od, 0.0f, 1.0f))
            engine.setParam(HP_OVERDRIVE, od);

        float vol = engine.getParam(HP_MASTER_VOL);
        ImGui::SetNextItemWidth(160);
        if (ImGui::SliderFloat("Volume##hmm", &vol, 0.0f, 1.0f))
            engine.setParam(HP_MASTER_VOL, vol);
    }

    ImGui::Text("Active voices: %d", engine.getActiveVoices());
}

void render(HammondEngine& engine) {
    if (!ImGui::Begin("Hammond B-3")) { ImGui::End(); return; }
    renderContent(engine);
    ImGui::End();
}

} // namespace PanelHammond
