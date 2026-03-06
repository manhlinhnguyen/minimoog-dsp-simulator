// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_modifiers.cpp
// BRIEF: Filter & Envelopes panel
// ─────────────────────────────────────────────────────────
#include "panel_modifiers.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "ui/widgets/adsr_display.h"
#include "shared/params.h"

namespace PanelModifiers {

static void renderADSRKnobs(AtomicParamStore& p,
                             int A, int D, int S, int R,
                             const char* label) {
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "%s", label);
    ImGui::Separator();

    float a = p.get(A), d = p.get(D),
          s = p.get(S), r = p.get(R);

    ImGui::PushID(label);
    if (ImGuiKnobs::Knob("A",&a,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,50.f)) p.set(A,a);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("D",&d,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,50.f)) p.set(D,d);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("S",&s,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,50.f)) p.set(S,s);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("R",&r,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,50.f)) p.set(R,r);

    AdsrDisplay::draw(a, d, s, r, ImVec2(220.f, 60.f));
    ImGui::PopID();
}

void render(AtomicParamStore& params) {
    ImGui::Begin("Filter & Envelopes");

    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MOOG FILTER");
    ImGui::Separator();

    float cutoff = params.get(P_FILTER_CUTOFF);
    float res    = params.get(P_FILTER_EMPHASIS);
    float amt    = params.get(P_FILTER_AMOUNT);

    if (ImGuiKnobs::Knob("Cutoff",&cutoff,0.f,1.f,0.005f,
            "%.2f",ImGuiKnobVariant_Wiper,55.f))
        params.set(P_FILTER_CUTOFF, cutoff);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Emphasis",&res,0.f,1.f,0.005f,
            "%.2f",ImGuiKnobVariant_Wiper,55.f))
        params.set(P_FILTER_EMPHASIS, res);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Env Amt",&amt,0.f,1.f,0.005f,
            "%.2f",ImGuiKnobVariant_Wiper,55.f))
        params.set(P_FILTER_AMOUNT, amt);

    const char* kbdModes[] = {"Off","1/3","2/3"};
    int kbd = static_cast<int>(params.get(P_FILTER_KBD_TRACK));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("KBD Track", &kbd, kbdModes, 3))
        params.set(P_FILTER_KBD_TRACK, static_cast<float>(kbd));

    ImGui::Spacing();
    renderADSRKnobs(params,
        P_FENV_ATTACK, P_FENV_DECAY,
        P_FENV_SUSTAIN, P_FENV_RELEASE, "FILTER ENV");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    renderADSRKnobs(params,
        P_AENV_ATTACK, P_AENV_DECAY,
        P_AENV_SUSTAIN, P_AENV_RELEASE, "AMP ENV");

    ImGui::End();
}

} // namespace PanelModifiers
