// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_rhodes.cpp
// BRIEF: Rhodes Mark I panel — Modal Resonator engine
// ─────────────────────────────────────────────────────────
#include "panel_rhodes.h"
#include "imgui.h"

namespace PanelRhodes {

void renderContent(RhodesEngine& engine) {
    auto slider = [&](const char* label, int id) {
        float v = engine.getParam(id);
        ImGui::SetNextItemWidth(220);
        if (ImGui::SliderFloat(label, &v,
                engine.getParamMin(id),
                engine.getParamMax(id)))
            engine.setParam(id, v);
    };

    // ── TINE ─────────────────────────────────────────────
    ImGui::Text("TINE");
    ImGui::Separator();
    slider("Decay",        RP_DECAY);
    ImGui::SameLine(); ImGui::TextDisabled("(sustain length)");

    slider("Tone",         RP_TONE);
    ImGui::SameLine(); ImGui::TextDisabled("(pickup brightness)");

    slider("Vel Sens",     RP_VEL_SENS);
    slider("Stereo Spread",RP_STEREO_SPREAD);
    slider("Release (ms)", RP_RELEASE);

    // ── TREMOLO ──────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Text("TREMOLO (Suitcase)");
    ImGui::Separator();
    slider("Rate (Hz)",    RP_TREMOLO_RATE);
    slider("Depth",        RP_TREMOLO_DEPTH);

    // ── VIBRATO ──────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Text("VIBRATO");
    ImGui::Separator();
    slider("Rate (Hz)##vib", RP_VIBRATO_RATE);
    slider("Depth##vib",     RP_VIBRATO_DEPTH);

    // ── OUTPUT ───────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Text("OUTPUT");
    ImGui::Separator();
    slider("Drive",        RP_DRIVE);
    slider("Master Volume",RP_MASTER_VOL);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Active voices: %d", engine.getActiveVoices());
}

void render(RhodesEngine& engine) {
    if (!ImGui::Begin("Rhodes Mark I")) { ImGui::End(); return; }
    renderContent(engine);
    ImGui::End();
}

} // namespace PanelRhodes
