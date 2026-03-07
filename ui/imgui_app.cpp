// ─────────────────────────────────────────────────────────
// FILE: ui/imgui_app.cpp
// BRIEF: ImGui application shell implementation
// ─────────────────────────────────────────────────────────
#include "imgui_app.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/gl.h>

#include "panels/panel_moog_engine.h"
#include "panels/panel_output.h"
#include "panels/panel_music.h"
#include "panels/panel_oscilloscope.h"
#include "panels/panel_presets.h"
#include "panels/panel_effects.h"

bool ImGuiApp::init(AtomicParamStore& params,
                     SynthEngine&      engine,
                     MidiEventQueue&   midiQueue,
                     EffectChain&      effectChain,
                     Config            cfg) {
    params_      = &params;
    engine_      = &engine;
    midiQueue_   = &midiQueue;
    effectChain_ = &effectChain;
    cfg_         = cfg;

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
    effectPresetStorage_.setDirectory(cfg_.effectPresetDir);

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
            ImGui::MenuItem("Engine",             nullptr, &showEnginePanel_);
            ImGui::MenuItem("Music",              nullptr, &showMusicPanel_);
            ImGui::MenuItem("Oscilloscope",nullptr, &showScopePanel_);
            ImGui::MenuItem("Output",      nullptr, &showOutputPanel_);
            ImGui::MenuItem("Presets",     nullptr, &showPresetPanel_);
            ImGui::MenuItem("Effects",     nullptr, &showEffectsPanel_);
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
    if (showEnginePanel_)  PanelEngine::render(*params_);
    if (showMusicPanel_)   PanelMusic::render(*params_, *engine_, patternStorage_,
                                              kbdInput_, *midiQueue_);
    if (showScopePanel_)   PanelOscilloscope::render(*engine_);
    if (showOutputPanel_)  PanelOutput::render(*params_);
    if (showPresetPanel_)  PanelPresets::render(*params_,
                                                presetStorage_,
                                                *effectChain_,
                                                effectPresetStorage_);
    if (showEffectsPanel_) PanelEffects::render(*effectChain_);

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
