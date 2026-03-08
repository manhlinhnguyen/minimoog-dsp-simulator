// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_presets.cpp
// BRIEF: Combined preset browser — Engine Presets + Effect Presets tabs
// ─────────────────────────────────────────────────────────
#include "panel_presets.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <cstring>

namespace PanelPresets {

// ─────────────────────────────────────────────────────────
// Tab 1: Engine Presets
// ─────────────────────────────────────────────────────────

static char s_engSaveName[128]          = "MyPreset";
static int  s_engSelected               = -1;
static int  s_lastActiveIdx             = -2;
static std::vector<std::string> s_engList;

static const char* presetHeader(const char* engineName) {
    if (strcmp(engineName, "MiniMoog Model D")   == 0) return "MOOG PRESETS";
    if (strcmp(engineName, "Hammond B-3")         == 0) return "HAMMOND PRESETS";
    if (strcmp(engineName, "Rhodes Mark I")       == 0) return "RHODES PRESETS";
    if (strcmp(engineName, "Yamaha DX7")          == 0) return "DX7 PRESETS";
    if (strcmp(engineName, "Mellotron M400")      == 0) return "MELLOTRON PRESETS";
    if (strcmp(engineName, "Hybrid Drum Machine") == 0) return "DRUM KITS";
    return "ENGINE PRESETS";
}

static void renderEnginePresetsTab(EngineManager&       mgr,
                                    EnginePresetStorage& storage) {
    IEngine*    active    = mgr.getActiveEngine();
    const char* name      = active ? active->getName() : "";
    const int   activeIdx = mgr.getActiveIndex();

    // Refresh list when engine switches
    if (activeIdx != s_lastActiveIdx) {
        s_engList       = storage.list();
        s_engSelected   = -1;
        s_lastActiveIdx = activeIdx;
    }
    if (s_engList.empty())
        s_engList = storage.list();

    // ── Header ────────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.1f, 1.f), "%s", presetHeader(name));
    ImGui::Separator();

    // ── Action buttons ────────────────────────────────────
    if (ImGui::Button("Reset##eng") && active) {
        active->allNotesOff();
        for (int i = 0; i < active->getParamCount(); ++i)
            active->setParam(i, active->getParamDefault(i));
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reset all parameters to default values");

    ImGui::SameLine();
    if (ImGui::Button("Refresh##eng")) {
        s_engList     = storage.list();
        s_engSelected = -1;
    }

    ImGui::SameLine(0.f, 16.f);
    ImGui::SetNextItemWidth(130.f);
    ImGui::InputText("##engname", s_engSaveName, sizeof(s_engSaveName));
    ImGui::SameLine();
    if (ImGui::Button("Save##eng") && active) {
        const std::string fn = std::string(s_engSaveName) + ".json";
        storage.save(fn, s_engSaveName, *active);
        s_engList = storage.list();
    }

    ImGui::Spacing();

    // ── Preset list ───────────────────────────────────────
    ImGui::BeginChild("##englist", ImVec2(0.f, 200.f), true);
    for (int i = 0; i < static_cast<int>(s_engList.size()); ++i) {
        const bool sel = (i == s_engSelected);
        if (ImGui::Selectable(s_engList[i].c_str(), sel,
                              ImGuiSelectableFlags_AllowDoubleClick)) {
            s_engSelected = i;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && active) {
                active->allNotesOff();
                storage.loadByIndex(i, *active);
            }
        }
    }
    ImGui::EndChild();

    // Load button — below list, right-aligned
    {
        const float btnW = 80.f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - btnW);
        ImGui::BeginDisabled(s_engSelected < 0 ||
                              s_engSelected >= static_cast<int>(s_engList.size()));
        if (ImGui::Button("Load##eng", ImVec2(btnW, 0.f)) && active) {
            active->allNotesOff();
            storage.loadByIndex(s_engSelected, *active);
        }
        ImGui::EndDisabled();
    }

    if (!storage.getLastError().empty())
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f),
                           "%s", storage.getLastError().c_str());
}

// ─────────────────────────────────────────────────────────
// Tab 2: Effect Presets
// ─────────────────────────────────────────────────────────

static char s_fxSaveName[128]           = "MyEffectPreset";
static int  s_fxSelected                = -1;
static std::vector<std::string> s_fxList;

