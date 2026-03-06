// ─────────────────────────────────────────────────────────
// FILE: ui/imgui_app.cpp
// BRIEF: ImGui application shell implementation
// ─────────────────────────────────────────────────────────
#include "imgui_app.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/gl.h>

#include "panels/panel_controllers.h"
#include "panels/panel_oscillators.h"
#include "panels/panel_mixer.h"
#include "panels/panel_modifiers.h"
#include "panels/panel_output.h"
#include "panels/panel_music.h"
#include "panels/panel_oscilloscope.h"
#include "panels/panel_presets.h"
#include "ui/widgets/keyboard_display.h"
#include "imgui-knobs.h"

bool ImGuiApp::init(AtomicParamStore& params,
                     SynthEngine&      engine,
                     MidiEventQueue&   midiQueue,
                     Config            cfg) {
    params_    = &params;
    engine_    = &engine;
    midiQueue_ = &midiQueue;
    cfg_       = cfg;

    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(cfg_.windowW, cfg_.windowH,
                               cfg_.title, nullptr, nullptr);
    if (!window_) { glfwTerminate(); return false; }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename  = "minimoog_layout.ini";

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    applyDarkMoogTheme();

    kbdInput_.init(window_, *midiQueue_);
    presetStorage_.setDirectory(cfg_.presetDir);
    patternStorage_.setDirectory(cfg_.patternDir);

    return true;
}

void ImGuiApp::run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        kbdInput_.update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderMainMenuBar();
        renderAllPanels();
        renderStatusBar();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);
    }
}

void ImGuiApp::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

bool ImGuiApp::shouldClose() const noexcept {
    return window_ && glfwWindowShouldClose(window_);
}

void ImGuiApp::renderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Controllers", nullptr, &showControlPanel_);
            ImGui::MenuItem("Oscillators", nullptr, &showOscPanel_);
            ImGui::MenuItem("Mixer",       nullptr, &showMixPanel_);
            ImGui::MenuItem("Filter/Env",  nullptr, &showFilterPanel_);
            ImGui::MenuItem("Music",       nullptr, &showMusicPanel_);
            ImGui::MenuItem("Oscilloscope",nullptr, &showScopePanel_);
            ImGui::MenuItem("Output",      nullptr, &showOutputPanel_);
            ImGui::MenuItem("Keyboard",    nullptr, &showKeyboardPanel_);
            ImGui::MenuItem("Presets",     nullptr, &showPresetPanel_);
            ImGui::Separator();
            ImGui::MenuItem("Debug",       nullptr, &showDebugPanel_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::Text("MiniMoog DSP Simulator v1.0");
            ImGui::Text("Keyboard: Z-M = C..B, Q-U = C+1..B+1");
            ImGui::Text("[/] = Octave Down/Up");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ImGuiApp::renderStatusBar() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float barH = ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos(
        ImVec2(vp->Pos.x, vp->Pos.y + vp->Size.y - barH));
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, barH));
    ImGui::SetNextWindowBgAlpha(0.85f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs     |
        ImGuiWindowFlags_NoNav        |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##statusbar", nullptr, flags)) {
        ImGui::Text("Voices: %d/%d  |  BPM: %.0f  |  Octave: %d",
            engine_->getActiveVoices(),
            static_cast<int>(params_->get(P_VOICE_COUNT)),
            params_->get(P_BPM),
            kbdInput_.getOctave());
    }
    ImGui::End();
}

