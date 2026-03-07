// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_music.cpp
// BRIEF: Combined Music panel — Keyboard, Arpeggiator,
//        Chord Engine, Scale Quantizer, Step Sequencer
//
// Tab-selection drives enable state:
//   Keyboard  → Arp=0, Chord=0, Scale=0, Seq=0
//   Arpeggiator → Arp=1, others=0
//   Chord       → Chord=1, others=0
//   Scale       → Scale=1, others=0
//   Sequencer   → Seq=1, others=0
// ─────────────────────────────────────────────────────────
#include "panel_music.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "ui/widgets/seq_display.h"
#include "ui/widgets/keyboard_display.h"
#include "shared/params.h"
#include "core/music/arpeggiator.h"
#include "core/music/chord_engine.h"
#include "core/music/scale_quantizer.h"
#include <array>
#include <cstring>
#include <string>

namespace PanelMusic {

// ── Sequencer local state ─────────────────────────────────
static std::array<SeqStep, StepSequencer::MAX_STEPS> s_steps;
static bool s_seqInitialized = false;

// ── Tab: Keyboard & Play ──────────────────────────────────
static void renderKeyboard(AtomicParamStore& /*p*/,
                            KeyboardInput& kbd,
                            MidiEventQueue& midiQueue) {
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "QWERTY KEYBOARD");
    ImGui::TextDisabled("Z X C V B N M , . /  = white keys (C..E+1)");
    ImGui::TextDisabled("S D   G H J   L ;    = black keys");
    ImGui::TextDisabled("Q W E R T Y U I      = upper octave");
    ImGui::TextDisabled("[ = Oct Down    ] = Oct Up     (Oct: %d)",
                        kbd.getOctave());
    ImGui::Spacing();

    bool activeNotes[128] = {};
    kbd.getActiveNotes(activeNotes);
    const int baseNote = kbd.getOctave() * 12;

    KeyboardDisplay::draw(
        activeNotes, baseNote,
        ImVec2(ImGui::GetContentRegionAvail().x, 80.f),
        [&midiQueue](int note, bool on) {
            MidiEvent ev;
            ev.type  = on ? MidiEvent::Type::NoteOn
                          : MidiEvent::Type::NoteOff;
            ev.data1 = static_cast<uint8_t>(note);
            ev.data2 = on ? 100 : 0;
            midiQueue.push(ev);
        });
}

// ── Tab: Arpeggiator ─────────────────────────────────────
static void renderArp(AtomicParamStore& p) {
    ImGui::Spacing();

    const char* modes[] = {"Up","Down","Up/Down","Down/Up","Random","As Played"};
    int mode = static_cast<int>(p.get(P_ARP_MODE));
    ImGui::SetNextItemWidth(130.f);
    if (ImGui::Combo("Mode", &mode, modes, 6))
        p.set(P_ARP_MODE, static_cast<float>(mode));

    int octaves = static_cast<int>(p.get(P_ARP_OCTAVES));
    ImGui::SetNextItemWidth(100.f);
    if (ImGui::SliderInt("Octaves", &octaves, 1, 4))
        p.set(P_ARP_OCTAVES, static_cast<float>(octaves));

    const char* rates[] = {"1/1","1/2","1/4","1/8","1/16","3/8","3/16","1/4T"};
    int rate = static_cast<int>(p.get(P_ARP_RATE));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("Rate##arp", &rate, rates, 8))
        p.set(P_ARP_RATE, static_cast<float>(rate));

    ImGui::Spacing();

    float gate = p.get(P_ARP_GATE);
    if (ImGuiKnobs::Knob("Gate##arp", &gate, 0.01f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 55.f))
        p.set(P_ARP_GATE, gate);
    ImGui::SameLine();
    float swing = p.get(P_ARP_SWING);
    if (ImGuiKnobs::Knob("Swing##arp", &swing, 0.f, 0.5f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 55.f))
        p.set(P_ARP_SWING, swing);

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.f),
        "Hold notes on keyboard to arpeggiate");
}

