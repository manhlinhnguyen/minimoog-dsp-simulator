// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_moog.cpp
// BRIEF: "Mini Moog Model D Engine" — 3-column fixed layout
//   Col 1 (215px) : Controllers
//   Col 2 (400px) : Oscillators  (top)  +  Mixer  (bottom)
//   Col 3 (260px) : Filter & Envelopes
// All column widths and heights are hard-coded so sections
// never shift or clip regardless of window resize.
// ─────────────────────────────────────────────────────────
#include "panel_moog.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "ui/widgets/adsr_display.h"
#include "core/engines/moog/moog_params.h"

namespace PanelEngine {

// ════════════════════════════════════════════════════════
// Fixed layout constants
// ════════════════════════════════════════════════════════
static constexpr float COL1_W  = 215.f;   // Controllers
static constexpr float COL2_W  = 400.f;   // Oscillators + Mixer
static constexpr float COL3_W  = 262.f;   // Filter & Envelopes
static constexpr float COL_H   = 610.f;   // tall enough for Filter & Envelopes (2× ADSR + filter knobs)
static constexpr float COL_GAP = 6.f;     // horizontal gap between columns

// ════════════════════════════════════════════════════════
// COLUMN 1 — Controllers
// ════════════════════════════════════════════════════════

static void renderControllers(MoogEngine& eng) {
    // ── Glide ──────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "GLIDE");
    ImGui::Separator();

    bool glideOn = eng.getParam(MP_GLIDE_ON) > 0.5f;
    if (ImGui::Checkbox("Glide", &glideOn))
        eng.setParam(MP_GLIDE_ON, glideOn ? 1.f : 0.f);

    float glideTime = eng.getParam(MP_GLIDE_TIME);
    ImGui::SetNextItemWidth(COL1_W - 90.f);
    if (ImGui::SliderFloat("Glide Time", &glideTime, 0.f, 1.f, "%.2f"))
        eng.setParam(MP_GLIDE_TIME, glideTime);

    ImGui::Spacing();

    // ── Modulation ─────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MODULATION");
    ImGui::Separator();

    float modMix = eng.getParam(MP_MOD_MIX);
    if (ImGuiKnobs::Knob("Mod Mix", &modMix, 0.f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 52.f))
        eng.setParam(MP_MOD_MIX, modMix);

    ImGui::SameLine();
    ImGui::BeginGroup();
    bool oscMod = eng.getParam(MP_OSC_MOD_ON) > 0.5f;
    if (ImGui::Checkbox("OSC Mod", &oscMod))
        eng.setParam(MP_OSC_MOD_ON, oscMod ? 1.f : 0.f);
    bool filterMod = eng.getParam(MP_FILTER_MOD_ON) > 0.5f;
    if (ImGui::Checkbox("Filter Mod", &filterMod))
        eng.setParam(MP_FILTER_MOD_ON, filterMod ? 1.f : 0.f);
    ImGui::EndGroup();

    ImGui::Spacing();

    // ── Polyphony ──────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "POLYPHONY");
    ImGui::Separator();

    const char* modes[] = {"Mono","Poly","Unison"};
    int voiceMode = static_cast<int>(eng.getParam(MP_VOICE_MODE));
    ImGui::SetNextItemWidth(COL1_W - 70.f);
    if (ImGui::Combo("Mode", &voiceMode, modes, 3))
        eng.setParam(MP_VOICE_MODE, static_cast<float>(voiceMode));

    if (voiceMode == 0) {
        const char* priorities[] = {"Last","Lowest","Highest"};
        int prio = static_cast<int>(eng.getParam(MP_NOTE_PRIORITY));
        ImGui::SetNextItemWidth(COL1_W - 70.f);
        if (ImGui::Combo("Priority", &prio, priorities, 3))
            eng.setParam(MP_NOTE_PRIORITY, static_cast<float>(prio));
    }

    int voiceCount = static_cast<int>(eng.getParam(MP_VOICE_COUNT));
    ImGui::SetNextItemWidth(COL1_W - 70.f);
    if (ImGui::SliderInt("Voices", &voiceCount, 1, 8))
        eng.setParam(MP_VOICE_COUNT, static_cast<float>(voiceCount));

    const char* stealModes[] = {"Oldest","Lowest","Quietest"};
    int stealMode = static_cast<int>(eng.getParam(MP_VOICE_STEAL));
    ImGui::SetNextItemWidth(COL1_W - 70.f);
    if (ImGui::Combo("Steal", &stealMode, stealModes, 3))
        eng.setParam(MP_VOICE_STEAL, static_cast<float>(stealMode));

    float detune = eng.getParam(MP_UNISON_DETUNE);
    ImGui::SetNextItemWidth(COL1_W - 70.f);
    if (ImGui::SliderFloat("Detune", &detune, 0.f, 1.f, "%.2f"))
        eng.setParam(MP_UNISON_DETUNE, detune);
}

// ════════════════════════════════════════════════════════
// COLUMN 2 — Oscillators
// ════════════════════════════════════════════════════════

// Shared column width — used by both oscillator table and mixer table
// so OSC 1/2/3 mixer knobs align perfectly with the oscillator strips.
static constexpr float OSC_COL_W = 125.f;

static void renderOscStrip(MoogEngine& eng, int oscIdx, const char* label) {
    const int base = MP_OSC1_ON + oscIdx * 4;
    const int ON  = base;
    const int RNG = base + 1;
    const int FRQ = base + 2;
    const int WAV = base + 3;

    ImGui::PushID(oscIdx);

    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "%s", label);
    ImGui::Separator();

