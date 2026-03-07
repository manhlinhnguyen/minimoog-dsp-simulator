// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_effects.cpp
// BRIEF: Effect chain editor + preset browser UI
// ─────────────────────────────────────────────────────────
#include "panel_effects.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include <cstring>
#include <cstdio>

namespace PanelEffects {

// Card colors per effect type
static ImVec4 effectColor(EffectType t) {
    switch (t) {
        case EffectType::Gain:    return ImVec4(0.70f, 0.35f, 0.10f, 1.f);
        case EffectType::Chorus:  return ImVec4(0.15f, 0.50f, 0.80f, 1.f);
        case EffectType::Flanger: return ImVec4(0.20f, 0.65f, 0.55f, 1.f);
        case EffectType::Phaser:  return ImVec4(0.55f, 0.25f, 0.75f, 1.f);
        case EffectType::Tremolo: return ImVec4(0.70f, 0.60f, 0.10f, 1.f);
        case EffectType::Delay:   return ImVec4(0.20f, 0.40f, 0.70f, 1.f);
        case EffectType::Reverb:     return ImVec4(0.15f, 0.55f, 0.45f, 1.f);
        case EffectType::Equalizer:  return ImVec4(0.60f, 0.45f, 0.10f, 1.f);
        default:                     return ImVec4(0.35f, 0.35f, 0.35f, 1.f);
    }
}

static const char* effectTooltip(EffectType t) {
    switch (t) {
        case EffectType::Gain:    return "Gain / Overdrive / Distortion\nMode: 0=Boost 1=OD 2=Dist";
        case EffectType::Chorus:  return "Stereo chorus — modulated delay voices";
        case EffectType::Flanger: return "Jet-plane flanger — short feedback delay";
        case EffectType::Phaser:  return "4-stage all-pass phaser";
        case EffectType::Tremolo: return "Amplitude tremolo — Sine/Triangle/Square";
        case EffectType::Delay:   return "Stereo echo delay (up to 2 seconds)";
        case EffectType::Reverb:     return "Schroeder reverb — comb + all-pass";
        case EffectType::Equalizer:  return "5-band EQ: Low(80Hz) LowMid(320Hz) Mid(1kHz) HiMid(3.5kHz) High(10kHz)\n±12 dB per band";
        default:                     return "";
    }
}

// ─────────────────────────────────────────────────────────
// renderEffectChainWindow (private)
// ─────────────────────────────────────────────────────────
static void renderEffectChainWindow(EffectChain& chain) {
    ImGui::Begin("Effects");

    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "EFFECT CHAIN");
    ImGui::SameLine();
    ImGui::TextDisabled("(drag not impl — use < > to reorder)");
    ImGui::Separator();

    const int numSlots = chain.slotCount();

    // Layout constants (measured from imgui-knobs internals):
    //   knob group height seen by parent = (13 title + sz circle + 21 dragscalar + 3×ItemSpacing.y) = sz+44+5 = sz+49
    //   For sz=42: one row = 91 px
    //   Card inner content = header(26) + sep(5) + rows + sep(5) + btn(18) + 2×pad(20) + 2×border(2)
    //   1-row card : 22+26+5+91+5+18      = 167 → 170 px
    //   2-row card : 22+26+5+91+91+5+18   = 258 → 262 px
    static constexpr float KNOB_SZ    = 42.f;
    static constexpr float CARD_W     = 190.f;  // 165 + 25 extra for label breathing room
    static constexpr float CARD_H     = 262.f;  // uniform height for all cards
    static constexpr float MAX_CARD_H = CARD_H;
    const float kGap = ImGui::GetStyle().ItemSpacing.x;  // 8 px

    // Horizontal scrolling container sized to tallest possible card
    ImGui::BeginChild("##effectchain",
                      ImVec2(0.f, MAX_CARD_H + 20.f),
                      false,
                      ImGuiWindowFlags_HorizontalScrollbar);

    bool changed = false;