// ── Tab: Chord ───────────────────────────────────────────
static void renderChord(AtomicParamStore& p) {
    ImGui::Spacing();

    const char* chordNames[ChordEngine::CHORD_COUNT];
    for (int i = 0; i < ChordEngine::CHORD_COUNT; ++i)
        chordNames[i] = ChordEngine::CHORDS[i].name;

    int chordType = static_cast<int>(p.get(P_CHORD_TYPE));
    ImGui::SetNextItemWidth(130.f);
    if (ImGui::Combo("Chord Type", &chordType, chordNames,
                     ChordEngine::CHORD_COUNT))
        p.set(P_CHORD_TYPE, static_cast<float>(chordType));

    const char* invNames[] = {"Root","1st Inv","2nd Inv","3rd Inv"};
    int inv = static_cast<int>(p.get(P_CHORD_INVERSION));
    ImGui::SetNextItemWidth(100.f);
    if (ImGui::Combo("Inversion", &inv, invNames, 4))
        p.set(P_CHORD_INVERSION, static_cast<float>(inv));

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.f),
        "Each key press expands to full chord");
}

// ── Tab: Scale Quantizer ─────────────────────────────────
static void renderScale(AtomicParamStore& p) {
    ImGui::Spacing();

    const char* roots[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    int root = static_cast<int>(p.get(P_SCALE_ROOT));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("Root", &root, roots, 12))
        p.set(P_SCALE_ROOT, static_cast<float>(root));

    const char* scaleNames[ScaleQuantizer::SCALE_COUNT];
    for (int i = 0; i < ScaleQuantizer::SCALE_COUNT; ++i)
        scaleNames[i] = ScaleQuantizer::SCALES[i].name;

    int scaleType = static_cast<int>(p.get(P_SCALE_TYPE));
    ImGui::SetNextItemWidth(140.f);
    if (ImGui::Combo("Scale", &scaleType, scaleNames,
                     ScaleQuantizer::SCALE_COUNT))
        p.set(P_SCALE_TYPE, static_cast<float>(scaleType));

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.f),
        "Notes snap to nearest scale degree");
}

// ── Pattern browser state ─────────────────────────────────
static std::vector<std::string> s_patList;
static int   s_patSelected  = -1;
static char  s_patSaveName[64] = "MyPattern";

// ── Tab: Sequencer ───────────────────────────────────────
static void renderSeq(AtomicParamStore& p, SynthEngine& engine,
                      PatternStorage& patterns) {
    if (!s_seqInitialized) {
        for (int i = 0; i < StepSequencer::MAX_STEPS; ++i)
            s_steps[i] = engine.getSeqStep(i);
        s_seqInitialized = true;
    }

    ImGui::Spacing();

    bool playing = p.get(P_SEQ_PLAYING) > 0.5f;
    if (ImGui::Button(playing ? "Stop##seq" : "Play##seq"))
        p.set(P_SEQ_PLAYING, playing ? 0.f : 1.f);
    ImGui::SameLine(0.f, 10.f);
    if (ImGui::Button("Clear##seq")) {
        for (int i = 0; i < StepSequencer::MAX_STEPS; ++i) {
            s_steps[i].active = false;
            engine.setSeqStep(i, s_steps[i]);
        }
    }

    // ── Pattern Library ───────────────────────────────────
    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Pattern Library", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (s_patList.empty())
            s_patList = patterns.listPatterns();

        ImGui::BeginChild("##patlist", ImVec2(0.f, 90.f), true);
        for (int i = 0; i < static_cast<int>(s_patList.size()); ++i) {
            if (ImGui::Selectable(s_patList[i].c_str(), i == s_patSelected))
                s_patSelected = i;
        }
        ImGui::EndChild();

        ImGui::BeginDisabled(s_patSelected < 0 ||
            s_patSelected >= static_cast<int>(s_patList.size()));
        if (ImGui::Button("Load##pat")) {
            SeqPattern pat;
            if (patterns.loadPattern(s_patList[s_patSelected], pat)) {
                for (int i = 0; i < StepSequencer::MAX_STEPS; ++i) {
                    s_steps[i] = pat.steps[i];
                    engine.setSeqStep(i, pat.steps[i]);
                }
                p.set(P_SEQ_STEPS, static_cast<float>(pat.stepCount));
                p.set(P_SEQ_RATE,  static_cast<float>(pat.rateIdx));
                p.set(P_SEQ_GATE,  pat.gate);
                p.set(P_SEQ_SWING, pat.swing);
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine(0.f, 8.f);
        if (ImGui::Button("Refresh##pat")) {
            s_patList = patterns.listPatterns();
            s_patSelected = -1;
        }

        ImGui::SameLine(0.f, 16.f);
        ImGui::SetNextItemWidth(110.f);
        ImGui::InputText("##patname", s_patSaveName, sizeof(s_patSaveName));
        ImGui::SameLine();
        if (ImGui::Button("Save##pat")) {
            SeqPattern pat;
            pat.name      = s_patSaveName;
            pat.category  = "User";
            pat.stepCount = static_cast<int>(p.get(P_SEQ_STEPS));
            pat.rateIdx   = static_cast<int>(p.get(P_SEQ_RATE));
            pat.gate      = p.get(P_SEQ_GATE);
            pat.swing     = p.get(P_SEQ_SWING);
            for (int i = 0; i < StepSequencer::MAX_STEPS; ++i)
                pat.steps[i] = s_steps[i];
            patterns.savePattern(pat, std::string(s_patSaveName) + ".json");
            s_patList = patterns.listPatterns();
        }

        if (!patterns.getLastError().empty())
            ImGui::TextColored(ImVec4(1.f,0.3f,0.3f,1.f),
                               "%s", patterns.getLastError().c_str());
    }
    ImGui::Separator();
    ImGui::Spacing();

    int steps = static_cast<int>(p.get(P_SEQ_STEPS));
    ImGui::SetNextItemWidth(120.f);
    if (ImGui::SliderInt("Steps##seq", &steps, 1, StepSequencer::MAX_STEPS))
        p.set(P_SEQ_STEPS, static_cast<float>(steps));

    const char* rates[] = {"1/1","1/2","1/4","1/8","1/16","3/8","3/16","1/4T"};
    int rate = static_cast<int>(p.get(P_SEQ_RATE));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("Rate##seqr", &rate, rates, 8))
        p.set(P_SEQ_RATE, static_cast<float>(rate));

    float gate = p.get(P_SEQ_GATE);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Gate##seqg", &gate, 0.01f, 1.f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 42.f))
        p.set(P_SEQ_GATE, gate);
    float swing = p.get(P_SEQ_SWING);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Swing##seqs", &swing, 0.f, 0.5f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper, 42.f))
        p.set(P_SEQ_SWING, swing);

    ImGui::Spacing();
    ImGui::TextDisabled("Click=on/off  Right-click=tie  Scroll=note");
    ImGui::Spacing();

    const std::array<SeqStep, StepSequencer::MAX_STEPS> before = s_steps;
    SeqDisplay::draw(s_steps.data(), steps, engine.getSeqStep(), p);
    for (int i = 0; i < steps; ++i) {
        if (std::memcmp(&s_steps[i], &before[i], sizeof(SeqStep)) != 0)
            engine.setSeqStep(i, s_steps[i]);
    }
}

