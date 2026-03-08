# MiniMoog DSP Simulator — V2.2

A high-fidelity multi-engine DSP synthesizer workstation built from scratch in C++17. Six independent synthesis engines share one real-time effect chain and one music layer (Arp/Seq/Chord/Scale), all rendered through an ImGui desktop interface. The DSP core is designed to port cleanly to Teensy 4.1 embedded hardware (V3) by swapping only the HAL layer.

---

## Engines

| # | Engine | Paradigm | Voices | Params |
|---|--------|----------|--------|--------|
| 1 | **MiniMoog Model D** | Subtractive (3 VCO → 4-pole ladder filter → VCA) | 1–8 (Mono/Poly/Unison) | 42 |
| 2 | **Hammond B-3** | Additive tonewheel (9 drawbars) + Leslie dual-rotor | 61 | 22 |
| 3 | **Rhodes Mark I** | Modal resonator physical model (Tolonen 1998) | 12 | 11 |
| 4 | **Yamaha DX7** | 6-operator FM, 32 algorithms, exponential ADSR | 8 | 63 |
| 5 | **Mellotron M400** | 8-cycle wavetable + tape wow/flutter emulation | 35 | 11 |
| 6 | **Hybrid Drum Machine** | DSP synthesis (808-style) + WAV sample pads | 16 pads | 67 |

Switch engines at runtime — no audio interruption. All engines share the music layer and effect chain.

---

## Features

### DSP Engines

**MiniMoog Model D**
- 3 analog oscillators — Triangle, TriSaw, RevSaw, Sawtooth, Square, Pulse (PolyBLEP)
- Range switching: LO / 32' / 16' / 8' / 4' / 2'; OSC3 dual-mode (audio or free LFO)
- 4-pole Moog Ladder Filter with tanh saturation, ADSR, keyboard tracking, ModMatrix
- Voice modes: Mono (Last/Lowest/Highest priority), Poly (3 steal strategies), Unison (analog jitter detune)
- Post-VCA tanh soft limiter — prevents clipping on unison stacking

**Hammond B-3**
- 9-drawbar additive synthesis (61 voices), 9× ParamSmoother (no zipper noise)
- First-note percussion (2nd/3rd harmonic); key-click transient; V1–V3/C1–C3 vibrato/chorus
- True Leslie dual-rotor: horn (0.8/6.2 Hz) + drum (0.67/5.6 Hz), Doppler delay + AM shading, mechanical inertia spin-up/down
- Tube overdrive pre-Leslie: tanh 1×–5×, smoothed — Jon Lord to Jimmy Smith

**Rhodes Mark I**
- 4-mode biquad modal resonator with inharmonic ratios from Tolonen (1998): 1.000×, 2.045×, 5.979×, 9.214×
- Displacement-based pickup polynomial: y = x + α·x² + β·x³ (velocity × tone → 4× timbre range)
- Per-mode hammer excitation scaling; torsional mode AM wobble; 2-stage damper release (30ms felt + user release)
- Sympathetic resonance (unison/5th/4th coupling); vibrato LFO ±35 cents; CC64 sustain pedal
- Cabinet EQ (+2dB@200Hz, −8dB@8kHz); 3× ParamSmoother on Tone/Decay/Drive

**Yamaha DX7**
- Full 6-operator FM with all 32 original algorithms
- Exponential ADSR envelopes — "snap" attack, plunge-and-tail decay
- Fixed-frequency operators (Hz mode) — inharmonic bells, metallic FM, drum synthesis
- Keyboard rate scaling per operator — high notes decay faster (natural piano behavior)
- Oldest-active voice stealing — pop-free at polyphony limit

**Mellotron M400**
- 8-cycle wavetable per tape (16384 samples) with per-cycle harmonic jitter ±8–12%
- Engine-level dual LFO: wow (0.1–3 Hz) + flutter (6–18 Hz) — shared transport (one capstan)
- Tape hiss floor −55dBFS, voice-count-scaled; tape runout envelope (2–12s)
- 4 tape types: Strings, Choir, Flute, Brass

