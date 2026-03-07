// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_moog_engine.cpp
// BRIEF: "Mini Moog Model D Engine" — 3-column fixed layout
//   Col 1 (215px) : Controllers
//   Col 2 (400px) : Oscillators  (top)  +  Mixer  (bottom)
//   Col 3 (260px) : Filter & Envelopes
// All column widths and heights are hard-coded so sections
// never shift or clip regardless of window resize.
// ─────────────────────────────────────────────────────────
#include "panel_moog_engine.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "ui/widgets/adsr_display.h"
#include "shared/params.h"

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

static void renderControllers(AtomicParamStore& p) {
    // ── Glide ──────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "GLIDE");
    ImGui::Separator();

    bool glideOn = p.get(P_GLIDE_ON) > 0.5f;
    if (ImGui::Checkbox("Glide On", &glideOn))
        p.set(P_GLIDE_ON, glideOn ? 1.f : 0.f);

    float glideTime = p.get(P_GLIDE_TIME);
    ImGui::SetNextItemWidth(COL1_W - 90.f);
    if (ImGui::SliderFloat("Glide Time", &glideTime, 0.f, 1.f, "%.2f"))
        p.set(P_GLIDE_TIME, glideTime);

    ImGui::Spacing();

    // ── Modulation ─────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MODULATION");
    ImGui::Separator();

    float modMix = p.get(P_MOD_MIX);
    if (ImGuiKnobs::Knob("Mod Mix", &modMix, 0.f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 52.f))
        p.set(P_MOD_MIX, modMix);

    ImGui::SameLine();
    ImGui::BeginGroup();
    bool oscMod = p.get(P_OSC_MOD_ON) > 0.5f;
    if (ImGui::Checkbox("OSC Mod", &oscMod))
        p.set(P_OSC_MOD_ON, oscMod ? 1.f : 0.f);
    bool filterMod = p.get(P_FILTER_MOD_ON) > 0.5f;
    if (ImGui::Checkbox("Filter Mod", &filterMod))
        p.set(P_FILTER_MOD_ON, filterMod ? 1.f : 0.f);
    ImGui::EndGroup();

    ImGui::Spacing();

    // ── Tempo ──────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "TEMPO");
    ImGui::Separator();

    float bpm = p.get(P_BPM);
    ImGui::SetNextItemWidth(COL1_W - 60.f);
    if (ImGui::SliderFloat("BPM", &bpm, 20.f, 300.f, "%.0f"))
        p.set(P_BPM, bpm);

    ImGui::Spacing();

    // ── Polyphony ──────────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "POLYPHONY");
    ImGui::Separator();

    const char* modes[] = {"Mono","Poly","Unison"};
    int voiceMode = static_cast<int>(p.get(P_VOICE_MODE));
    ImGui::SetNextItemWidth(COL1_W - 70.f);
    if (ImGui::Combo("Mode", &voiceMode, modes, 3))
        p.set(P_VOICE_MODE, static_cast<float>(voiceMode));

    int voiceCount = static_cast<int>(p.get(P_VOICE_COUNT));
    ImGui::SetNextItemWidth(COL1_W - 70.f);
    if (ImGui::SliderInt("Voices", &voiceCount, 1, 8))
        p.set(P_VOICE_COUNT, static_cast<float>(voiceCount));

    const char* stealModes[] = {"Oldest","Lowest","Quietest"};
    int stealMode = static_cast<int>(p.get(P_VOICE_STEAL));
    ImGui::SetNextItemWidth(COL1_W - 70.f);
    if (ImGui::Combo("Steal", &stealMode, stealModes, 3))
        p.set(P_VOICE_STEAL, static_cast<float>(stealMode));

    float detune = p.get(P_UNISON_DETUNE);
    ImGui::SetNextItemWidth(COL1_W - 70.f);
    if (ImGui::SliderFloat("Detune", &detune, 0.f, 1.f, "%.2f"))
        p.set(P_UNISON_DETUNE, detune);
}

// ════════════════════════════════════════════════════════
// COLUMN 2 — Oscillators
// ════════════════════════════════════════════════════════

