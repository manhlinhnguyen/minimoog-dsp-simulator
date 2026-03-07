// ─────────────────────────────────────────────────────────
// FILE: ui/imgui_app.h
// BRIEF: ImGui application shell (window, render loop)
// ─────────────────────────────────────────────────────────
#pragma once
#include <GLFW/glfw3.h>
#include "shared/interfaces.h"
#include "shared/params.h"
#include "core/engine/synth_engine.h"
#include "hal/pc/moog_preset_storage.h"
#include "hal/pc/sequencer_pattern_storage.h"
#include "hal/pc/keyboard_input.h"
#include "hal/pc/effect_preset_storage.h"
#include "core/effects/effect_chain.h"

class ImGuiApp {
public:
    struct Config {
        int         windowW        = 1400;
        int         windowH        =  820;
        const char* title          = "MiniMoog DSP Simulator v1.0";
        const char* presetDir      = "./moog_presets";
        const char* patternDir     = "./sequencer_patterns";
        const char* effectPresetDir = "./effect_presets";
    };

    bool init(AtomicParamStore& params,
              SynthEngine&      engine,
              MidiEventQueue&   midiQueue,
              EffectChain&      effectChain,
              Config            cfg = {});

    void run();       // blocks until window closed
    void shutdown();

    bool shouldClose() const noexcept;

private:
    GLFWwindow*       window_       = nullptr;
    AtomicParamStore* params_       = nullptr;
    SynthEngine*      engine_       = nullptr;
    MidiEventQueue*   midiQueue_    = nullptr;
    EffectChain*      effectChain_  = nullptr;

    PresetStorage          presetStorage_;
    PatternStorage         patternStorage_;
    EffectPresetStorage    effectPresetStorage_;
    KeyboardInput          kbdInput_;
    Config                 cfg_;

    bool showEnginePanel_  = true;   // Controllers + Oscillators + Mixer + Filter & Envelopes
    bool showScopePanel_   = true;
    bool showMusicPanel_   = true;
    bool showPresetPanel_  = true;
    bool showOutputPanel_  = true;
    bool showEffectsPanel_ = true;
    bool showDebugPanel_   = false;

    void renderMainMenuBar();
    void renderStatusBar();
    void renderAllPanels();
    void applyDarkMoogTheme();
};
