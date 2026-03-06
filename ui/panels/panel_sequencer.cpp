// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_sequencer.cpp
// BRIEF: Step sequencer panel
// ─────────────────────────────────────────────────────────
#include "panel_sequencer.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "ui/widgets/seq_display.h"
#include "shared/params.h"
#include <array>
#include <cstring>

namespace PanelSequencer {

// Local step cache for GUI — mirrors engine's seq_ steps
static std::array<SeqStep, StepSequencer::MAX_STEPS> s_steps;
static bool s_initialized = false;

void render(AtomicParamStore& p, SynthEngine& engine) {
    // ── Init: pull step data from engine on first render ──
    if (!s_initialized) {
        for (int i = 0; i < StepSequencer::MAX_STEPS; ++i)
            s_steps[i] = engine.getSeqStep(i);
        s_initialized = true;
    }

    ImGui::Begin("Sequencer");

    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "STEP SEQUENCER");
    ImGui::Separator();

    bool seqOn = p.get(P_SEQ_ON) > 0.5f;
    if (ImGui::Checkbox("Enable", &seqOn))
        p.set(P_SEQ_ON, seqOn ? 1.f : 0.f);

    ImGui::SameLine(0.f, 20.f);

    bool playing = p.get(P_SEQ_PLAYING) > 0.5f;
    if (ImGui::Button(playing ? "Stop##seq" : "Play##seq"))
        p.set(P_SEQ_PLAYING, playing ? 0.f : 1.f);

    ImGui::SameLine(0.f, 20.f);
    if (ImGui::Button("Clear##seq")) {
        for (int i = 0; i < StepSequencer::MAX_STEPS; ++i) {
            s_steps[i].active = false;
            engine.setSeqStep(i, s_steps[i]);
        }
    }

    ImGui::BeginDisabled(!seqOn);

    int steps = static_cast<int>(p.get(P_SEQ_STEPS));
    ImGui::SetNextItemWidth(120.f);
    if (ImGui::SliderInt("Steps", &steps, 1, StepSequencer::MAX_STEPS))
        p.set(P_SEQ_STEPS, static_cast<float>(steps));

    const char* rates[] = {"1/1","1/2","1/4","1/8","1/16","3/8","3/16","1/4T"};
    int rate = static_cast<int>(p.get(P_SEQ_RATE));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("Rate##seq", &rate, rates, 8))
        p.set(P_SEQ_RATE, static_cast<float>(rate));

    float gate = p.get(P_SEQ_GATE);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Gate##seq", &gate, 0.01f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 45.f))
        p.set(P_SEQ_GATE, gate);

    float swing = p.get(P_SEQ_SWING);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Swing##seq", &swing, 0.f, 0.5f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 45.f))
        p.set(P_SEQ_SWING, swing);

    ImGui::Spacing();
    ImGui::TextDisabled("Left-click = toggle on/off | Right-click = tie | Scroll = change note");
    ImGui::Spacing();

    // ── Step grid — capture copy, draw, detect changes ────
    const std::array<SeqStep, StepSequencer::MAX_STEPS> before = s_steps;
    const int activeStep = engine.getSeqStep();
    SeqDisplay::draw(s_steps.data(), steps, activeStep, p);

    // Push any changed steps to the engine
    for (int i = 0; i < steps; ++i) {
        if (std::memcmp(&s_steps[i], &before[i], sizeof(SeqStep)) != 0)
            engine.setSeqStep(i, s_steps[i]);
    }

    ImGui::EndDisabled();
    ImGui::End();
}

} // namespace PanelSequencer
