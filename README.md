# MiniMoog DSP Simulator — V2.1

A high-fidelity software emulation of the **Minimoog Model D** synthesizer, built from scratch in C++17 with a real-time DSP engine, post-synth effect chain, and ImGui interface. The architecture is layered for clean portability to embedded hardware (Teensy 4.1).

---

## Features

### DSP Engine
- **3 analog oscillators** — Triangle, TriSaw, RevSaw, Sawtooth, Square, Pulse with PolyBLEP anti-aliasing
- **Range switching** — LO (sub-audio LFO), 32', 16', 8', 4', 2'
- **OSC3 dual mode** — audio oscillator or free-running LFO
- **4-pole Moog Ladder Filter** — low-pass with resonance (self-oscillates at ~0.9), tanh saturation (Huovilainen model)
- **Filter modulation** — keyboard tracking (Off / 1/3 / 2/3), filter envelope amount
- **Two independent ADSR envelopes** — Filter envelope and Amplitude envelope
- **Glide / Portamento** — exponential pitch slide between notes
- **LFO modulation** — OSC pitch mod and filter cutoff mod via ModMatrix
- **Noise generator** — White and Pink noise with mixer level control
- **ParamSmoother** on every continuous parameter — eliminates zipper noise

### Voice Architecture
- **Mono** — classic single-voice Minimoog mode with portamento
- **Poly** — up to 8 simultaneous voices, configurable stealing (Oldest / Lowest / Quietest)
- **Unison** — all voices stacked on one note with per-voice detuning

### Effect Chain (V2)
Post-synth serial effect chain with up to 16 slots. Add, remove, and reorder effects freely:

| Effect | Description |
|--------|-------------|
| **Gain** | Boost / Overdrive / Distortion with asymmetry and tone control |
| **Chorus** | Stereo chorus — modulated delay voices |
| **Flanger** | Jet-plane flanger — short feedback delay |
| **Phaser** | 4-stage all-pass phaser |
| **Tremolo** | Amplitude tremolo — Sine / Triangle / Square LFO |
| **Delay** | Stereo echo delay up to 2 seconds |
| **Reverb** | Schroeder reverb — comb + all-pass |
| **Equalizer** | 5-band parametric EQ: 80 Hz / 320 Hz / 1 kHz / 3.5 kHz / 10 kHz, ±12 dB per band |

### Music Features
- **Arpeggiator** — Up / Down / Up-Down / Down-Up / Random / As-Played, 1–4 octaves, 8 rate divisions, Gate and Swing
- **Step Sequencer** — 16 steps, per-step note + velocity + gate + tie, swing, 8 rate divisions
- **Chord Engine** — 16 chord types, 4 inversions — one keypress triggers full chord
- **Scale Quantizer** — 16 scale types, 12 root notes — notes snap to scale degrees

### Interface
- **6 floating panels** — freely arrangeable, show/hide via View menu
- **Software Oscilloscope** — triggered waveform display, auto/manual scale toggle
- **Interactive piano keyboard** — click-to-play with QWERTY mapping
- **Moog Preset system** — load/save JSON sound presets (20 factory included)
- **Sequencer Pattern Library** — load/save JSON patterns (10 factory included)
- **Effect Preset system** — load/save full effect chain configurations (10 factory included)
- **Dark Moog theme** — warm amber-on-dark-brown color scheme

### Stability
- **Lock-free MIDI queue** (SPSC ring buffer) — zero blocking between threads
- **EQ coefficient double-buffering** — prevents audio glitch during real-time knob updates
- **Stuck note prevention** — automatic NoteOff on octave shift and window focus loss
- **Fully static binary** — no runtime DLL dependencies on Windows

---

## Quick Start

### Windows (MSVC)
```bash
git clone <repo>
cd minimog_digitwin

cmake -B build_sim -DCMAKE_BUILD_TYPE=Release -DBUILD_SIMULATOR=ON
cmake --build build_sim --parallel

build_sim\bin\Debug\minimoog_sim.exe
```

### Linux
```bash
sudo apt install libglfw3-dev libopengl-dev

cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SIMULATOR=ON
cmake --build build --parallel

./build/bin/minimoog_sim
```

All dependencies are fetched automatically via CMake FetchContent — no additional installation required.

---

## Playing

