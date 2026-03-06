# MiniMoog DSP Simulator — V1

A high-fidelity software emulation of the **Minimoog Model D** synthesizer, built from scratch in C++17 with a real-time DSP engine, ImGui interface, and a clean layered architecture designed for portability to embedded hardware (Teensy 4.1).

---

## Screenshots

```
┌──────────────────────────────────────────────────────────────────────┐
│  [View ▼]  [Help ▼]                                                  │
├──────┬────────────────┬────────┬──────────────────────────────────── │
│      │                │        │                                      │
│  Controllers  │  Oscillators  │  Mixer  │  Filter & Envelopes        │
│  Glide        │  OSC 1/2/3    │ Levels  │  Cutoff / Emphasis         │
│  Mod LFO      │  Range/Wave   │ Noise   │  Filter ADSR / Amp ADSR    │
│  BPM          │  Freq/Tune    │         │                             │
│  Polyphony    │               │         │                             │
├──────┴────────────────┴────────┴─────────────────────────────────── │
│  Music [Arp | Chord | Scale | Sequencer]   Oscilloscope  ~~~~       │
│  Presets  │  Output  │  Keyboard & Play [piano keys]                 │
├───────────────────────────────────────────────────────────────────── │
│  Voices: 1/1  |  BPM: 120  |  Octave: 4                             │
└──────────────────────────────────────────────────────────────────────┘
```

---

## Features

### DSP Engine
- **3 analog oscillators** — Triangle, TriSaw, RevSaw, Sawtooth, Square, Pulse waveforms
- **Range switching** — LO (sub-audio LFO), 32', 16', 8', 4', 2'
- **OSC3 dual mode** — audio oscillator or free-running LFO
- **4-pole Moog Ladder Filter** — low-pass with resonance (self-oscillates at ~0.9), tanh saturation
- **Filter modulation** — keyboard tracking (Off / 1/3 / 2/3), filter envelope amount
- **Two independent ADSR envelopes** — Filter envelope and Amplitude envelope
- **Glide / Portamento** — smooth pitch slide between notes
- **LFO modulation** — OSC pitch mod and filter cutoff mod via ModMatrix
- **Noise generator** — White and Pink noise with mixer level control
- **ParamSmoother** on every continuous parameter — eliminates zipper noise

### Voice Architecture
- **Mono** — classic single-voice Minimoog mode with portamento
- **Poly** — up to 8 simultaneous voices with configurable voice stealing (Oldest / Lowest / Quietest)
- **Unison** — all voices stacked on one note with per-voice detuning

### Music Features
- **Arpeggiator** — Up / Down / Up-Down / Down-Up / Random / As-Played modes, 1–4 octaves, 8 rate divisions, Gate and Swing
- **Step Sequencer** — 16 steps, per-step note + velocity + gate + tie, swing, 8 rate divisions
- **Chord Engine** — 16 chord types, 4 inversions — one keypress triggers full chord
- **Scale Quantizer** — 16 scale types, 12 root notes — notes snap to scale degrees

### Interface
- **9 floating panels** — freely arrangeable, show/hide via View menu
- **Software Oscilloscope** — triggered waveform display with auto-scale
- **Interactive piano keyboard** — click-to-play with QWERTY mapping, keys light up when pressed
- **Preset system** — load/save JSON presets with 20 factory presets included
- **Sequencer Pattern Library** — load/save JSON patterns with 10 classic patterns included
- **Dark Moog theme** — warm amber-on-dark-brown color scheme

### Stability
- **Lock-free MIDI queue** (SPSC ring buffer) — zero blocking between threads
- **Stuck note prevention** — automatic NoteOff on octave shift and window focus loss
- **Fully static binary** — no runtime DLL dependencies on Windows

---

## Quick Start

### Windows (MSVC)
```bash
git clone <repo>
cd minimog_digitwin

cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SIMULATOR=ON
cmake --build build --parallel

# Run
build\bin\Release\minimoog_sim.exe
```