    bool on = eng.getParam(ON) > 0.5f;
    if (ImGui::Checkbox("ON", &on))
        eng.setParam(ON, on ? 1.f : 0.f);

    const char* ranges[] = {"LO","32'","16'","8'","4'","2'"};
    int rng = static_cast<int>(eng.getParam(RNG));
    ImGui::SetNextItemWidth(60.f);
    if (ImGui::Combo("Range", &rng, ranges, 6))
        eng.setParam(RNG, static_cast<float>(rng));

    const char* waves[] = {"Tri","TriSaw","RevSaw","Saw","Square","Pulse"};
    int wav = static_cast<int>(eng.getParam(WAV));
    ImGui::SetNextItemWidth(70.f);
    if (ImGui::Combo("Wave", &wav, waves, 6))
        eng.setParam(WAV, static_cast<float>(wav));

    float frq = eng.getParam(FRQ);
    if (ImGuiKnobs::Knob("Freq", &frq, 0.f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 55.f))
        eng.setParam(FRQ, frq);

    if (oscIdx == 2) {
        ImGui::Separator();
        bool lfoMode = eng.getParam(MP_OSC3_LFO_ON) > 0.5f;
        if (ImGui::Checkbox("LFO Mode", &lfoMode))
            eng.setParam(MP_OSC3_LFO_ON, lfoMode ? 1.f : 0.f);
    }

    ImGui::PopID();
}

static void renderOscillators(MoogEngine& eng) {
    // 3-column table so mixer table can share the same column widths
    if (ImGui::BeginTable("##osctbl", 3,
                          ImGuiTableFlags_None,
                          ImVec2(3.f * OSC_COL_W, 0.f))) {
        for (int i = 0; i < 3; ++i)
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, OSC_COL_W);

        ImGui::TableNextColumn(); renderOscStrip(eng, 0, "OSC 1");
        ImGui::TableNextColumn(); renderOscStrip(eng, 1, "OSC 2");
        ImGui::TableNextColumn(); renderOscStrip(eng, 2, "OSC 3");

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();

    float tune = eng.getParam(MP_MASTER_TUNE);
    ImGui::SetNextItemWidth(150.f);
    if (ImGui::SliderFloat("Master Tune", &tune, -1.f, 1.f))
        eng.setParam(MP_MASTER_TUNE, tune);
}

// ════════════════════════════════════════════════════════
// COLUMN 2 — Mixer
// ════════════════════════════════════════════════════════

static void renderMixer(MoogEngine& eng) {
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MIXER");
    ImGui::Separator();

    // OSC 1/2/3 knobs — same table column widths as oscillator section
    if (ImGui::BeginTable("##mixtbl", 3,
                          ImGuiTableFlags_None,
                          ImVec2(3.f * OSC_COL_W, 0.f))) {
        for (int i = 0; i < 3; ++i)
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, OSC_COL_W);

        const struct { const char* label; int id; } osc[3] = {
            {"OSC 1", MP_MIX_OSC1},
            {"OSC 2", MP_MIX_OSC2},
            {"OSC 3", MP_MIX_OSC3},
        };
        for (int i = 0; i < 3; ++i) {
            ImGui::TableNextColumn();
            float v = eng.getParam(osc[i].id);
            if (ImGuiKnobs::Knob(osc[i].label, &v, 0.f, 1.f, 0.005f,
                                  "%.2f", ImGuiKnobVariant_Wiper, 55.f))
                eng.setParam(osc[i].id, v);
        }
        ImGui::EndTable();
    }

    // Noise knob + Noise Color combo on the same row
    ImGui::Spacing();
    float vNoise = eng.getParam(MP_MIX_NOISE);
    if (ImGuiKnobs::Knob("Noise", &vNoise, 0.f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 55.f))
        eng.setParam(MP_MIX_NOISE, vNoise);

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Spacing();
    const char* colors[] = {"White","Pink"};
    int color = static_cast<int>(eng.getParam(MP_NOISE_COLOR));
    ImGui::SetNextItemWidth(90.f);
    if (ImGui::Combo("Noise Color", &color, colors, 2))
        eng.setParam(MP_NOISE_COLOR, static_cast<float>(color));
    ImGui::EndGroup();
}

