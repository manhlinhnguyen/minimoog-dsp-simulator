// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_chord_scale.cpp
// BRIEF: Chord Engine + Scale Quantizer panel
// ─────────────────────────────────────────────────────────
#include "panel_chord_scale.h"
#include "imgui.h"
#include "shared/params.h"
#include "core/music/chord_engine.h"
#include "core/music/scale_quantizer.h"

namespace PanelChordScale {

void render(AtomicParamStore& p) {
    ImGui::Begin("Chord & Scale");

    // ── Chord Engine ──────────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "CHORD ENGINE");
    ImGui::Separator();

    bool chordOn = p.get(P_CHORD_ON) > 0.5f;
    if (ImGui::Checkbox("Enable##chord", &chordOn))
        p.set(P_CHORD_ON, chordOn ? 1.f : 0.f);

    ImGui::BeginDisabled(!chordOn);

    // Build chord name list
    const char* chordNames[ChordEngine::CHORD_COUNT];
    for (int i = 0; i < ChordEngine::CHORD_COUNT; ++i)
        chordNames[i] = ChordEngine::CHORDS[i].name;

    int chordType = static_cast<int>(p.get(P_CHORD_TYPE));
    ImGui::SetNextItemWidth(120.f);
    if (ImGui::Combo("Chord", &chordType, chordNames,
                     ChordEngine::CHORD_COUNT))
        p.set(P_CHORD_TYPE, static_cast<float>(chordType));

    const char* invNames[] = {"Root","1st","2nd","3rd"};
    int inv = static_cast<int>(p.get(P_CHORD_INVERSION));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("Inversion", &inv, invNames, 4))
        p.set(P_CHORD_INVERSION, static_cast<float>(inv));

    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ── Scale Quantizer ───────────────────────────────
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "SCALE QUANTIZER");
    ImGui::Separator();

    bool scaleOn = p.get(P_SCALE_ON) > 0.5f;
    if (ImGui::Checkbox("Enable##scale", &scaleOn))
        p.set(P_SCALE_ON, scaleOn ? 1.f : 0.f);

    ImGui::BeginDisabled(!scaleOn);

    const char* roots[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    int root = static_cast<int>(p.get(P_SCALE_ROOT));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("Root", &root, roots, 12))
        p.set(P_SCALE_ROOT, static_cast<float>(root));

    const char* scaleNames[ScaleQuantizer::SCALE_COUNT];
    for (int i = 0; i < ScaleQuantizer::SCALE_COUNT; ++i)
        scaleNames[i] = ScaleQuantizer::SCALES[i].name;

    int scaleType = static_cast<int>(p.get(P_SCALE_TYPE));
    ImGui::SetNextItemWidth(130.f);
    if (ImGui::Combo("Scale", &scaleType, scaleNames,
                     ScaleQuantizer::SCALE_COUNT))
        p.set(P_SCALE_TYPE, static_cast<float>(scaleType));

    ImGui::EndDisabled();
    ImGui::End();
}

} // namespace PanelChordScale