void ImGuiApp::renderAllPanels() {
    if (showControlPanel_) PanelControllers::render(*params_);
    if (showOscPanel_)     PanelOscillators::render(*params_);
    if (showMixPanel_)     PanelMixer::render(*params_);
    if (showFilterPanel_)  PanelModifiers::render(*params_);
    if (showMusicPanel_)   PanelMusic::render(*params_, *engine_, patternStorage_);
    if (showScopePanel_)   PanelOscilloscope::render(*engine_);
    if (showOutputPanel_)  PanelOutput::render(*params_);
    if (showPresetPanel_)  PanelPresets::render(*params_, *engine_,
                                                presetStorage_);

    // ── Keyboard / Play panel ────────────────────────────────────────────
    if (showKeyboardPanel_) {
        ImGui::Begin("Keyboard & Play");

        ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "MASTER VOLUME");
        float vol = params_->get(P_MASTER_VOL);
        if (ImGuiKnobs::Knob("Volume", &vol, 0.f, 1.f, 0.005f,
                              "%.2f", ImGuiKnobVariant_Wiper, 55.f))
            params_->set(P_MASTER_VOL, vol);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "QWERTY KEYBOARD");
        ImGui::TextDisabled("Z X C V B N M , . /  = white keys (C..E+1)");
        ImGui::TextDisabled("S D   G H J   L ;    = black keys");
        ImGui::TextDisabled("Q W E R T Y U I      = upper octave");
        ImGui::TextDisabled("[ = Oct Down    ] = Oct Up     (Oct: %d)",
                            kbdInput_.getOctave());
        ImGui::Spacing();

        // Clickable visual keyboard — lit keys follow QWERTY input
        bool activeNotes[128] = {};
        kbdInput_.getActiveNotes(activeNotes);
        const int baseNote = kbdInput_.getOctave() * 12;

        KeyboardDisplay::draw(
            activeNotes, baseNote,
            ImVec2(ImGui::GetContentRegionAvail().x, 80.f),
            [this](int note, bool on) {
                MidiEvent ev;
                ev.type  = on ? MidiEvent::Type::NoteOn
                              : MidiEvent::Type::NoteOff;
                ev.data1 = static_cast<uint8_t>(note);
                ev.data2 = on ? 100 : 0;
                midiQueue_->push(ev);
            });

        ImGui::End();
    }

    if (showDebugPanel_) {
        if (ImGui::Begin("Debug")) {
            ImGui::Text("Active voices: %d", engine_->getActiveVoices());
            ImGui::Text("Seq step     : %d", engine_->getSeqStep());
            ImGui::Text("Arp note     : %d", engine_->getArpNote());
        }
        ImGui::End();
    }
}

void ImGuiApp::applyDarkMoogTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding    = 4.0f;
    s.FrameRounding     = 3.0f;
    s.GrabRounding      = 3.0f;
    s.ScrollbarRounding = 3.0f;
    s.FramePadding      = ImVec2(6.f, 4.f);
    s.ItemSpacing       = ImVec2(8.f, 5.f);
    s.WindowPadding     = ImVec2(10.f, 10.f);

    auto* c = s.Colors;
    c[ImGuiCol_WindowBg]         = ImVec4(0.10f,0.10f,0.13f,1.f);
    c[ImGuiCol_TitleBg]          = ImVec4(0.08f,0.08f,0.10f,1.f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.18f,0.09f,0.02f,1.f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.16f,0.16f,0.20f,1.f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.24f,0.20f,0.12f,1.f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.30f,0.22f,0.04f,1.f);
    c[ImGuiCol_Button]           = ImVec4(0.20f,0.12f,0.02f,1.f);
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.70f,0.40f,0.00f,1.f);
    c[ImGuiCol_ButtonActive]     = ImVec4(0.85f,0.50f,0.00f,1.f);
    c[ImGuiCol_SliderGrab]       = ImVec4(0.85f,0.50f,0.00f,1.f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(1.00f,0.65f,0.10f,1.f);
    c[ImGuiCol_Header]           = ImVec4(0.25f,0.15f,0.02f,1.f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.55f,0.32f,0.02f,1.f);
    c[ImGuiCol_HeaderActive]     = ImVec4(0.75f,0.45f,0.02f,1.f);
    c[ImGuiCol_CheckMark]        = ImVec4(1.00f,0.65f,0.10f,1.f);
    c[ImGuiCol_Text]             = ImVec4(0.92f,0.88f,0.80f,1.f);
    c[ImGuiCol_TextDisabled]     = ImVec4(0.50f,0.48f,0.44f,1.f);
    c[ImGuiCol_Separator]        = ImVec4(0.35f,0.25f,0.08f,1.f);
    c[ImGuiCol_Tab]              = ImVec4(0.14f,0.10f,0.04f,1.f);
    c[ImGuiCol_TabHovered]       = ImVec4(0.60f,0.36f,0.02f,1.f);
    c[ImGuiCol_TabActive]        = ImVec4(0.40f,0.24f,0.02f,1.f);
}