// Shared column width — used by both oscillator table and mixer table
// so OSC 1/2/3 mixer knobs align perfectly with the oscillator strips.
static constexpr float OSC_COL_W = 125.f;

static void renderOscStrip(AtomicParamStore& p,
                             int oscIdx, const char* label) {
    const int base = P_OSC1_ON + oscIdx * 4;
    const int ON  = base;
    const int RNG = base + 1;
    const int FRQ = base + 2;
    const int WAV = base + 3;

    ImGui::PushID(oscIdx);

    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "%s", label);
    ImGui::Separator();

    bool on = p.get(ON) > 0.5f;
    if (ImGui::Checkbox("ON", &on))
        p.set(ON, on ? 1.f : 0.f);

    const char* ranges[] = {"LO","32'","16'","8'","4'","2'"};
    int rng = static_cast<int>(p.get(RNG));
    ImGui::SetNextItemWidth(60.f);
    if (ImGui::Combo("Range", &rng, ranges, 6))
        p.set(RNG, static_cast<float>(rng));

    const char* waves[] = {"Tri","TriSaw","RevSaw","Saw","Square","Pulse"};
    int wav = static_cast<int>(p.get(WAV));
    ImGui::SetNextItemWidth(70.f);
    if (ImGui::Combo("Wave", &wav, waves, 6))
        p.set(WAV, static_cast<float>(wav));

    float frq = p.get(FRQ);
    if (ImGuiKnobs::Knob("Freq", &frq, 0.f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 55.f))
        p.set(FRQ, frq);

    if (oscIdx == 2) {
        ImGui::Separator();
        bool lfoMode = p.get(P_OSC3_LFO_ON) > 0.5f;
        if (ImGui::Checkbox("LFO Mode", &lfoMode))
            p.set(P_OSC3_LFO_ON, lfoMode ? 1.f : 0.f);
    }

    ImGui::PopID();
}

static void renderOscillators(AtomicParamStore& params) {
    // 3-column table so mixer table can share the same column widths
    if (ImGui::BeginTable("##osctbl", 3,
                          ImGuiTableFlags_None,
                          ImVec2(3.f * OSC_COL_W, 0.f))) {
        for (int i = 0; i < 3; ++i)
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, OSC_COL_W);

        ImGui::TableNextColumn(); renderOscStrip(params, 0, "OSC 1");
        ImGui::TableNextColumn(); renderOscStrip(params, 1, "OSC 2");
        ImGui::TableNextColumn(); renderOscStrip(params, 2, "OSC 3");

        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();

    float tune = params.get(P_MASTER_TUNE);
    ImGui::SetNextItemWidth(150.f);
    if (ImGui::SliderFloat("Master Tune", &tune, -1.f, 1.f))
        params.set(P_MASTER_TUNE, tune);
}

// ════════════════════════════════════════════════════════
// COLUMN 2 — Mixer
// ════════════════════════════════════════════════════════

static void renderMixer(AtomicParamStore& p) {
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MIXER");
    ImGui::Separator();

    // OSC 1/2/3 knobs — same table column widths as oscillator section
    if (ImGui::BeginTable("##mixtbl", 3,
                          ImGuiTableFlags_None,
                          ImVec2(3.f * OSC_COL_W, 0.f))) {
        for (int i = 0; i < 3; ++i)
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, OSC_COL_W);

        const struct { const char* label; int id; } osc[3] = {
            {"OSC 1", P_MIX_OSC1},
            {"OSC 2", P_MIX_OSC2},
            {"OSC 3", P_MIX_OSC3},
        };
        for (int i = 0; i < 3; ++i) {
            ImGui::TableNextColumn();
            float v = p.get(osc[i].id);
            if (ImGuiKnobs::Knob(osc[i].label, &v, 0.f, 1.f, 0.005f,
                                  "%.2f", ImGuiKnobVariant_Wiper, 55.f))
                p.set(osc[i].id, v);
        }
        ImGui::EndTable();
    }

    // Noise knob + Noise Color combo on the same row
    ImGui::Spacing();
    float vNoise = p.get(P_MIX_NOISE);
    if (ImGuiKnobs::Knob("Noise", &vNoise, 0.f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 55.f))
        p.set(P_MIX_NOISE, vNoise);

    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Spacing();
    const char* colors[] = {"White","Pink"};
    int color = static_cast<int>(p.get(P_NOISE_COLOR));
    ImGui::SetNextItemWidth(90.f);
    if (ImGui::Combo("Noise Color", &color, colors, 2))
        p.set(P_NOISE_COLOR, static_cast<float>(color));
    ImGui::EndGroup();
}

