// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_presets.cpp
// BRIEF: Combined preset browser — Moog Presets + Effect Presets tabs
// ─────────────────────────────────────────────────────────
#include "panel_presets.h"
#include "imgui.h"
#include "shared/params.h"
#include <vector>
#include <string>
#include <cstring>

namespace PanelPresets {

// ── Tab 1: Moog Presets ────────────────────────────────
static char s_moogSaveName[128] = "MyMoogPreset";
static int  s_moogSelected      = -1;
static std::vector<std::string> s_moogList;

static void renderMoogPresetsTab(AtomicParamStore& p, PresetStorage& storage) {
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MOOG PRESETS");
    ImGui::Separator();

    // Reset synth params to factory defaults (music params untouched)
    if (ImGui::Button("Reset")) {
        for (int k = 0; k < PARAM_COUNT; ++k)
            if (!isMusicParam(k))
                p.set(k, PARAM_META[k].defaultVal);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Resets synth parameters to factory defaults\n(Arp / Seq / Chord / Scale are not affected)");

    ImGui::SameLine();

    // Refresh
    if (ImGui::Button("Refresh")) {
        s_moogList     = storage.listPresets();
        s_moogSelected = -1;
    }
    ImGui::SameLine();

    // Save current
    ImGui::SetNextItemWidth(130.f);
    ImGui::InputText("##moogname", s_moogSaveName, sizeof(s_moogSaveName));
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        const std::string fn = std::string(s_moogSaveName) + ".json";
        Preset preset;
        p.snapshot(preset.params);
        preset.name     = s_moogSaveName;
        preset.category = "User";
        storage.savePreset(preset, fn);
        s_moogList = storage.listPresets();
    }

    ImGui::Spacing();

    if (s_moogList.empty()) s_moogList = storage.listPresets();

    ImGui::BeginChild("##mooglist", ImVec2(0.f, 200.f), true);
    for (int i = 0; i < static_cast<int>(s_moogList.size()); ++i) {
        const bool sel = (i == s_moogSelected);
        if (ImGui::Selectable(s_moogList[i].c_str(), sel,
                              ImGuiSelectableFlags_AllowDoubleClick)) {
            s_moogSelected = i;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                Preset preset;
                if (storage.loadPreset(s_moogList[i], preset)) {
                    for (int k = 0; k < PARAM_COUNT; ++k)
                        if (!isMusicParam(k))
                            p.set(k, preset.params[k]);
                }
            }
        }
    }
    ImGui::EndChild();

    ImGui::BeginDisabled(s_moogSelected < 0 ||
                          s_moogSelected >= static_cast<int>(s_moogList.size()));
    if (ImGui::Button("Load")) {
        Preset preset;
        if (storage.loadPreset(s_moogList[s_moogSelected], preset)) {
            for (int i = 0; i < PARAM_COUNT; ++i)
                if (!isMusicParam(i))
                    p.set(i, preset.params[i]);
        }
    }
    ImGui::EndDisabled();

    if (!storage.getLastError().empty())
        ImGui::TextColored(ImVec4(1.f,0.3f,0.3f,1.f),
                           "%s", storage.getLastError().c_str());
}

// ── Tab 2: Effect Presets ──────────────────────────────
static char s_fxSaveName[128] = "MyEffectPreset";
static int  s_fxSelected      = -1;
static std::vector<std::string> s_fxList;

static void renderEffectPresetsTab(EffectChain& chain,
                                    EffectPresetStorage& storage) {
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "EFFECT PRESETS");
    ImGui::Separator();

    // Reset — clear the entire effect chain
    if (ImGui::Button("Reset")) {
        EffectChainConfig empty{};
        chain.setConfig(empty);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Removes all effects from the chain");

    ImGui::SameLine();

    // Refresh
    if (ImGui::Button("Refresh")) {
        s_fxList     = storage.list();
        s_fxSelected = -1;
    }
    ImGui::SameLine();

    // Save current chain
    ImGui::SetNextItemWidth(130.f);
    ImGui::InputText("##fxname", s_fxSaveName, sizeof(s_fxSaveName));
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        EffectPreset ep;
        ep.name     = s_fxSaveName;
        ep.category = "User";
        ep.chain    = chain.getConfig();
        storage.save(ep, std::string(s_fxSaveName) + ".json");
        s_fxList = storage.list();
    }

    ImGui::Spacing();

    if (s_fxList.empty()) s_fxList = storage.list();

    ImGui::BeginChild("##fxlist", ImVec2(0.f, 200.f), true);
    for (int i = 0; i < static_cast<int>(s_fxList.size()); ++i) {
        const bool sel = (i == s_fxSelected);
        if (ImGui::Selectable(s_fxList[i].c_str(), sel,
                              ImGuiSelectableFlags_AllowDoubleClick)) {
            s_fxSelected = i;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                EffectPreset ep;
                if (storage.load(s_fxList[i], ep))
                    chain.setConfig(ep.chain);
            }
        }
    }
    ImGui::EndChild();

    ImGui::BeginDisabled(s_fxSelected < 0 ||
                          s_fxSelected >= static_cast<int>(s_fxList.size()));
    if (ImGui::Button("Load")) {
        EffectPreset ep;
        if (storage.load(s_fxList[s_fxSelected], ep))
            chain.setConfig(ep.chain);
    }
    ImGui::EndDisabled();

    if (!storage.getLastError().empty())
        ImGui::TextColored(ImVec4(1.f,0.3f,0.3f,1.f),
                           "%s", storage.getLastError().c_str());
}

// ── Public entry point ─────────────────────────────────
void render(AtomicParamStore&    params,
            PresetStorage&       moogStorage,
            EffectChain&         effectChain,
            EffectPresetStorage& effectStorage) {
    ImGui::Begin("Presets");

    if (ImGui::BeginTabBar("##presets_tabs")) {
        if (ImGui::BeginTabItem("Moog Presets")) {
            renderMoogPresetsTab(params, moogStorage);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Effect Presets")) {
            renderEffectPresetsTab(effectChain, effectStorage);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace PanelPresets