// ── Entry point ──────────────────────────────────────────
void render(AtomicParamStore& params, SynthEngine& engine,
            PatternStorage& patterns, KeyboardInput& kbd,
            MidiEventQueue& midiQueue) {
    ImGui::Begin("Music");

    // Track which tab is active; tab selection drives enable state.
    static int s_activeTab     = 0;  // 0=Keyboard,1=Arp,2=Chord,3=Scale,4=Seq
    static int s_prevActiveTab = 0;
    int        s_newTab        = s_activeTab;

    if (ImGui::BeginTabBar("MusicTabs")) {
        if (ImGui::BeginTabItem("Keyboard")) {
            s_newTab = 0;
            renderKeyboard(params, kbd, midiQueue);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Arpeggiator")) {
            s_newTab = 1;
            renderArp(params);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Chord")) {
            s_newTab = 2;
            renderChord(params);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Scale")) {
            s_newTab = 3;
            renderScale(params);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Sequencer")) {
            s_newTab = 4;
            renderSeq(params, engine, patterns);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // On tab change: silence everything immediately
    if (s_newTab != s_prevActiveTab) {
        kbd.allNotesOff();                    // release QWERTY-held notes
        MidiEvent cc;
        cc.type  = MidiEvent::Type::ControlChange;
        cc.data1 = 123;                       // All Notes Off
        cc.data2 = 0;
        midiQueue.push(cc);
        // Also stop sequencer transport when leaving Sequencer tab
        if (s_prevActiveTab == 4)
            params.set(P_SEQ_PLAYING, 0.f);
    }
    s_activeTab     = s_newTab;
    s_prevActiveTab = s_newTab;

    // Apply enable state based on active tab
    params.set(P_ARP_ON,   s_activeTab == 1 ? 1.f : 0.f);
    params.set(P_CHORD_ON, s_activeTab == 2 ? 1.f : 0.f);
    params.set(P_SCALE_ON, s_activeTab == 3 ? 1.f : 0.f);
    params.set(P_SEQ_ON,   s_activeTab == 4 ? 1.f : 0.f);

    ImGui::End();
}

} // namespace PanelMusic