    for (int s = 0; s < numSlots; ++s) {
        const int   pc    = chain.slotParamCount(s);

        const ImVec4 col = effectColor(chain.slotType(s));
        ImGui::PushID(s);

        ImGui::PushStyleColor(ImGuiCol_ChildBg,
            ImVec4(col.x*0.25f, col.y*0.25f, col.z*0.25f, 1.f));
        ImGui::BeginChild("##card", ImVec2(CARD_W, CARD_H), true);

        // ── Header: ON toggle + effect name ──────────────────
        bool en = chain.slotEnabled(s);
        ImGui::PushStyleColor(ImGuiCol_CheckMark, col);
        if (ImGui::Checkbox("##en", &en))
            chain.setSlotEnabled(s, en);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::TextColored(col, "%s", chain.slotTypeName(s));
        ImGui::Separator();

        // ── Knob rows: fill row 1 (up to 3), then row 2 (centered) ──
        const int row1 = (pc < 3) ? pc : 3;   // min(pc, 3)
        const int row2 = pc - row1;

        // Row 1
        for (int p = 0; p < row1; ++p) {
            float v = chain.slotParam(s, p);
            ImGui::PushID(p);
            if (ImGuiKnobs::Knob(chain.slotParamName(s, p), &v,
                                 chain.slotParamMin(s, p),
                                 chain.slotParamMax(s, p),
                                 0.f, "%.2f", ImGuiKnobVariant_Wiper, KNOB_SZ))
                chain.setSlotParam(s, p, v);
            ImGui::PopID();
            if (p + 1 < row1) ImGui::SameLine();
        }

        // Row 2 — centered under row 1
        if (row2 > 0) {
            const float row1W   = row1 * KNOB_SZ + (row1 - 1) * kGap;
            const float row2W   = row2 * KNOB_SZ + (row2 - 1) * kGap;
            const float offX    = (row1W - row2W) * 0.5f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offX);
            for (int p = row1; p < pc; ++p) {
                float v = chain.slotParam(s, p);
                ImGui::PushID(p);
                if (ImGuiKnobs::Knob(chain.slotParamName(s, p), &v,
                                     chain.slotParamMin(s, p),
                                     chain.slotParamMax(s, p),
                                     0.f, "%.2f", ImGuiKnobVariant_Wiper, KNOB_SZ))
                    chain.setSlotParam(s, p, v);
                ImGui::PopID();
                if (p + 1 < pc) ImGui::SameLine();
            }
        }

        // ── Footer: reorder / remove buttons ─────────────────
        ImGui::SetCursorPosY(CARD_H - ImGui::GetFrameHeight() - 12.f);
        ImGui::Separator();

        ImGui::BeginDisabled(s == 0);
        if (ImGui::SmallButton("<")) {
            EffectChainConfig cfg = chain.getConfig();
            if (s > 0) { std::swap(cfg.slots[s], cfg.slots[s-1]); chain.setConfig(cfg); }
            changed = true;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::BeginDisabled(s >= numSlots - 1);
        if (ImGui::SmallButton(">")) {
            EffectChainConfig cfg = chain.getConfig();
            if (s < cfg.numSlots - 1) { std::swap(cfg.slots[s], cfg.slots[s+1]); chain.setConfig(cfg); }
            changed = true;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f,0.1f,0.1f,1.f));
        if (ImGui::SmallButton("X")) {
            EffectChainConfig cfg = chain.getConfig();
            for (int k = s; k < cfg.numSlots - 1; ++k) cfg.slots[k] = cfg.slots[k+1];
            cfg.numSlots--;
            chain.setConfig(cfg);
            changed = true;
        }
        ImGui::PopStyleColor();

        ImGui::EndChild();
        ImGui::PopStyleColor();  // ChildBg
        ImGui::SameLine();
        ImGui::PopID();

        if (changed) break;
    }

    // "+ Add Effect" button
    if (!changed) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f,0.35f,0.15f,1.f));
        if (ImGui::Button("[+ Add Effect]", ImVec2(CARD_W * 0.75f, 40.f)))
            ImGui::OpenPopup("AddEffect");
        ImGui::PopStyleColor();

        // Add effect popup
        if (ImGui::BeginPopup("AddEffect")) {
            ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "Choose effect:");
            ImGui::Separator();
            for (int t = 0; t < static_cast<int>(EffectType::COUNT); ++t) {
                const EffectType et = static_cast<EffectType>(t);
                if (ImGui::Selectable(effectTypeName(et))) {
                    EffectChainConfig cfg = chain.getConfig();
                    if (cfg.numSlots < EffectChain::MAX_SLOTS) {
                        EffectSlotConfig& slot = cfg.slots[cfg.numSlots];
                        slot.type    = et;
                        slot.enabled = true;
                        // Default params — create temp effect to read them
                        auto tmp = createEffect(et, 44100.f);
                        if (tmp) {
                            const int pc = tmp->paramCount();
                            for (int p = 0; p < pc && p < 8; ++p)
                                slot.params[p] = tmp->paramDefault(p);
                        }
                        cfg.numSlots++;
                        chain.setConfig(cfg);
                    }
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", effectTooltip(et));
            }
            ImGui::EndPopup();
        }
    }

    ImGui::EndChild();  // ##effectchain

    if (numSlots == 0)
        ImGui::TextDisabled("No effects — click [+ Add Effect] to start.");

    ImGui::End();
}

// ─────────────────────────────────────────────────────────
// PUBLIC
// ─────────────────────────────────────────────────────────
void render(EffectChain& chain) {
    renderEffectChainWindow(chain);
}

} // namespace PanelEffects
