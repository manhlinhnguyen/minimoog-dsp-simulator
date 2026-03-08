// ─────────────────────────────────────────────────────────
// FILE: ui/imgui_app.h
// BRIEF: ImGui application shell (window, render loop)
// ─────────────────────────────────────────────────────────
#pragma once
#include <GLFW/glfw3.h>
#include "shared/interfaces.h"
#include "shared/params.h"
#include "core/engines/engine_manager.h"
#include "hal/pc/sequencer_pattern_storage.h"
#include "hal/pc/keyboard_input.h"
#include "hal/pc/effect_preset_storage.h"
#include "hal/pc/engine_preset_storage.h"
#include "hal/pc/global_preset_storage.h"
#include "core/effects/effect_chain.h"
#include "ui/panels/controls/panel_midi_player.h"

class ImGuiApp {
public:
    struct Config {
        int         windowW              = 1400;
        int         windowH              =  900;
        const char* title                = "MiniMoog DSP Simulator v2.2";
        const char* presetDir            = "./moog_presets";
        const char* patternDir           = "./sequencer_patterns";
        const char* effectPresetDir      = "./effect_presets";
        const char* hammondPresetDir     = "./hammond_presets";
        const char* rhodesPresetDir      = "./rhodes_presets";
        const char* dx7PresetDir         = "./dx7_presets";
        const char* mellotronPresetDir   = "./mellotron_presets";
        const char* drumKitDir           = "./drum_kits/default";
        const char* globalPresetDir      = "./global_presets";
        const char* midiDir              = "./midi";
    };

    bool init(AtomicParamStore& globalParams,
              EngineManager&    engineMgr,
              MidiEventQueue&   midiQueue,
              EffectChain&      effectChain,
              Config            cfg = {});

    void run();
    void shutdown();

    bool shouldClose() const noexcept;

private:
    GLFWwindow*       window_       = nullptr;
    AtomicParamStore* params_       = nullptr;
    EngineManager*    engineMgr_    = nullptr;
    MidiEventQueue*   midiQueue_    = nullptr;
    EffectChain*      effectChain_  = nullptr;

    PatternStorage         patternStorage_;
    EffectPresetStorage    effectPresetStorage_;
    EnginePresetStorage    enginePresetStorage_;
    GlobalPresetStorage    globalPresetStorage_;
    KeyboardInput          kbdInput_;
    Config                 cfg_;

    PanelMidiPlayer::State midiPlayerState_;

    bool showEnginePanel_   = true;
    bool showEngineDetailPanel_ = true;
    bool showScopePanel_    = true;
    bool showMusicPanel_    = true;
    bool showPresetPanel_   = true;
    bool showOutputPanel_   = true;
    bool showEffectsPanel_  = true;
    bool showDebugPanel_    = false;
    // Visualization panels
    bool showSpectrumPanel_     = false;
    bool showLissajousPanel_    = false;
    bool showVuMeterPanel_      = false;
    bool showSpectrogramPanel_  = false;
    bool showCorrelationPanel_  = false;

    void renderMainMenuBar();
    void renderStatusBar();
    void renderAllPanels();
    void applyDarkMoogTheme();
};