// ════════════════════════════════════════════════════════
// COLUMN 3 — Filter & Envelopes
// ════════════════════════════════════════════════════════

static void renderADSRKnobs(MoogEngine& eng,
                              int A, int D, int S, int R,
                              const char* label) {
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "%s", label);
    ImGui::Separator();

    float a = eng.getParam(A), d = eng.getParam(D),
          s = eng.getParam(S), r = eng.getParam(R);

    ImGui::PushID(label);
    if (ImGuiKnobs::Knob("A",&a,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,48.f)) eng.setParam(A,a);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("D",&d,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,48.f)) eng.setParam(D,d);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("S",&s,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,48.f)) eng.setParam(S,s);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("R",&r,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,48.f)) eng.setParam(R,r);

    AdsrDisplay::draw(a, d, s, r, ImVec2(COL3_W - 18.f, 55.f));
    ImGui::PopID();
}

static void renderFilterAndEnvelopes(MoogEngine& eng) {
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MOOG FILTER");
    ImGui::Separator();

    float cutoff = eng.getParam(MP_FILTER_CUTOFF);
    float res    = eng.getParam(MP_FILTER_EMPHASIS);
    float amt    = eng.getParam(MP_FILTER_AMOUNT);

    if (ImGuiKnobs::Knob("Cutoff",&cutoff,0.f,1.f,0.005f,
            "%.2f",ImGuiKnobVariant_Wiper,52.f))
        eng.setParam(MP_FILTER_CUTOFF, cutoff);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Emphasis",&res,0.f,1.f,0.005f,
            "%.2f",ImGuiKnobVariant_Wiper,52.f))
        eng.setParam(MP_FILTER_EMPHASIS, res);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Env Amt",&amt,0.f,1.f,0.005f,
            "%.2f",ImGuiKnobVariant_Wiper,52.f))
        eng.setParam(MP_FILTER_AMOUNT, amt);

    const char* kbdModes[] = {"Off","1/3","2/3"};
    int kbd = static_cast<int>(eng.getParam(MP_FILTER_KBD_TRACK));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("KBD Track", &kbd, kbdModes, 3))
        eng.setParam(MP_FILTER_KBD_TRACK, static_cast<float>(kbd));

    ImGui::Spacing();
    renderADSRKnobs(eng,
        MP_FENV_ATTACK, MP_FENV_DECAY,
        MP_FENV_SUSTAIN, MP_FENV_RELEASE, "FILTER ENV");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    renderADSRKnobs(eng,
        MP_AENV_ATTACK, MP_AENV_DECAY,
        MP_AENV_SUSTAIN, MP_AENV_RELEASE, "AMP ENV");
}

// ════════════════════════════════════════════════════════
// PUBLIC ENTRY POINT
// ════════════════════════════════════════════════════════

void renderContent(MoogEngine& eng) {
    // ── Column 1: Controllers (fixed 215 × 510 px) ───────
    ImGui::BeginChild("##col_ctrl",
                      ImVec2(COL1_W, COL_H), true);

    if (ImGui::CollapsingHeader("Controllers",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        renderControllers(eng);
    }

    ImGui::EndChild();  // col_ctrl
    ImGui::SameLine(0.f, COL_GAP);

    // ── Column 2: Oscillators + Mixer (fixed 400 × 510 px) ─
    ImGui::BeginChild("##col_mid",
                      ImVec2(COL2_W, COL_H), true);

    if (ImGui::CollapsingHeader("Oscillators",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        renderOscillators(eng);
        ImGui::Spacing();
    }

    if (ImGui::CollapsingHeader("Mixer",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        renderMixer(eng);
        ImGui::Spacing();
    }

    ImGui::EndChild();  // col_mid
    ImGui::SameLine(0.f, COL_GAP);

    // ── Column 3: Filter & Envelopes (fixed 262 × 510 px) ──
    ImGui::BeginChild("##col_filter",
                      ImVec2(COL3_W, COL_H), true);

    if (ImGui::CollapsingHeader("Filter & Envelopes",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        renderFilterAndEnvelopes(eng);
    }

    ImGui::EndChild();  // col_filter
}

void render(MoogEngine& eng) {
    const float winW = COL1_W + COL2_W + COL3_W
                       + COL_GAP * 2.f
                       + ImGui::GetStyle().WindowPadding.x * 2.f
                       + ImGui::GetStyle().ScrollbarSize;
    const float winH = COL_H
                       + ImGui::GetFrameHeight()
                       + ImGui::GetStyle().WindowPadding.y * 2.f
                       + ImGui::GetFrameHeight() + 6.f;

    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mini Moog Model D Engine");
    renderContent(eng);
    ImGui::End();
}

} // namespace PanelEngine
