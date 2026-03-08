# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**MiniMoog DSP Simulator V2.2** — a cross-platform desktop app written in C++17 with a multi-engine synth architecture. Six engines (Moog, Hammond B-3, Rhodes, DX7, Mellotron, Drum Machine) share one Effect Chain and one Music Layer (Arp/Seq/Chord/Scale). The architecture is designed so the DSP Core can be ported to Teensy 4.1 (V3) by swapping only the HAL layer.

Full Technical Design Document: `documents/MiniMoog DSP Simulator - TDD - V2.2.md`
User Manual (Vietnamese): `documents/Huong_Dan_Su_Dung.md`

## Build Commands

```bash
# Configure (Release)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_SIMULATOR=ON

# Configure with tests
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_SIMULATOR=ON

# Build
cmake --build build --parallel $(nproc)

# Run simulator (assets auto-copied by post-build step)
./build/bin/minimoog_sim

# Run all tests
cd build && ctest --output-on-failure -j$(nproc)

# Run a single test by tag or name
./build/tests/minimoog_tests "[osc]"
./build/tests/minimoog_tests "MoogFilter*"
```

**Windows (MSVC) — primary development platform:**
```bash
cmake -B build_sim -DCMAKE_BUILD_TYPE=Release -DBUILD_SIMULATOR=ON
cmake --build build_sim --parallel
# Output: build_sim/bin/Debug/minimoog_sim.exe  (standalone, no DLLs needed)
```

All dependencies (ImGui v1.90.4, RtAudio, RtMidi, imgui-knobs, nlohmann/json v3.11.3, Catch2 v3.5.2, GLFW 3.3.9) are fetched automatically via CMake `FetchContent`. `BUILD_SHARED_LIBS=OFF` is forced — all deps link statically, no DLLs required at runtime.

On Linux also install: `libglfw3-dev libopengl-dev`

## Architecture

Strict layering — lower layers must never depend on higher ones:

```
APPLICATION   sim/main.cpp
      ↓
UI LAYER      ui/                      ImGui panels + widgets
      ↓
PC HAL        hal/pc/                  RtAudio, RtMidi, GLFW kbd, JSON I/O
      ↓
SHARED        shared/                  types, params, interfaces, AtomicParamStore
      ↓
DSP CORE      core/                    Pure C++17, zero platform dependency
  ├── dsp/     Oscillator, MoogFilter, Envelope, LFO, Glide, Noise, ParamSmoother
  ├── voice/   Voice, VoicePool (mono/poly/unison, voice stealing)
  ├── music/   Arpeggiator, Sequencer, ChordEngine, ScaleQuantizer
  ├── music/   Arpeggiator, Sequencer, ChordEngine, ScaleQuantizer, MidiFilePlayer
  ├── engines/ IEngine interface + EngineManager (music layer)
  │    ├── moog/       MoogEngine (subtractive: 3 VCO + ladder filter, 42 params)
  │    ├── hammond/    HammondB3Engine (9-drawbar tonewheel + Leslie dual-rotor, 22 params)
  │    ├── rhodes/     RhodesEngine (modal resonator physical model, Tolonen 1998, 11 params)
  │    ├── dx7/        DX7Engine (6-op FM, 32 algorithms, exp-ADSR, fixed-freq ops, 63 params)
  │    ├── mellotron/  MellotronEngine (8-cycle wavetable + wow/flutter + tape hiss, 11 params)
  │    └── drums/      DrumEngine (8 DSP pads 808-RM + 8 WAV sample pads, 67 params)
  ├── effects/ EffectChain + 8 effect types (Gain, Chorus, Flanger, Phaser,
  │            Tremolo, Delay, Reverb, Equalizer)
  └── util/    SPSCQueue (lock-free), math_utils (header-only)
```

### Thread Model

| Thread | Role |
|--------|------|
| **UI Thread** | Renders ImGui, writes params to `AtomicParamStore` (`std::atomic<float>`), pushes `MidiEvent` to `MidiEventQueue` (SPSC ring buffer), calls `EffectChain::setSlotParam` (RT-safe via atomics) |
| **Audio Thread** | RtAudio callback → `EngineManager::processBlock()` → drains `MidiEventQueue` → music layer (Arp/Seq) → active `IEngine::tickSample()` → `EffectChain::processBlock()` → outputs `outL[]`/`outR[]`. |
| **MIDI Thread** | RtMidi callback → pushes events to `MidiEventQueue` |

### Signal Path

```
OSC1 ─┐
OSC2 ─┼──► MIXER ──► MOOG LADDER FILTER ──► VCA ──► EFFECT CHAIN ──► OUTPUT
OSC3 ─┤              ▲         ▲            ▲         (up to 16 slots)
NOISE─┘           FilterEnv  KbdTrack    AmpEnv
                                │
                    LFO (OSC3 in LFO mode via ModMatrix)
```

