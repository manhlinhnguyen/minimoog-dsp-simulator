// ─────────────────────────────────────────────────────────
// FILE: ui/imgui_app.h
// BRIEF: ImGui application shell (window, render loop)
// ─────────────────────────────────────────────────────────
#pragma once
#include <GLFW/glfw3.h>
#include "shared/interfaces.h"
#include "shared/params.h"
#include "core/engine/synth_engine.h"
#include "hal/pc/preset_storage.h"
#include "hal/pc/pattern_storage.h"
#include "hal/pc/keyboard_input.h"

class ImGuiApp {
public:
    struct Config {
        int         windowW    = 1400;
        int         windowH    =  820;
        const char* title      = "MiniMoog DSP Simulator v1.0";
        const char* presetDir  = "./presets";
        const char* patternDir = "./patterns";
    };

    bool init(AtomicParamStore& params,
              SynthEngine&      engine,
              MidiEventQueue&   midiQueue,
              Config            cfg = {});

    void run();       // blocks until window closed
    void shutdown();

    bool shouldClose() const noexcept;

private:
    GLFWwindow*       window_    = nullptr;
    AtomicParamStore* params_    = nullptr;
    SynthEngine*      engine_    = nullptr;
    MidiEventQueue*   midiQueue_ = nullptr;

    PresetStorage     presetStorage_;
    PatternStorage    patternStorage_;
    KeyboardInput     kbdInput_;
    Config            cfg_;

    bool showControlPanel_ = true;
    bool showOscPanel_     = true;      // oscillators
    bool showScopePanel_   = true;      // oscilloscope
    bool showMixPanel_     = true;
    bool showFilterPanel_  = true;
    bool showMusicPanel_   = true;      // combined music panel
    bool showPresetPanel_  = true;
    bool showOutputPanel_  = true;
    bool showKeyboardPanel_= true;
    bool showDebugPanel_   = false;

    void renderMainMenuBar();
    void renderStatusBar();
    void renderAllPanels();
    void applyDarkMoogTheme();
};
