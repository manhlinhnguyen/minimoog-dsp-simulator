// ─────────────────────────────────────────────────────────
// FILE: sim/main.cpp
// BRIEF: MiniMoog DSP Simulator — application entry point
// ─────────────────────────────────────────────────────────
#include <iostream>

#include "shared/interfaces.h"
#include "shared/params.h"
#include "core/engine/synth_engine.h"
#include "core/effects/effect_chain.h"
#include "hal/pc/rtaudio_backend.h"
#include "hal/pc/pc_midi.h"
#include "ui/imgui_app.h"

int main() {
    std::cout << "MiniMoog DSP Simulator v2.1\n";

    // ── 1. Shared state ───────────────────────────────
    AtomicParamStore params;
    MidiEventQueue   midiQueue;

    // ── 2. DSP Engine ─────────────────────────────────
    SynthEngine engine;
    engine.init(&params, &midiQueue);

    // ── 2b. Effect chain ──────────────────────────────
    EffectChain effectChain;
    effectChain.init(44100.f);

    // ── 3. Audio backend ──────────────────────────────
    RtAudioBackend audio(engine);
    RtAudioBackend::Config audioCfg;
    audioCfg.sampleRate = 44100;
    audioCfg.bufferSize = 256;

    if (!audio.open(audioCfg, &effectChain)) {
        std::cerr << "Audio open failed: "
                  << audio.getLastError() << "\n";
        return 1;
    }
    if (!audio.start()) {
        std::cerr << "Audio start failed: "
                  << audio.getLastError() << "\n";
        return 1;
    }
    std::cout << "Audio: " << audio.getSampleRate()
              << " Hz / "  << audio.getBufferSize() << " spl\n";

    // ── 4. MIDI input (best-effort, non-fatal) ────────
    PcMidi midi(midiQueue);
    if (midi.open())
        std::cout << "MIDI: port opened OK\n";
    else
        std::cout << "MIDI: " << midi.getLastError()
                  << " (continuing without MIDI)\n";

    // ── 5. UI (blocks until window closed) ───────────
    ImGuiApp ui;
    ImGuiApp::Config uiCfg;
    uiCfg.windowW         = 1400;
    uiCfg.windowH         =  820;
    uiCfg.presetDir       = "./moog_presets";
    uiCfg.patternDir      = "./sequencer_patterns";
    uiCfg.effectPresetDir = "./effect_presets";

    if (!ui.init(params, engine, midiQueue, effectChain, uiCfg)) {
        std::cerr << "UI init failed\n";
        return 1;
    }

    ui.run();

    // ── 6. Shutdown (reverse order) ───────────────────
    audio.stop();
    midi.close();
    ui.shutdown();

    std::cout << "Bye.\n";
    return 0;
}