OSC3 dual-mode: Audio (goes into Mixer) **or** LFO (feeds ModMatrix only, no audio output).

### Effect Chain

Up to 16 serial effect slots, each holding one of 8 types:

| Type | Class | Params |
|------|-------|--------|
| Gain | `GainEffect` | Mode, Gain, Asymmetry, Level, Tone |
| Chorus | `ChorusEffect` | Depth, Rate, Voices, Mix |
| Flanger | `FlangerEffect` | Depth, Rate, Feedback, Mix |
| Phaser | `PhaserEffect` | Rate, Depth, Feedback, Mix |
| Tremolo | `TremoloEffect` | Rate, Depth, Shape |
| Delay | `DelayEffect` | Time, Feedback, Mix, BpmSync |
| Reverb | `ReverbEffect` | Size, Decay, Damping, PreDelay, Mix |
| Equalizer | `EqEffect` | Low, LowMid, Mid, HiMid, High (±12dB), Level |

Effect parameters use a double-buffer pattern (`pendingCoef_` + acquire/release atomics) in `EqEffect` to prevent data races on biquad coefficient structs.

### UI Panels

Panels are organized in three subfolders under `ui/panels/`:

**`ui/panels/engines/`** — per-engine controls:
| Panel | File | Description |
|-------|------|-------------|
| Engine Selector | `panel_engine_selector.cpp` | RadioButton engine switch + per-engine sub-panel |
| Moog | `panel_moog.cpp` | Controllers + Oscillators + Mixer + Filter & Envelopes |
| Hammond | `panel_hammond.cpp` | 9 drawbar sliders, Leslie, tube overdrive, percussion |
| Rhodes | `panel_rhodes.cpp` | Decay, tone, drive, vibrato, tremolo |
| DX7 | `panel_dx7.cpp` | Algorithm selector, 6-operator table (ADSR/ratio/KRS/fixed) |
| Mellotron | `panel_mellotron.cpp` | Tape selector, wow/flutter, pitch spread, runout |
| Drums | `panel_drums.cpp` | 16-pad grid + kick sweep + global params |

**`ui/panels/controls/`** — global controls:
| Panel | File | Description |
|-------|------|-------------|
| Effects | `panel_effects.cpp` | Effect chain editor — add/remove/reorder slots |
| Music | `panel_music.cpp` | Arpeggiator + Chord + Scale + Sequencer (5 tabs) + Keyboard |
| MIDI Player | `panel_midi_player.cpp` | MIDI file transport — Play/Pause/Stop/Seek |
| Output | `panel_output.cpp` | Master Volume knob |
| Presets | `panel_presets.cpp` | Engine + Effect + Global tabs (Load below list, right-aligned) |

**`ui/panels/analysis/`** — signal visualization:
| Panel | File | Description |
|-------|------|-------------|
| Oscilloscope | `panel_oscilloscope.cpp` | Triggered waveform, auto/manual scale |
| Spectrum | `panel_spectrum.cpp` | Real-time FFT magnitude (log scale) |
| Spectrogram | `panel_spectrogram.cpp` | Scrolling time-frequency heatmap |
| Lissajous | `panel_lissajous.cpp` | L vs R scatter plot |
| VU Meter | `panel_vumeter.cpp` | Peak + RMS L/R with peak hold |
| Correlation | `panel_correlation.cpp` | L/R cross-correlation curve |

### Assets

- `assets/moog_presets/` — 10 JSON sound presets
- `assets/hammond_presets/` — 10 JSON Hammond presets (Jazz, Gospel, Rock, etc.)
- `assets/rhodes_presets/` — 10 JSON Rhodes presets (Classic EP, Neo Soul, etc.)
- `assets/dx7_presets/` — 10 JSON DX7 presets (EP, Bells, Brass, Organ, etc.)
- `assets/mellotron_presets/` — 10 JSON Mellotron presets
- `assets/drum_kits/default/` — WAV samples for drum sample pads (pads 8–15)
- `assets/global_presets/` — 35 JSON global presets (engine + effects combined)
- `assets/sequencer_patterns/` — 20 JSON sequencer patterns
- `assets/effect_presets/` — 20 JSON effect chain presets
- `assets/midi/` — 10 classical MIDI files (public domain, for MIDI player)
- All asset directories are auto-copied next to the `.exe` via CMake post-build step.

## Coding Rules

### Real-Time Thread Safety (Critical)

Functions called from the audio callback **must be RT-SAFE** — annotate with `// [RT-SAFE]`.