### Linux
```bash
sudo apt install libglfw3-dev libopengl-dev

cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SIMULATOR=ON
cmake --build build --parallel

./build/bin/minimoog_sim
```

No additional installation required — all dependencies (ImGui, RtAudio, RtMidi, GLFW, nlohmann/json, imgui-knobs) are fetched automatically via CMake FetchContent.

---

## Playing

### QWERTY Keyboard Layout

```
[ S ][ D ]     [ G ][ H ][ J ]     [ L ][;]       ← black keys
[ Z ][ X ][ C ][ V ][ B ][ N ][ M ][,][.][/]      ← white keys (C–E+1)
  C    D    E    F    G    A    B   C+1 D+1 E+1

[ 2 ][ 3 ]     [ 5 ][ 6 ][ 7 ]                    ← upper black keys
[ Q ][ W ][ E ][ R ][ T ][ Y ][ U ][ I ]           ← upper white keys (C+1–C+2)

[  [  ] = Octave Down        [  ]  ] = Octave Up
```

Keys light up in the piano display as you press them.

### Getting Sound
1. Ensure **Master Volume** > 0 (Output panel or Keyboard & Play panel)
2. Ensure at least one OSC is **On** with **Mix** level > 0
3. Press `]` a few times to reach Octave 4–5
4. Press **V** (F4) — you should hear sound

---

## Factory Presets (20)

| Category | Name | Description |
|----------|------|-------------|
| Bass | bass_classic | Classic Minimoog bass |
| Bass | bass_fat | Fat filtered bass |
| Bass | bass_taurus | Moog Taurus deep sub |
| Bass | bass_funk | Percussive funk bass |
| Bass | bass_sub | Sub-frequency bass |
| Lead | lead_acid | Acid lead with filter sweep |
| Lead | lead_mellow | Soft mellow lead |
| Lead | lead_emerson | Keith Emerson / Rick Wakeman style |
| Lead | lead_lucky_man | ELP "Lucky Man" solo (1970) — filter wah |
| Lead | lead_prog_rock | Aggressive prog rock lead |
| Lead | lead_theremin | Theremin with LFO vibrato |
| Lead | lead_unison_fat | 6-voice unison detuned wall of sound |
| Pad | pad_warm | Warm evolving pad |
| Pad | pad_strings | String-like pad |
| Pad | pad_analog_strings | Thick detuned analog strings |
| Pad | pad_solar_wind | Atmospheric — OSC3 LFO filter sweep |
| Brass | brass_stab | Funk/rock brass hit |
| Keys | keys_pluck | Pluck-style keys |
| FX | fx_sweep | Filter sweep effect |
| Demo | arp_sequence | Arpeggiator demonstration |

---

## Factory Sequencer Patterns (10)

| Name | Category | Description |
|------|----------|-------------|
| acid_303 | Techno | Classic TB-303 acid bassline — C minor, slides & ties |
| funk_pocket | Funk | 16th-note syncopated funk bass in E, swing 0.08 |
| blue_monday | New Wave | New Order-inspired B minor driving bass |
| oxygene_arp | Electronic | Jean-Michel Jarre Oxygène IV — Cmaj7 arpeggio |
| donna_disco | Disco | Giorgio Moroder / "I Feel Love" hypnotic pulse |
| popcorn_melody | Classic Synth | Gershon Kingsley Popcorn (1969) melody |
| blues_shuffle | Blues | Walking blues in E, swing 0.2 |
| techno_pulse | Techno | Minimal single-note syncopated groove |
| pentatonic_run | Electronic | A minor pentatonic ascend/descend |
| reggae_offbeat | Reggae | C major off-beat skank, swing 0.12 |

---

## Running Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build --parallel
cd build && ctest --output-on-failure
```

**44 tests** covering oscillator, filter, envelope, glide, voice, arpeggiator, sequencer, and scale quantizer.

---

## Architecture Overview

```
sim/main.cpp          Application entry point
ui/                   ImGui panels and widgets
  imgui_app.cpp         Main app shell, render loop, GLFW window
  panels/               One .cpp per UI panel
  widgets/              Reusable widgets (ADSR display, SeqDisplay, KeyboardDisplay)