### QWERTY Keyboard Layout
```
[ S ][ D ]     [ G ][ H ][ J ]     [ L ][;]       ← black keys
[ Z ][ X ][ C ][ V ][ B ][ N ][ M ][,][.][/]      ← white keys (C–E+1)

[ Q ][ W ][ E ][ R ][ T ][ Y ][ U ][ I ]           ← upper octave

[  [  ] = Octave Down        [  ]  ] = Octave Up
```

### Getting Sound
1. Ensure at least one OSC is **On** with **Mix** level > 0
2. Press `]` to reach Octave 4–5
3. Press **V** — you should hear a sawtooth tone
4. Adjust **Filter Cutoff** and **Amp ENV** for shape

---

## Factory Assets

**20 Moog Presets:** bass_classic, bass_fat, bass_funk, bass_sub, bass_taurus, lead_acid, lead_emerson, lead_lucky_man, lead_mellow, lead_prog_rock, lead_theremin, lead_unison_fat, pad_analog_strings, pad_solar_wind, pad_strings, pad_warm, brass_stab, keys_pluck, fx_sweep, arp_sequence

**10 Sequencer Patterns:** acid_303, blue_monday, blues_shuffle, donna_disco, funk_pocket, oxygene_arp, pentatonic_run, popcorn_melody, reggae_offbeat, techno_pulse

**10 Effect Presets:** 01_clean_boost, 02_mild_overdrive, 03_heavy_distortion, 04_chorus_clean, 05_delay_reverb, 06_cathedral_shimmer, 07_summer_boost, 08_jet_flanger, 09_phaser_funk, 10_tremolo_warm

---

## Running Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build --parallel
cd build && ctest --output-on-failure
```

**44 tests** covering oscillator, filter, envelope, glide, voice, arpeggiator, sequencer, and scale quantizer.

---

## Architecture

```
sim/main.cpp              Application entry point (v2.1)
ui/panels/
  panel_moog_engine.*     Controllers + Oscillators + Mixer + Filter & Envelopes
  panel_music.*           Arp + Chord + Scale + Sequencer + Keyboard (5 tabs)
  panel_oscilloscope.*    Triggered waveform display (auto/manual scale)
  panel_output.*          Master Volume
  panel_presets.*         Moog Presets + Effect Presets (2 tabs)
  panel_effects.*         Effect chain editor (add/remove/reorder slots)
ui/widgets/               Reusable widgets (knob, ADSR, piano, seq grid)
hal/pc/
  moog_preset_storage.*   JSON sound preset save/load
  sequencer_pattern_storage.* JSON pattern save/load
  effect_preset_storage.* JSON effect chain save/load
  rtaudio_backend.*       Audio I/O
  pc_midi.*               MIDI I/O
  keyboard_input.*        QWERTY → MIDI
shared/
  params.h                103 ParamIDs + PARAM_META + norm helpers
  interfaces.h            AtomicParamStore, MidiEventQueue, IAudioProcessor
  types.h                 MidiEvent, HeldNote, VoiceMode
core/dsp/                 RT-SAFE DSP primitives
core/voice/               Mono/Poly/Unison voice management
core/music/               Arp, Seq, Chord, Scale
core/engine/              SynthEngine — top-level processBlock()
core/effects/             8 effects + EffectChain (up to 16 serial slots)
core/util/                SPSCQueue, math_utils
tests/                    44 Catch2 unit tests
assets/
  moog_presets/           20 factory sound presets
  sequencer_patterns/     10 factory sequencer patterns
  effect_presets/         10 factory effect chain presets
```

---

## Dependencies

| Library | Version | Use |
|---------|---------|-----|
| Dear ImGui | v1.90.4 | UI rendering |
| imgui-knobs | main | Rotary knob widget |
| GLFW | 3.3.9 | Window + OpenGL context |
| RtAudio | 6.0.1 | Cross-platform audio I/O |
| RtMidi | 6.0.0 | Cross-platform MIDI I/O |
| nlohmann/json | v3.11.3 | JSON preset / pattern / effect files |
| Catch2 | v3.5.2 | Unit testing |

---

## V3 Roadmap

- Teensy 4.1 HAL (`hal/teensy/`) — I2S audio, SD card presets
- FM operator routing — Rhodes EP, DX7-style bells
- Karplus-Strong oscillator — plucked string / acoustic guitar
- Wavetable oscillator — sample-based timbres
- Velocity → filter/timbre mapping
- MIDI CC learn
- Delay BpmSync wired to live BPM

---

*MiniMoog DSP Simulator V2.1 — March 2026*