**FORBIDDEN in audio thread:** `new`/`delete`/`malloc`, `std::vector::push_back` (if reallocating), `std::string` construction, `std::mutex::lock()`, `std::cout`/`printf`, file I/O, any blocking syscall, `throw`.

**ALLOWED:** Stack allocation (≤64 KB fixed arrays), `std::atomic` load/store (`memory_order_relaxed`), `SPSCQueue::push/pop`, `std::array`, arithmetic/math.

**Effect chain RT model:**
- Structural changes (add/remove slots): `setConfig()` → mutex → `configDirty_` flag → audio thread applies via `try_lock` (max ~5ms delay).
- Parameter knob updates: `setSlotParam()` → direct `effect->setParam()` + atomic store. RT-safe because effects clamp in `setParam()` and return clamped value via `getParam()`.
- EQ coefficient updates: double-buffered — UI writes `pendingCoef_[b]` then sets `coefDirty_[b]` (release); audio flushes to `coef_[b]` in `processBlock` (acquire).

### Naming Conventions

| Kind | Convention | Example |
|------|-----------|---------|
| Files | `snake_case.h/.cpp` | `moog_filter.cpp` |
| Class/Struct | `PascalCase` | `MoogFilter` |
| Interface | `IPascalCase` | `IAudioProcessor` |
| Member variable | `camelCase_` (trailing `_`) | `sampleRate_` |
| Static member | `s_camelCase` | `s_instance_` |
| Constant/Macro | `UPPER_SNAKE` | `MAX_VOICES` |
| Free function/method | `camelCase` | `midiNoteToHz()` |
| ParamID enum values | `P_UPPER_CASE` | `P_FILTER_CUTOFF` |

### Other Rules

- **No global state** — no globals, no singletons (except app entry point). All state injected via constructor/`init()`.
- **DSP Core has zero platform dependency** — no `#include <RtAudio.h>`, `<imgui.h>`, `<Arduino.h>`.
- **All continuous params go through `ParamSmoother`** (default 5 ms) to prevent zipper noise.
- **All buffers pre-allocated at `init()`** — no allocation in audio thread.
- Getters: always `const noexcept`. `tick()`/`process()`/`processBlock()`: always `noexcept`.
- `setSampleRate()` / `setBlockSize()`: always `noexcept`.
- Use `std::clamp()` at setters; `assert()` in Debug, clamp/saturate in Release.
- Annotate RT-unsafe functions with `// [RT-UNSAFE]`.

### Stuck Note Prevention

`KeyboardInput` maintains `noteState_[128]`. Two defensive measures prevent stuck notes:
1. Octave shift (`[`/`]`) calls `allNotesOff()` before changing octave.
2. `glfwSetWindowFocusCallback` → `allNotesOff()` on window focus loss.

### File Header Template

```cpp
// ─────────────────────────────────────────────────────────
// FILE: core/dsp/moog_filter.h
// BRIEF: 4-pole Moog ladder filter with tanh saturation
// ─────────────────────────────────────────────────────────
#pragma once
// (1) system/STL includes (alphabetical)
// (2) third-party includes
// (3) project includes (relative to project root)
#include "shared/types.h"
```

## Testing

Tests use **Catch2 v3** in `tests/`. Each file maps to one DSP component:

`test_oscillator.cpp`, `test_moog_filter.cpp`, `test_envelope.cpp`, `test_glide.cpp`, `test_voice.cpp`, `test_arpeggiator.cpp`, `test_sequencer.cpp`, `test_scale_quantizer.cpp`

All 44 tests must pass before merging any DSP changes.

## Git Commit Convention

```
<type>(<scope>): <subject>

Types:  feat | fix | perf | refactor | test | docs | build | style
Scopes: dsp | voice | engine | arp | seq | ui | hal | shared | effects
```

## V3 Roadmap (Planned — Teensy 4.1 port + P4 DSP improvements)

**Hardware port:**
1. **Teensy 4.1 HAL** — swap `hal/pc/` for `hal/teensy/` (I2S audio, SD card presets)
2. **Karplus-Strong oscillator** — plucked string / acoustic guitar

**P4 DSP improvements (see `documents/DSP_Engine_Analysis_v2.md` §11):**
3. **DX7 system LFO** — PM + AM modulation to operators (6 waveforms)
4. **DX7 operator key-level scaling** — amplitude curve by note pitch
5. **Moog hard sync** OSC1 → OSC2
6. **Velocity → Moog filter cutoff** (`MP_VEL_FILTER_AMT`)
7. **DX7 ratio range → 24×** (from current 8×)

**Platform features:**
8. **MIDI CC learn** — map any MIDI CC to any parameter
9. **Delay BpmSync** — `ctx.bpm` already wired, needs UI toggle
10. **Drum kit loader UI** — browse/load WAV kits from `assets/drum_kits/` in the UI