**Hybrid Drum Machine**
- 808-style hi-hat: 6 oscillators ring-modulated in pairs (ratios: 1.000, 1.4471, 1.6818, 1.9545, 2.2727, 2.6364) + 30% noise
- Kick: parameterized exponential pitch sweep — depth (0–1) and time (10–200ms) exposed
- Hi-hat choke group (open ↔ close mutual kill); velocity → amplitude + decay scaling
- 8 WAV sample pads (pads 8–15) loaded at startup from `assets/drum_kits/default/`

### Effect Chain

Post-synth serial effect chain — up to 16 slots, add/remove/reorder freely:

| Effect | Description |
|--------|-------------|
| **Gain** | Boost / Overdrive / Distortion — mode, gain, asymmetry, tone |
| **Chorus** | Stereo modulated delay — depth, rate, voices, mix |
| **Flanger** | Jet-plane feedback delay — depth, rate, feedback, mix |
| **Phaser** | 4-stage all-pass — rate, depth, feedback, mix |
| **Tremolo** | Amplitude LFO — Sine / Triangle / Square, rate, depth |
| **Delay** | Stereo echo up to 2s — time, feedback, mix, BpmSync |
| **Reverb** | Schroeder comb+allpass — size, decay, damping, predelay, mix |
| **Equalizer** | 5-band shelving/peak: 80Hz / 320Hz / 1kHz / 3.5kHz / 10kHz, ±12dB |

EQ coefficients are double-buffered — no audio glitch during real-time knob moves.

### Music Layer

Shared across all 6 engines:
- **Arpeggiator** — Up / Down / Up-Down / Down-Up / Random / As-Played, 1–4 octaves, 8 rate divisions, gate and swing
- **Step Sequencer** — 16 steps, per-step note + velocity + gate + tie, swing, 8 rate divisions
- **Chord Engine** — 16 chord types, 4 inversions — one keypress triggers full chord
- **Scale Quantizer** — 16 scale types, 12 root notes — notes snap to scale degrees

### MIDI File Player

- Load and play Standard MIDI Files (SMF Type 0/1)
- Transport: Play / Pause / Stop / Seek
- MIDI file BPM synced to Arpeggiator/Sequencer tempo
- 10 classical MIDI files included (Bach, Beethoven, Chopin, Mozart, Tchaikovsky — public domain)

### Interface

- **Engine window** — RadioButton engine selector + per-engine controls
- **Music window** — Arp + Chord + Scale + Sequencer + Keyboard (5 tabs)
- **Effects window** — effect chain editor
- **Presets window** — Engine / Effect / Global tabs (Load below list, right-aligned)
- **MIDI Player window** — transport + file browser
- **Analysis windows** — Oscilloscope, Spectrum, Spectrogram, Lissajous, VU Meter, Correlation
- **Dark Moog theme** — warm amber on dark brown

### Stability & Safety

- **Lock-free MIDI queue** (SPSC ring buffer) — zero blocking between MIDI and audio threads
- **RT-safe audio thread** — no malloc, no mutex, no I/O in audio callback
- **EQ double-buffering** — biquad coefficients updated race-free
- **Stuck note prevention** — `allNotesOff()` on octave shift and window focus loss
- **Preset load safety** — `allNotesOff()` called before any preset load (prevents sustain=0 silence)
- **Static binary** — no runtime DLL dependencies on Windows

---

## Quick Start

### Windows (MSVC)

