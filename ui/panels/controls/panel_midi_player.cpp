// ─────────────────────────────────────────────────────────
// FILE: ui/panels/controls/panel_midi_player.cpp
// BRIEF: MIDI file player transport panel implementation
// ─────────────────────────────────────────────────────────
#include "panel_midi_player.h"
#include "hal/pc/midi_file_loader.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>
#include <filesystem>

namespace PanelMidiPlayer {

// ── Helpers ───────────────────────────────────────────────

static void formatTime(float sec, char* buf, int sz) {
    const int m = static_cast<int>(sec) / 60;
    const int s = static_cast<int>(sec) % 60;
    std::snprintf(buf, sz, "%d:%02d", m, s);
}

// ── Init ─────────────────────────────────────────────────

void init(State& st, const char* midiDir) {
    if (midiDir && midiDir[0])
        std::strncpy(st.midiDir, midiDir, sizeof(st.midiDir) - 1);
    st.fileList   = MidiFileLoader::scan(st.midiDir);
    st.listScanned = true;
}

// ── Render ────────────────────────────────────────────────

static void renderContent(State& st, MidiFilePlayer& player) {

    // ── Rescan button ─────────────────────────────────────
    if (ImGui::Button("Rescan")) {
        player.stop();
        st.fileList    = MidiFileLoader::scan(st.midiDir);
        st.selectedIdx = -1;
    }
    ImGui::SameLine();
    ImGui::TextDisabled("Dir: %s  (%d files)",
                        st.midiDir,
                        static_cast<int>(st.fileList.size()));

    ImGui::Separator();

    // ── File list ─────────────────────────────────────────
    const float listH = ImGui::GetTextLineHeightWithSpacing() * 6.5f;
    if (ImGui::BeginListBox("##midfiles", ImVec2(-1, listH))) {
        for (int i = 0; i < static_cast<int>(st.fileList.size()); ++i) {
            const std::string name = MidiFileLoader::stem(st.fileList[i]);
            const bool sel = (i == st.selectedIdx);
            if (ImGui::Selectable(name.c_str(), sel)) {
                if (i != st.selectedIdx) {
                    player.stop();
                    // Load file
                    auto bytes = MidiFileLoader::read(st.fileList[i]);
                    if (!bytes.empty()) {
                        if (player.load(bytes.data(), bytes.size(),
                                        name.c_str())) {
                            st.selectedIdx = i;
                        }
                    }
                }
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }

    if (st.fileList.empty()) {
        ImGui::TextDisabled("No .mid files found in %s", st.midiDir);
    }

    ImGui::Separator();

    // ── Now playing ───────────────────────────────────────
    if (player.hasFile()) {
        ImGui::Text("Now playing: %s", player.fileName());
        ImGui::TextDisabled("BPM: %d", player.getFileBPM());
    } else {
        ImGui::TextDisabled("No file loaded");
    }

    ImGui::Spacing();

    // ── Transport buttons ─────────────────────────────────
    const bool hasFile  = player.hasFile();
    const bool playing  = player.isPlaying();
    const bool paused   = player.isPaused();

    // |< Rewind
    if (!hasFile) ImGui::BeginDisabled();
    if (ImGui::Button("|<##rew", ImVec2(36, 28))) {
        player.stop();
    }
    ImGui::SameLine();

    // > / ||
    const char* playLbl = playing ? "||##pp" : ">##pp";
    if (ImGui::Button(playLbl, ImVec2(44, 28))) {
        if (!playing && !paused) player.play();
        else                     player.pause();
    }
    ImGui::SameLine();

    // [] Stop
    if (ImGui::Button("[]##stop", ImVec2(36, 28))) {
        player.stop();
    }
    if (!hasFile) ImGui::EndDisabled();

    ImGui::SameLine(0, 14);

    // Loop toggle
    bool looping = player.isLooping();
    if (ImGui::Checkbox("Loop", &looping))
        player.setLoop(looping);

    ImGui::Spacing();

    // ── Position scrubber ─────────────────────────────────
    {
        float pos  = player.getPositionNorm();
        const float dur  = player.getDurationSec();
        const float cur  = player.getPositionSec();

        char tCur[16], tDur[16];
        formatTime(cur, tCur, sizeof(tCur));
        formatTime(dur, tDur, sizeof(tDur));

        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderFloat("##pos", &pos, 0.0f, 1.0f, "")) {
            if (player.isPlaying()) player.pause();
            player.seekNorm(pos);
        }
        // Release → resume if was playing before scrub
        // (simple: just let user press play again)

        ImGui::Text("%s / %s", tCur, tDur);
    }

    ImGui::Spacing();

    // ── Tempo scale ───────────────────────────────────────
    float scale = player.getTempoScale();
    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderFloat("Speed##tscale", &scale, 0.25f, 4.0f,
                           "%.2fx", ImGuiSliderFlags_Logarithmic))
        player.setTempoScale(scale);
    ImGui::SameLine();
    if (ImGui::Button("1x##rst")) player.setTempoScale(1.0f);
}

void render(State& st, MidiFilePlayer& player) {
    ImGui::SetNextWindowSize(ImVec2(420, 340), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("MIDI Player")) { ImGui::End(); return; }
    renderContent(st, player);
    ImGui::End();
}

void renderTab(State& st, MidiFilePlayer& player) {
    renderContent(st, player);
}

} // namespace PanelMidiPlayer