static void renderEffectPresetsTab(EffectChain& chain, EffectPresetStorage& storage) {
    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.1f, 1.f), "EFFECT PRESETS");
    ImGui::Separator();

    if (ImGui::Button("Reset##fx")) {
        EffectChainConfig empty{};
        chain.setConfig(empty);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Removes all effects from the chain");

    ImGui::SameLine();
    if (ImGui::Button("Refresh##fx")) {
        s_fxList     = storage.list();
        s_fxSelected = -1;
    }
    ImGui::SameLine(0.f, 16.f);
    ImGui::SetNextItemWidth(130.f);
    ImGui::InputText("##fxname", s_fxSaveName, sizeof(s_fxSaveName));
    ImGui::SameLine();
    if (ImGui::Button("Save##fx")) {
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

    // Load button — right-aligned
    {
        const float btnW = 80.f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - btnW);
        ImGui::BeginDisabled(s_fxSelected < 0 ||
                              s_fxSelected >= static_cast<int>(s_fxList.size()));
        if (ImGui::Button("Load##fx", ImVec2(btnW, 0.f))) {
            EffectPreset ep;
            if (storage.load(s_fxList[s_fxSelected], ep))
                chain.setConfig(ep.chain);
        }
        ImGui::EndDisabled();
    }

    if (!storage.getLastError().empty())
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f),
                           "%s", storage.getLastError().c_str());
}

// ─────────────────────────────────────────────────────────
// Tab 3: Global Presets (engine + effects combined)
// ─────────────────────────────────────────────────────────

static char s_glbSaveName[128]          = "MyGlobalPreset";
static char s_glbCategory[64]           = "User";
static int  s_glbSelected               = -1;
static std::vector<std::string> s_glbList;

static void renderGlobalPresetsTab(EngineManager&       mgr,
                                    EffectChain&         chain,
                                    GlobalPresetStorage& storage) {
    ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.1f, 1.f), "GLOBAL PRESETS");
    ImGui::TextDisabled("Load engine sound + effect chain together");
    ImGui::Separator();

    if (s_glbList.empty())
        s_glbList = storage.list();

    // ── Action buttons ────────────────────────────────────
    if (ImGui::Button("Refresh##glb")) {
        s_glbList    = storage.list();
        s_glbSelected = -1;
    }

    ImGui::SameLine(0.f, 16.f);
    ImGui::SetNextItemWidth(130.f);
    ImGui::InputText("##glbname", s_glbSaveName, sizeof(s_glbSaveName));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.f);
    ImGui::InputText("##glbcat", s_glbCategory, sizeof(s_glbCategory));
    ImGui::SameLine();
    if (ImGui::Button("Save##glb")) {
        GlobalPreset gp = storage.capture(s_glbSaveName, s_glbCategory,
                                           mgr, chain);
        const std::string fn = std::string(s_glbSaveName) + ".json";
        storage.save(fn, gp);
        s_glbList = storage.list();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Save current engine + effects as a global preset");

    ImGui::Spacing();

    // ── Preset list ───────────────────────────────────────
    ImGui::BeginChild("##glblist", ImVec2(0.f, 200.f), true);
    for (int i = 0; i < static_cast<int>(s_glbList.size()); ++i) {
        const bool sel = (i == s_glbSelected);
        if (ImGui::Selectable(s_glbList[i].c_str(), sel,
                              ImGuiSelectableFlags_AllowDoubleClick)) {
            s_glbSelected = i;
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                GlobalPreset gp;
                if (storage.load(s_glbList[i], gp))
                    storage.apply(gp, mgr, chain);
            }
        }
    }
    ImGui::EndChild();

    // Load button — below list, right-aligned
    {
        const float btnW = 80.f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - btnW);
        ImGui::BeginDisabled(s_glbSelected < 0 ||
                              s_glbSelected >= static_cast<int>(s_glbList.size()));
        if (ImGui::Button("Load##glb", ImVec2(btnW, 0.f))) {
            GlobalPreset gp;
            if (storage.load(s_glbList[s_glbSelected], gp))
                storage.apply(gp, mgr, chain);
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Load engine + effects from selected preset");
        ImGui::EndDisabled();
    }

    if (!storage.getLastError().empty())
        ImGui::TextColored(ImVec4(1.f, 0.3f, 0.3f, 1.f),
                           "%s", storage.getLastError().c_str());
}

// ─────────────────────────────────────────────────────────
// Public entry point
// ─────────────────────────────────────────────────────────

void render(EngineManager&       mgr,
            EnginePresetStorage& engineStorage,
            EffectChain&         effectChain,
            EffectPresetStorage& effectStorage,
            GlobalPresetStorage& globalStorage) {
    ImGui::Begin("Presets");

    if (ImGui::BeginTabBar("##presets_tabs")) {
        if (ImGui::BeginTabItem("Engine")) {
            renderEnginePresetsTab(mgr, engineStorage);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Effect")) {
            renderEffectPresetsTab(effectChain, effectStorage);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Global")) {
            renderGlobalPresetsTab(mgr, effectChain, globalStorage);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace PanelPresets