// ════════════════════════════════════════════════════════
// COLUMN 3 — Filter & Envelopes
// ════════════════════════════════════════════════════════

static void renderADSRKnobs(AtomicParamStore& p,
                              int A, int D, int S, int R,
                              const char* label) {
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "%s", label);
    ImGui::Separator();

    float a = p.get(A), d = p.get(D),
          s = p.get(S), r = p.get(R);

    ImGui::PushID(label);
    if (ImGuiKnobs::Knob("A",&a,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,48.f)) p.set(A,a);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("D",&d,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,48.f)) p.set(D,d);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("S",&s,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,48.f)) p.set(S,s);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("R",&r,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper,48.f)) p.set(R,r);

    AdsrDisplay::draw(a, d, s, r, ImVec2(COL3_W - 18.f, 55.f));
    ImGui::PopID();
}

static void renderFilterAndEnvelopes(AtomicParamStore& params) {
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MOOG FILTER");
    ImGui::Separator();

    float cutoff = params.get(P_FILTER_CUTOFF);
    float res    = params.get(P_FILTER_EMPHASIS);
    float amt    = params.get(P_FILTER_AMOUNT);

    if (ImGuiKnobs::Knob("Cutoff",&cutoff,0.f,1.f,0.005f,
            "%.2f",ImGuiKnobVariant_Wiper,52.f))
        params.set(P_FILTER_CUTOFF, cutoff);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Emphasis",&res,0.f,1.f,0.005f,
            "%.2f",ImGuiKnobVariant_Wiper,52.f))
        params.set(P_FILTER_EMPHASIS, res);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Env Amt",&amt,0.f,1.f,0.005f,
            "%.2f",ImGuiKnobVariant_Wiper,52.f))
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
}

// ════════════════════════════════════════════════════════
// PUBLIC ENTRY POINT
// ════════════════════════════════════════════════════════

void render(AtomicParamStore& params) {
    // Set a sensible initial size; user can resize the outer window freely
    const float winW = COL1_W + COL2_W + COL3_W
                       + COL_GAP * 2.f
                       + ImGui::GetStyle().WindowPadding.x * 2.f
                       + ImGui::GetStyle().ScrollbarSize;
    const float winH = COL_H
                       + ImGui::GetFrameHeight()       // title bar
                       + ImGui::GetStyle().WindowPadding.y * 2.f
                       + ImGui::GetFrameHeight() + 6.f; // footer

    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_FirstUseEver);
    ImGui::Begin("Mini Moog Model D Engine");

    // ── Column 1: Controllers (fixed 215 × 510 px) ───────
    ImGui::BeginChild("##col_ctrl",
                      ImVec2(COL1_W, COL_H), true);

    if (ImGui::CollapsingHeader("Controllers",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        renderControllers(params);
    }

    ImGui::EndChild();  // col_ctrl
    ImGui::SameLine(0.f, COL_GAP);

    // ── Column 2: Oscillators + Mixer (fixed 400 × 510 px) ─
    ImGui::BeginChild("##col_mid",
                      ImVec2(COL2_W, COL_H), true);

    if (ImGui::CollapsingHeader("Oscillators",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        renderOscillators(params);
        ImGui::Spacing();
    }

    if (ImGui::CollapsingHeader("Mixer",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Spacing();
        renderMixer(params);
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
        renderFilterAndEnvelopes(params);
    }

    ImGui::EndChild();  // col_filter

    // ── Footer ──────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MINI MOOG MODEL D");

    ImGui::End();
}

} // namespace PanelEngine
