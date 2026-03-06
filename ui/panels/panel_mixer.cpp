// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_mixer.cpp
// BRIEF: Mixer panel (OSC1/2/3 levels, Noise)
// ─────────────────────────────────────────────────────────
#include "panel_mixer.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "shared/params.h"

namespace PanelMixer {

void render(AtomicParamStore& p) {
    ImGui::Begin("Mixer");

    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MIXER");
    ImGui::Separator();

    struct { const char* label; int id; } knobs[] = {
        {"OSC 1", P_MIX_OSC1},
        {"OSC 2", P_MIX_OSC2},
        {"OSC 3", P_MIX_OSC3},
        {"Noise", P_MIX_NOISE},
    };

    for (auto& k : knobs) {
        float v = p.get(k.id);
        if (ImGuiKnobs::Knob(k.label, &v, 0.f, 1.f, 0.005f,
                              "%.2f", ImGuiKnobVariant_Wiper, 55.f))
            p.set(k.id, v);
        ImGui::SameLine(0.f, 10.f);
    }
    ImGui::NewLine();

    ImGui::Spacing();
    const char* colors[] = {"White", "Pink"};
    int color = static_cast<int>(p.get(P_NOISE_COLOR));
    ImGui::SetNextItemWidth(100.f);
    if (ImGui::Combo("Noise Color", &color, colors, 2))
        p.set(P_NOISE_COLOR, static_cast<float>(color));

    ImGui::End();
}

} // namespace PanelMixer
