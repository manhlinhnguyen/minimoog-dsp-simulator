// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_drums.cpp
// BRIEF: Drum machine panel — 16 pads grid
// ─────────────────────────────────────────────────────────
#include "panel_drums.h"
#include "imgui.h"
#include <cstdio>

namespace PanelDrums {

static void renderPadRow(DrumEngine& engine, int padIdx,
                          const char* name, bool isSample) {
    ImGui::PushID(padIdx);

    // Pad trigger button
    ImVec4 btnCol = isSample
        ? ImVec4(0.1f,0.3f,0.5f,1)
        : ImVec4(0.4f,0.1f,0.1f,1);
    ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
    char btnLabel[32];
    snprintf(btnLabel, sizeof(btnLabel), "[%s]", name);
    if (ImGui::Button(btnLabel, ImVec2(90, 30))) {
        // Trigger pad via direct engine call (UI thread noteOn for testing)
        // In actual use, this goes through the MIDI queue
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();

    // 4 param sliders
    static const char* pnames[] = {"Vol","Pitch","Decay","Pan"};
    for (int p = 0; p < 4; ++p) {
        const int id = drumParamId(padIdx, static_cast<DrumParamType>(p));
        float v = engine.getParam(id);
        char slid[16]; snprintf(slid, sizeof(slid), "##p%d_%d", padIdx, p);
        ImGui::SetNextItemWidth(70);
        if (ImGui::SliderFloat(slid, &v, 0.0f, 1.0f, pnames[p]))
            engine.setParam(id, v);
        ImGui::SameLine();
    }

    if (isSample) {
        const std::string& sname = engine.getSampleName(padIdx - DSP_PADS);
        ImGui::TextDisabled("[%s]", sname.empty() ? "no sample" : sname.c_str());
    }

    ImGui::PopID();
}

void renderContent(DrumEngine& engine) {
    // Global volume
    float gvol = engine.getParam(DRUM_GLOBAL_VOL_ID);
    ImGui::SetNextItemWidth(160);
    if (ImGui::SliderFloat("Global Volume", &gvol, 0.0f, 1.0f))
        engine.setParam(DRUM_GLOBAL_VOL_ID, gvol);

    ImGui::SameLine();

    // Kick sweep params
    float sd = engine.getParam(DRUM_KICK_SWEEP_DEPTH_ID);
    ImGui::SetNextItemWidth(120);
    if (ImGui::SliderFloat("Kick Sweep##sd", &sd, 0.0f, 1.0f, "D:%.2f"))
        engine.setParam(DRUM_KICK_SWEEP_DEPTH_ID, sd);
    ImGui::SameLine();
    float st = engine.getParam(DRUM_KICK_SWEEP_TIME_ID);
    ImGui::SetNextItemWidth(120);
    if (ImGui::SliderFloat("Sweep Time##st", &st, 0.0f, 1.0f, "T:%.2f"))
        engine.setParam(DRUM_KICK_SWEEP_TIME_ID, st);

    ImGui::Separator();

    // DSP pads (0-7)
    ImGui::Text("DSP SYNTHESIS PADS");
    ImGui::Separator();
    for (int i = 0; i < DSP_PADS; ++i)
        renderPadRow(engine, i, DSP_PAD_NAMES[i], false);

    ImGui::Separator();

    // Sample pads (8-15)
    ImGui::Text("SAMPLE PADS");
    ImGui::Separator();
    static const char* sampleLabels[SAMPLE_PADS] = {
        "Pad 9","Pad10","Pad11","Pad12","Pad13","Pad14","Pad15","Pad16"
    };
    for (int i = 0; i < SAMPLE_PADS; ++i)
        renderPadRow(engine, DSP_PADS + i, sampleLabels[i], true);

    ImGui::Text("Active voices: %d", engine.getActiveVoices());
}

void render(DrumEngine& engine) {
    if (!ImGui::Begin("Drum Machine")) { ImGui::End(); return; }
    renderContent(engine);
    ImGui::End();
}

} // namespace PanelDrums