hal/pc/               PC platform implementation
  rtaudio_backend.*     Audio I/O via RtAudio
  pc_midi.*             MIDI I/O via RtMidi
  keyboard_input.*      QWERTY → MIDI, stuck-note prevention
  preset_storage.*      JSON preset load/save
  pattern_storage.*     JSON sequencer pattern load/save
shared/               Cross-layer types and interfaces
  types.h               MidiEvent, sample rate constants
  params.h              ParamID enum (49 params), PARAM_META table
  interfaces.h          AtomicParamStore, MidiEventQueue
core/dsp/             DSP primitives
  oscillator.*          6-waveform oscillator with phase accumulator
  moog_filter.*         4-pole Moog ladder filter (Huovilainen model)
  envelope.*            ADSR envelope
  glide.*               Exponential portamento
  lfo.*                 LFO (wraps Oscillator)
  noise.*               White / Pink noise generator
  param_smoother.*      1-pole LP smoother for zipper noise
core/voice/           Voice management
  voice.*               Single voice (OSC1+2+3 + Filter + Amp)
  voice_pool.*          Mono/Poly/Unison + voice stealing
core/music/           Musical features
  arpeggiator.*         6 modes, 4 octaves, swing
  sequencer.*           16-step with tie, per-step gate, swing
  chord_engine.*        16 chord types, 4 inversions
  scale_quantizer.*     16 scales, 12 roots
core/engine/          Top-level orchestration
  synth_engine.*        processBlock(), ModMatrix, param sync
core/util/
  spsc_queue.h          Lock-free single-producer single-consumer queue
  math_utils.h          midiNoteToHz(), fast math helpers
tests/                Catch2 unit tests (44 total)
assets/
  presets/              20 factory sound presets (.json)
  patterns/             10 factory sequencer patterns (.json)
documents/            Technical design doc + user manual
```

### Thread Safety Model

```
UI Thread    ──write──►  AtomicParamStore (std::atomic<float>[49])
                                │
                         Audio Thread reads each block

UI Thread    ──push──►  MidiEventQueue (SPSCQueue, lock-free)
MIDI Thread  ──push──►        │
                         Audio Thread drains each block
```

---

## Dependencies

All fetched automatically at configure time via CMake FetchContent:

| Library | Version | Use |
|---------|---------|-----|
| [Dear ImGui](https://github.com/ocornut/imgui) | v1.90.4 | UI rendering |
| [imgui-knobs](https://github.com/altschuler/imgui-knobs) | main | Rotary knob widget |
| [GLFW](https://github.com/glfw/glfw) | 3.3.9 | Window + OpenGL context |
| [RtAudio](https://github.com/thestk/rtaudio) | 6.0.1 | Cross-platform audio I/O |
| [RtMidi](https://github.com/thestk/rtmidi) | 6.0.0 | Cross-platform MIDI I/O |
| [nlohmann/json](https://github.com/nlohmann/json) | v3.11.3 | JSON preset/pattern files |
| [Catch2](https://github.com/catchorg/Catch2) | v3.5.2 | Unit testing |

---

## V2 Roadmap

The codebase is structured to make these additions clean to implement:

- **Chorus / Ensemble effect** — unlocks convincing strings, organ, electric piano
- **Reverb (FDN)** — essential realism for all instrument types
- **FM operator** — Rhodes electric piano, DX7-style bells and marimba
- **Karplus-Strong oscillator** — physically modeled plucked strings and guitar
- **Wavetable oscillator** — sample-based timbres including piano
- **Velocity → filter/timbre mapping** — currently velocity controls volume only
- **MIDI CC learn** — map any MIDI CC to any parameter
- **Teensy 4.1 HAL** — swap `hal/pc/` for `hal/teensy/` to run on embedded hardware

---

*MiniMoog DSP Simulator — V1 completed March 2026*
