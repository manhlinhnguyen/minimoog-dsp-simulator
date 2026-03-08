// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_mellotron.cpp
// BRIEF: Mellotron M400 panel implementation
// ─────────────────────────────────────────────────────────
#include "panel_mellotron.h"
#include "imgui.h"

namespace PanelMellotron {

void renderContent(MellotronEngine& engine) {
    // ── Tape selector ─────────────────────────────────────
    ImGui::Text("TAPE");
    ImGui::Separator();
    static const char* tapeNames[] = { "Strings", "Choir", "Flute", "Brass" };
    int tape = static_cast<int>(engine.getParam(MP_TAPE));
    for (int i = 0; i < 4; ++i) {
        if (ImGui::RadioButton(tapeNames[i], tape == i)) {
            engine.setParam(MP_TAPE, static_cast<float>(i));
        }
        if (i < 3) ImGui::SameLine();
    }

    ImGui::Separator();
    ImGui::Text("CONTROLS");

    auto slider = [&](const char* label, int id) {
        float v = engine.getParam(id);
        ImGui::SetNextItemWidth(200);
        if (ImGui::SliderFloat(label, &v,
            engine.getParamMin(id), engine.getParamMax(id)))
            engine.setParam(id, v);
    };

    slider("Volume",       MP_VOLUME);
    slider("Pitch Spread", MP_PITCH_SPREAD);
    slider("Tape Speed",   MP_TAPE_SPEED);
    slider("Runout Time",  MP_RUNOUT_TIME);
    slider("Attack",       MP_ATTACK);
    slider("Release",      MP_RELEASE);

    ImGui::Separator();
    ImGui::Text("TAPE MOD");
    slider("Wow Depth",    MP_WOW_DEPTH);
    slider("Wow Rate",     MP_WOW_RATE);
    slider("Flutter Depth",MP_FLUTTER_DEPTH);
    slider("Flutter Rate", MP_FLUTTER_RATE);

    ImGui::Text("Active voices: %d", engine.getActiveVoices());
}

void render(MellotronEngine& engine) {
    if (!ImGui::Begin("Mellotron M400")) { ImGui::End(); return; }
    renderContent(engine);
    ImGui::End();
}

} // namespace PanelMellotron
