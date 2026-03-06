# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**MiniMoog Model D DSP Simulator** — a cross-platform desktop app written in C++17 that emulates the Minimoog synthesizer with full DSP accuracy. The architecture is designed so the DSP Core can be ported to Teensy 4.1 (V3) by swapping only the HAL layer.

Full Technical Design Document: `documents/MiniMoog DSP Simulator - TDD - V1.md`
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

# Debug build (ASan + UBSan, Linux only)
cmake -B build_dbg -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build_dbg --parallel $(nproc)
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
  ├── dsp/    Oscillator, MoogFilter, Envelope, LFO, Glide, Noise, ParamSmoother
  ├── voice/  Voice, VoicePool (mono/poly/unison, voice stealing)
  ├── music/  Arpeggiator, Sequencer, ChordEngine, ScaleQuantizer
  ├── engine/ SynthEngine (orchestrator), ModMatrix, PresetManager
  └── util/   SPSCQueue (lock-free), math_utils (header-only)
```

### Thread Model

| Thread | Role |
|--------|------|
| **UI Thread** | Renders ImGui, writes params to `AtomicParamStore` (`std::atomic<float>`), pushes `MidiEvent` to `MidiEventQueue` (SPSC ring buffer) |
| **Audio Thread** | RtAudio callback → `SynthEngine::processBlock()` → drains `MidiEventQueue` → ticks `VoicePool` → outputs `outL[]`/`outR[]`. Also fills `oscBuf_[2048]` for oscilloscope. |
| **MIDI Thread** | RtMidi callback → pushes events to `MidiEventQueue` |

### Signal Path

```
OSC1 ─┐
OSC2 ─┼──► MIXER ──► MOOG LADDER FILTER ──► VCA ──► OUTPUT
OSC3 ─┤              ▲         ▲            ▲
NOISE─┘           FilterEnv  KbdTrack    AmpEnv
                                │
                    LFO (OSC3 in LFO mode via ModMatrix)
```

OSC3 dual-mode: Audio (goes into Mixer) **or** LFO (feeds ModMatrix only, no audio output).

### UI Panels

| Panel | File | Description |
|-------|------|-------------|
| Controllers | `panel_controllers.cpp` | Glide, Modulation, BPM, Polyphony |
| Oscillators | `panel_oscillators.cpp` | OSC1/2/3 — Range, Wave, Freq, LFO Mode |
| Mixer | `panel_mixer.cpp` | OSC1/2/3/Noise levels, Noise Color |
| Filter & Envelopes | `panel_modifiers.cpp` | Moog Filter, Filter ADSR, Amp ADSR |
| Music | `panel_music.cpp` | Arpeggiator + Chord + Scale + Sequencer (4 tabs) |
| Oscilloscope | `panel_oscilloscope.cpp` | Triggered waveform display |
| Output | `panel_output.cpp` | Master Volume |
| Presets | `panel_presets.cpp` | Load/Save/Reset JSON presets |
| Keyboard & Play | inline in `imgui_app.cpp` | QWERTY kbd, clickable piano, Volume knob |

**Note:** `panel_arpeggiator.cpp`, `panel_sequencer.cpp`, `panel_chord_scale.cpp` exist in the repo but are **not compiled** — functionality consolidated into `panel_music.cpp`.

### Assets

- `assets/presets/` — 20 JSON sound presets (Bass, Lead, Pad, Brass, FX categories)
- `assets/patterns/` — 10 JSON sequencer patterns (Acid, Funk, Techno, Jazz, etc.)
- Both directories are auto-copied next to the `.exe` via CMake post-build step.

## Coding Rules

### Real-Time Thread Safety (Critical)

Functions called from the audio callback **must be RT-SAFE** — annotate with `// [RT-SAFE]`.

**FORBIDDEN in audio thread:** `new`/`delete`/`malloc`, `std::vector::push_back` (if reallocating), `std::string` construction, `std::mutex::lock()`, `std::cout`/`printf`, file I/O, any blocking syscall, `throw`.

**ALLOWED:** Stack allocation (≤64 KB fixed arrays), `std::atomic` load/store (`memory_order_relaxed`), `SPSCQueue::push/pop`, `std::array`, arithmetic/math.

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
- Getters: always `const noexcept`. `tick()`/`process()`: always `noexcept`.
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
Scopes: dsp | voice | engine | arp | seq | ui | hal | shared
```

## V2 Roadmap (Planned)

High-value modules for V2:
1. **Chorus/Ensemble** — unlocks strings, organ, electric piano realism
2. **Reverb (FDN)** — essential for all instrument types
3. **FM operator routing** — Rhodes EP, DX7-style bell/marimba
4. **Karplus-Strong oscillator** — plucked string / acoustic guitar
5. **Wavetable oscillator** — sample-based timbres, piano
6. **Velocity → timbre mapping** — currently velocity only affects volume
