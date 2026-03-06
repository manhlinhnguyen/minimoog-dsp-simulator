// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_oscillators.cpp
// BRIEF: Oscillator panel (OSC 1/2/3)
// ─────────────────────────────────────────────────────────
#include "panel_oscillators.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "shared/params.h"

namespace PanelOscillators {

static void renderOscStrip(AtomicParamStore& p,
                            int oscIdx, const char* label) {
    const int base = P_OSC1_ON + oscIdx * 4;
    const int ON  = base;
    const int RNG = base + 1;
    const int FRQ = base + 2;
    const int WAV = base + 3;

    ImGui::PushID(oscIdx);
    ImGui::BeginGroup();
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "%s", label);
    ImGui::Separator();

    bool on = p.get(ON) > 0.5f;
    if (ImGui::Checkbox("ON", &on))
        p.set(ON, on ? 1.f : 0.f);

    const char* ranges[] = {"LO","32'","16'","8'","4'","2'"};
    int rng = static_cast<int>(p.get(RNG));
    ImGui::SetNextItemWidth(70.f);
    if (ImGui::Combo("Range", &rng, ranges, 6))
        p.set(RNG, static_cast<float>(rng));

    const char* waves[] = {"Tri","TriSaw","RevSaw","Saw","Square","Pulse"};
    int wav = static_cast<int>(p.get(WAV));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("Wave", &wav, waves, 6))
        p.set(WAV, static_cast<float>(wav));

    float frq = p.get(FRQ);
    if (ImGuiKnobs::Knob("Freq", &frq, 0.f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 55.f))
        p.set(FRQ, frq);

    if (oscIdx == 2) {
        ImGui::Separator();
        bool lfoMode = p.get(P_OSC3_LFO_ON) > 0.5f;
        if (ImGui::Checkbox("LFO Mode", &lfoMode))
            p.set(P_OSC3_LFO_ON, lfoMode ? 1.f : 0.f);
    }

    ImGui::EndGroup();
    ImGui::PopID();
}

void render(AtomicParamStore& params) {
    ImGui::Begin("Oscillators");

    renderOscStrip(params, 0, "OSC 1");
    ImGui::SameLine(0.f, 20.f);
    renderOscStrip(params, 1, "OSC 2");
    ImGui::SameLine(0.f, 20.f);
    renderOscStrip(params, 2, "OSC 3");

    ImGui::Separator();
    float tune = params.get(P_MASTER_TUNE);
    ImGui::SetNextItemWidth(150.f);
    if (ImGui::SliderFloat("Master Tune", &tune, -1.f, 1.f))
        params.set(P_MASTER_TUNE, tune);

    ImGui::End();
}

} // namespace PanelOscillators
