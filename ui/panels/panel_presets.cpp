// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_presets.cpp
// BRIEF: Preset browser, save/load
// ─────────────────────────────────────────────────────────
#include "panel_presets.h"
#include "imgui.h"
#include "shared/params.h"
#include <vector>
#include <string>
#include <cstring>

namespace PanelPresets {

static char s_saveName[128] = "MyPreset";
static int  s_selected      = -1;
static std::vector<std::string> s_list;

void render(AtomicParamStore& p, SynthEngine& /*engine*/,
            PresetStorage& storage) {
    ImGui::Begin("Presets");

    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "PRESETS");
    ImGui::Separator();

    // Reset all params to factory defaults
    if (ImGui::Button("Reset to Defaults")) {
        p.resetToDefaults();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Resets ALL parameters to factory default values");

    ImGui::SameLine();

    // Refresh button
    if (ImGui::Button("Refresh")) {
        s_list    = storage.listPresets();
        s_selected = -1;
    }
    ImGui::SameLine();

    // Save current
    ImGui::SetNextItemWidth(130.f);
    ImGui::InputText("##name", s_saveName, sizeof(s_saveName));
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        const std::string fn = std::string(s_saveName) + ".json";
        Preset preset;
        // Build a minimal preset from params only
        p.snapshot(preset.params);
        preset.name     = s_saveName;
        preset.category = "User";
        storage.savePreset(preset, fn);
        s_list = storage.listPresets();
    }

    ImGui::Spacing();

    // Preset list
    if (s_list.empty()) s_list = storage.listPresets();

    ImGui::BeginChild("##presetlist", ImVec2(0.f, 200.f), true);
    for (int i = 0; i < static_cast<int>(s_list.size()); ++i) {
        const bool sel = (i == s_selected);
        if (ImGui::Selectable(s_list[i].c_str(), sel))
            s_selected = i;
    }
    ImGui::EndChild();

    ImGui::BeginDisabled(s_selected < 0 ||
                          s_selected >= static_cast<int>(s_list.size()));
    if (ImGui::Button("Load")) {
        Preset preset;
        if (storage.loadPreset(s_list[s_selected], preset)) {
            for (int i = 0; i < PARAM_COUNT; ++i)
                p.set(i, preset.params[i]);
        }
    }
    ImGui::EndDisabled();

    if (!storage.getLastError().empty()) {
        ImGui::TextColored(ImVec4(1.f,0.3f,0.3f,1.f),
                           "%s", storage.getLastError().c_str());
    }

    ImGui::End();
}

} // namespace PanelPresets
