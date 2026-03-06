// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_controllers.cpp
// BRIEF: Controllers panel (Glide, Mod, BPM, Polyphony)
// ─────────────────────────────────────────────────────────
#include "panel_controllers.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "shared/params.h"

namespace PanelControllers {

void render(AtomicParamStore& p) {
    ImGui::Begin("Controllers");

    // ── Glide ─────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "GLIDE");
    ImGui::Separator();

    bool glideOn = p.get(P_GLIDE_ON) > 0.5f;
    if (ImGui::Checkbox("Glide On", &glideOn))
        p.set(P_GLIDE_ON, glideOn ? 1.f : 0.f);

    float glideTime = p.get(P_GLIDE_TIME);
    ImGui::SetNextItemWidth(120.f);
    if (ImGui::SliderFloat("Glide Time", &glideTime, 0.f, 1.f, "%.2f"))
        p.set(P_GLIDE_TIME, glideTime);

    ImGui::Spacing();

    // ── Modulation ────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MODULATION");
    ImGui::Separator();

    float modMix = p.get(P_MOD_MIX);
    if (ImGuiKnobs::Knob("Mod Mix", &modMix, 0.f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 55.f))
        p.set(P_MOD_MIX, modMix);

    ImGui::SameLine();
    bool oscMod = p.get(P_OSC_MOD_ON) > 0.5f;
    if (ImGui::Checkbox("OSC Mod", &oscMod))
        p.set(P_OSC_MOD_ON, oscMod ? 1.f : 0.f);

    bool filterMod = p.get(P_FILTER_MOD_ON) > 0.5f;
    if (ImGui::Checkbox("Filter Mod", &filterMod))
        p.set(P_FILTER_MOD_ON, filterMod ? 1.f : 0.f);

    ImGui::Spacing();

    // ── BPM ───────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "TEMPO");
    ImGui::Separator();

    float bpm = p.get(P_BPM);
    ImGui::SetNextItemWidth(150.f);
    if (ImGui::SliderFloat("BPM", &bpm, 20.f, 300.f, "%.0f"))
        p.set(P_BPM, bpm);

    ImGui::Spacing();

    // ── Polyphony ─────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "POLYPHONY");
    ImGui::Separator();

    const char* modes[] = {"Mono", "Poly", "Unison"};
    int voiceMode = static_cast<int>(p.get(P_VOICE_MODE));
    ImGui::SetNextItemWidth(100.f);
    if (ImGui::Combo("Mode", &voiceMode, modes, 3))
        p.set(P_VOICE_MODE, static_cast<float>(voiceMode));

    int voiceCount = static_cast<int>(p.get(P_VOICE_COUNT));
    ImGui::SetNextItemWidth(100.f);
    if (ImGui::SliderInt("Voices", &voiceCount, 1, 8))
        p.set(P_VOICE_COUNT, static_cast<float>(voiceCount));

    const char* stealModes[] = {"Oldest", "Lowest", "Quietest"};
    int stealMode = static_cast<int>(p.get(P_VOICE_STEAL));
    ImGui::SetNextItemWidth(100.f);
    if (ImGui::Combo("Steal", &stealMode, stealModes, 3))
        p.set(P_VOICE_STEAL, static_cast<float>(stealMode));

    float detune = p.get(P_UNISON_DETUNE);
    ImGui::SetNextItemWidth(120.f);
    if (ImGui::SliderFloat("Detune", &detune, 0.f, 1.f, "%.2f"))
        p.set(P_UNISON_DETUNE, detune);

    ImGui::End();
}

} // namespace PanelControllers
