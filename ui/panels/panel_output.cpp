// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_output.cpp
// BRIEF: Master output panel
// ─────────────────────────────────────────────────────────
#include "panel_output.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "shared/params.h"

namespace PanelOutput {

void render(AtomicParamStore& p) {
    ImGui::Begin("Output");

    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MASTER");
    ImGui::Separator();

    float vol = p.get(P_MASTER_VOL);
    if (ImGuiKnobs::Knob("Volume", &vol, 0.f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 65.f))
        p.set(P_MASTER_VOL, vol);

    ImGui::End();
}

} // namespace PanelOutput