```bash
git clone <repo>
cd minimoog_digitwin

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

All dependencies (ImGui v1.90.4, RtAudio, RtMidi, imgui-knobs, nlohmann/json v3.11.3, Catch2 v3.5.2, GLFW 3.3.9) are fetched automatically via CMake FetchContent — no additional installation required.

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

1. Select an engine (default: MiniMoog Model D)
2. Press `]` to reach Octave 4–5
3. Press **V** — you should hear a tone
4. For Moog: adjust **Filter Cutoff** and **Amp ENV** for shape
5. Load a factory preset from the **Presets** window

---

## Factory Assets

**145 factory assets total:**

| Directory | Count | Description |
|-----------|-------|-------------|
| `assets/moog_presets/` | 10 | Bass, Lead, Pad, Brass, FX |
| `assets/hammond_presets/` | 10 | Jazz, Gospel, Rock, Soul, Church |
| `assets/rhodes_presets/` | 10 | Classic EP, Neo Soul, Jazz, Funk |
| `assets/dx7_presets/` | 10 | EP, Bells, Brass, Organ, Strings |
| `assets/mellotron_presets/` | 10 | Strings, Choir, Flute, Brass variations |
| `assets/global_presets/` | 35 | Cross-engine: engine + effect chain combined |
| `assets/effect_presets/` | 20 | Effect chain configurations |
| `assets/sequencer_patterns/` | 20 | Acid, Funk, Techno, Jazz, Bossa Nova, etc. |
| `assets/midi/` | 10 | Classical MIDI (Bach, Beethoven, Chopin…) |
| `assets/drum_kits/default/` | — | WAV samples for drum sample pads |

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
sim/main.cpp               Application entry point
ui/panels/engines/         Per-engine UI panels (Moog, Hammond, Rhodes, DX7, Mellotron, Drums)
ui/panels/controls/        Global controls (Effects, Music, MIDI Player, Presets, Output)
ui/panels/analysis/        Signal visualization (Oscilloscope, Spectrum, Lissajous, VU, Spectrogram, Correlation)
ui/widgets/                Reusable widgets (knob, ADSR, piano, seq grid)
hal/pc/                    RtAudio, RtMidi, GLFW kbd, JSON I/O, MIDI file loader, WAV loader
shared/                    types.h, params.h, interfaces.h (AtomicParamStore, MidiEventQueue)
core/dsp/                  RT-safe DSP primitives (Oscillator, MoogFilter, Envelope, LFO, …)
core/music/                Arp, Seq, Chord, Scale, MidiFilePlayer
core/engines/              IEngine interface + EngineManager + 6 engine implementations
core/effects/              EffectChain + 8 effect types (up to 16 serial slots)
core/util/                 SPSCQueue (lock-free), math_utils
tests/                     44 Catch2 unit tests
assets/                    Factory presets, patterns, MIDI files, drum samples
```

### Thread Model

| Thread | Role |
|--------|------|
| **UI** | ImGui render, param writes (`std::atomic<float>`), MIDI file load/control |
| **Audio** | `EngineManager::processBlock()` → MidiFilePlayer → drain SPSC queue → music layer → engine → effects → DAC |
| **MIDI** | RtMidi callback → push to SPSC `MidiEventQueue` |

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

## Documentation

| File | Description |
|------|-------------|
| `documents/MiniMoog DSP Simulator - TDD - V2.2.md` | Full Technical Design Document |
| `documents/DSP_Engine_Analysis_v2.md` | Post-improvement engine analysis + P4 roadmap |
| `documents/Engine_Improvements_Log.md` | P1/P2/P3 implementation log with code patterns |
| `documents/Huong_Dan_Su_Dung.md` | User manual (Vietnamese) |
| `documents/Preset_Reference_Guide.md` | All factory preset descriptions |
| `documents/Visualization_Guide.md` | Analysis panel usage guide |

---

## V3 Roadmap

- **Teensy 4.1 HAL** — `hal/teensy/`: I2S audio, SD card presets, rotary encoders
- **DX7 system LFO** — PM + AM modulation per operator
- **Moog hard sync** OSC1 → OSC2
- **Karplus-Strong oscillator** — plucked string / acoustic guitar
- **MIDI CC learn** — map any CC to any parameter
- **Velocity → filter/timbre** — Moog + DX7
- **Drum kit browser UI** — load WAV kits from `assets/drum_kits/` in the interface

---

*MiniMoog DSP Simulator V2.2 — March 2026*
