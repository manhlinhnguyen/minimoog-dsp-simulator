// ─────────────────────────────────────────────────────────
// FILE: sim/main.cpp
// BRIEF: MiniMoog DSP Simulator v2.2 — Multi-Engine entry point
// ─────────────────────────────────────────────────────────
#include <iostream>
#include <memory>

#include "shared/interfaces.h"
#include "shared/params.h"
#include "core/engines/engine_manager.h"
#include "core/engines/moog/moog_engine.h"
#include "core/engines/hammond/hammond_engine.h"
#include "core/engines/rhodes/rhodes_engine.h"
#include "core/engines/dx7/dx7_engine.h"
#include "core/engines/mellotron/mellotron_engine.h"
#include "core/engines/drums/drum_engine.h"
#include "core/effects/effect_chain.h"
#include "hal/pc/rtaudio_backend.h"
#include "hal/pc/pc_midi.h"
#include "hal/pc/drum_sample_loader.h"
#include "ui/imgui_app.h"

int main() {
    std::cout << "MiniMoog DSP Simulator v2.2 — Multi-Engine\n";

    // ── 1. Shared state ───────────────────────────────────
    AtomicParamStore globalParams;   // BPM + music layer params
    MidiEventQueue   midiQueue;

    // ── 2. Engine Manager + all engines ──────────────────
    EngineManager engineMgr;

    // Register engines (order = index in selector)
    // MoogEngine — default engine (index 0); owns its own param store
    engineMgr.registerEngine(std::make_unique<MoogEngine>());

    // Hammond B-3
    engineMgr.registerEngine(std::make_unique<HammondEngine>());

    // Rhodes Mark I (Modal Resonator + Pickup Nonlinearity)
    engineMgr.registerEngine(std::make_unique<RhodesEngine>());

    // Yamaha DX7
    engineMgr.registerEngine(std::make_unique<DX7Engine>());

    // Mellotron M400
    engineMgr.registerEngine(std::make_unique<MellotronEngine>());

    // Hybrid Drum Machine
    auto drumEng = std::make_unique<DrumEngine>();
    DrumSampleLoader::loadKitIntoEngine(*drumEng, "./drum_kits/default");
    engineMgr.registerEngine(std::move(drumEng));

    // Init EngineManager with global params + MIDI queue (must be before audio.open)
    engineMgr.init(&globalParams, &midiQueue);

    // ── 3. Effect chain ───────────────────────────────────
    EffectChain effectChain;
    effectChain.init(44100.f);

    // ── 4. Audio backend ──────────────────────────────────
    RtAudioBackend audio(engineMgr);
    RtAudioBackend::Config audioCfg;
    audioCfg.sampleRate = 44100;
    audioCfg.bufferSize = 256;

    if (!audio.open(audioCfg, &effectChain)) {
        std::cerr << "Audio open failed: " << audio.getLastError() << "\n";
        return 1;
    }
    if (!audio.start()) {
        std::cerr << "Audio start failed: " << audio.getLastError() << "\n";
        return 1;
    }
    std::cout << "Audio: " << audio.getSampleRate()
              << " Hz / "  << audio.getBufferSize() << " spl\n";

    // ── 5. MIDI input ─────────────────────────────────────
    PcMidi midi(midiQueue);
    if (midi.open())
        std::cout << "MIDI: port opened OK\n";
    else
        std::cout << "MIDI: " << midi.getLastError()
                  << " (continuing without MIDI)\n";

    // ── 6. UI (blocks until window closed) ───────────────
    ImGuiApp ui;
    ImGuiApp::Config uiCfg;
    uiCfg.windowW            = 1400;
    uiCfg.windowH            =  900;
    uiCfg.presetDir          = "./moog_presets";
    uiCfg.patternDir         = "./sequencer_patterns";
    uiCfg.effectPresetDir    = "./effect_presets";
    uiCfg.hammondPresetDir   = "./hammond_presets";
    uiCfg.rhodesPresetDir    = "./rhodes_presets";
    uiCfg.dx7PresetDir       = "./dx7_presets";
    uiCfg.mellotronPresetDir = "./mellotron_presets";
    uiCfg.drumKitDir         = "./drum_kits/default";
    uiCfg.globalPresetDir    = "./global_presets";
    uiCfg.midiDir            = "./midi";

    if (!ui.init(globalParams, engineMgr, midiQueue, effectChain, uiCfg)) {
        std::cerr << "UI init failed\n";
        return 1;
    }

    ui.run();

    // ── 7. Shutdown (reverse order) ───────────────────────
    audio.stop();
    midi.close();
    ui.shutdown();

    std::cout << "Bye.\n";
    return 0;
}
