// ─────────────────────────────────────────────────────────
// FILE: ui/panels/engines/panel_engine_selector.cpp
// BRIEF: Engine selector + per-engine sub-panel dispatch
// ─────────────────────────────────────────────────────────
#include "panel_engine_selector.h"
#include "panel_moog.h"
#include "panel_hammond.h"
#include "panel_rhodes.h"
#include "panel_dx7.h"
#include "panel_mellotron.h"
#include "panel_drums.h"
#include "imgui.h"
#include <cstring>

namespace PanelEngineSelector {

void render(EngineManager& mgr) {
    if (!ImGui::Begin("Synth Engine")) { ImGui::End(); return; }

    const int count     = mgr.getEngineCount();
    const int activeIdx = mgr.getActiveIndex();

    // ── Engine RadioButton row ────────────────────────────
    ImGui::Text("SELECT ENGINE:");
    ImGui::Separator();

    for (int i = 0; i < count; ++i) {
        IEngine* eng = mgr.getEngine(i);
        if (!eng) continue;

        char label[64];
        snprintf(label, sizeof(label), "[%s] %s",
                 eng->getCategory(), eng->getName());

        bool selected = (i == activeIdx);
        if (ImGui::RadioButton(label, selected) && !selected)
            mgr.switchEngine(i);
    }

    ImGui::Separator();

    // ── Active engine sub-panel ───────────────────────────
    IEngine* active = mgr.getActiveEngine();
    if (!active) { ImGui::End(); return; }

    ImGui::Text("Active: %s", active->getName());

    ImGui::End();
}

void renderEngine(EngineManager& mgr) {
    IEngine* active = mgr.getActiveEngine();
    if (!active) return;

    char title[128];
    snprintf(title, sizeof(title), "Engine: %s", active->getName());
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title)) { ImGui::End(); return; }

    const char* name = active->getName();

    if (strcmp(name, "MiniMoog Model D") == 0) {
        if (auto* eng = dynamic_cast<MoogEngine*>(active))
            PanelEngine::renderContent(*eng);
    }
    else if (strcmp(name, "Hammond B-3") == 0) {
        if (auto* eng = dynamic_cast<HammondEngine*>(active))
            PanelHammond::renderContent(*eng);
    }
    else if (strcmp(name, "Rhodes Mark I") == 0) {
        if (auto* eng = dynamic_cast<RhodesEngine*>(active))
            PanelRhodes::renderContent(*eng);
    }
    else if (strcmp(name, "Yamaha DX7") == 0) {
        if (auto* eng = dynamic_cast<DX7Engine*>(active))
            PanelDX7::renderContent(*eng);
    }
    else if (strcmp(name, "Mellotron M400") == 0) {
        if (auto* eng = dynamic_cast<MellotronEngine*>(active))
            PanelMellotron::renderContent(*eng);
    }
    else if (strcmp(name, "Hybrid Drum Machine") == 0) {
        if (auto* eng = dynamic_cast<DrumEngine*>(active))
            PanelDrums::renderContent(*eng);
    }
    else {
        ImGui::Text("Engine: %s", name);
        ImGui::Separator();
        for (int i = 0; i < active->getParamCount(); ++i) {
            float v = active->getParam(i);
            ImGui::SetNextItemWidth(200);
            char id[32]; snprintf(id, sizeof(id), "##ep%d", i);
            if (ImGui::SliderFloat(id, &v,
                    active->getParamMin(i), active->getParamMax(i),
                    active->getParamName(i)))
                active->setParam(i, v);
        }
    }

    ImGui::End();
}

} // namespace PanelEngineSelector
