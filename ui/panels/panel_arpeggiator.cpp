// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_arpeggiator.cpp
// BRIEF: Arpeggiator panel
// ─────────────────────────────────────────────────────────
#include "panel_arpeggiator.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "shared/params.h"

namespace PanelArpeggiator {

void render(AtomicParamStore& p) {
    ImGui::Begin("Arpeggiator");

    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "ARPEGGIATOR");
    ImGui::Separator();

    bool arpOn = p.get(P_ARP_ON) > 0.5f;
    if (ImGui::Checkbox("Enable", &arpOn))
        p.set(P_ARP_ON, arpOn ? 1.f : 0.f);

    ImGui::BeginDisabled(!arpOn);

    const char* modes[] = {"Up","Down","Up/Down","Down/Up","Random","As Played"};
    int mode = static_cast<int>(p.get(P_ARP_MODE));
    ImGui::SetNextItemWidth(110.f);
    if (ImGui::Combo("Mode", &mode, modes, 6))
        p.set(P_ARP_MODE, static_cast<float>(mode));

    int octaves = static_cast<int>(p.get(P_ARP_OCTAVES));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::SliderInt("Octaves", &octaves, 1, 4))
        p.set(P_ARP_OCTAVES, static_cast<float>(octaves));

    const char* rates[] = {"1/1","1/2","1/4","1/8","1/16","3/8","3/16","1/12"};
    int rate = static_cast<int>(p.get(P_ARP_RATE));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("Rate", &rate, rates, 8))
        p.set(P_ARP_RATE, static_cast<float>(rate));

    float gate = p.get(P_ARP_GATE);
    if (ImGuiKnobs::Knob("Gate", &gate, 0.01f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 50.f))
        p.set(P_ARP_GATE, gate);

    ImGui::SameLine();

    float swing = p.get(P_ARP_SWING);
    if (ImGuiKnobs::Knob("Swing", &swing, 0.f, 0.5f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 50.f))
        p.set(P_ARP_SWING, swing);

    ImGui::EndDisabled();
    ImGui::End();
}

} // namespace PanelArpeggiator
