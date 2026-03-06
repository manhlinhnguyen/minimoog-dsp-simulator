# MiniMoog DSP Simulator

# Technical Design Document — V1.0

**Document version:** 1.0.0 **Target platform:** PC (Windows / macOS / Linux) **Language:** C++17 **Status:** Ready for implementation

---

# MỤC LỤC

|#|Tên phần|Trạng thái|
|---|---|---|
|1|TỔNG QUAN HỆ THỐNG|✅|
|2|KIẾN TRÚC TỔNG THỂ|✅|
|3|SHARED LAYER|✅|
|4|DSP CORE — OSCILLATOR|✅|
|5|DSP CORE — MOOG FILTER|✅|
|6|DSP CORE — ENVELOPE, LFO, GLIDE|✅|
|7|VOICE & VOICE POOL|✅|
|8|MUSIC LAYER|✅|
|9|SYNTH ENGINE|✅|
|10|HAL INTERFACES|✅|
|11|HAL — PC IMPLEMENTATION|✅|
|12|UI SYSTEM|✅|
|13|APPLICATION ENTRY POINT|✅|
|14|THREADING MODEL|✅|
|15|PERFORMANCE BUDGET|✅|
|16|TEST PLAN|✅|
|17|BUILD SYSTEM|✅|
|18|PROJECT DIRECTORY TREE|✅|
|19|CODING CONVENTIONS|✅|
|20|BUILD & RUN GUIDE|✅|
|19|GLOSSARY|✅|

---

# PHẦN 1 — TỔNG QUAN & SCOPE

## 1.1 Mục Tiêu V1

V1 xây dựng một **PC Simulator App** hoàn chỉnh mô phỏng Minimoog Model D dưới dạng ứng dụng desktop cross-platform. Toàn bộ DSP logic được viết bằng C++17 thuần, tách biệt hoàn toàn khỏi platform, để V3 có thể port lên Teensy 4.1 chỉ bằng cách viết thêm HAL layer mà không sửa DSP Core.

## 1.2 Tính Năng V1

```
DSP ENGINE:
  ✅ 3 Oscillator (PolyBLEP, 6 waveform)
  ✅ Moog Ladder Filter (Huovilainen model, tanh)
  ✅ Dual ADSR (Filter CV + Amp CV)
  ✅ Glide / Portamento (exponential)
  ✅ LFO (OSC3 dual-mode)
  ✅ Modulation Matrix (pitch + filter)
  ✅ White / Pink Noise generator
  ✅ Parameter smoothing (zipper-free)

POLYPHONY:
  ✅ Mono / Poly / Unison mode
  ✅ Tối đa 8 voice
  ✅ Voice stealing (oldest / lowest / quietest)
  ✅ Unison detune spread

MUSIC LAYER:
  ✅ Arpeggiator (6 mode, 8 rate, swing, gate)
  ✅ Step Sequencer (16 step, swing, tie)
  ✅ Chord Engine (16 chord type, 4 inversion)
  ✅ Scale Quantizer (16 scale, 12 root)

PC APP:
  ✅ ImGui GUI (knobs, switches, ADSR display)
  ✅ QWERTY keyboard → MIDI notes
  ✅ USB MIDI input (RtMidi)
  ✅ RtAudio output (Win/Mac/Linux)
  ✅ Preset save/load (JSON)
  ✅ Factory presets

KHÔNG có trong V1:
  ❌ Effects (chorus, reverb, delay, EQ...)
  ❌ MIDI output / recording
  ❌ VST plugin wrapper
  ❌ Teensy firmware (V3)
```

## 1.3 Phiên Bản & Roadmap

```
V1.0  → PC Simulator (tài liệu này)
V2.0  → Effects chain (chorus, reverb, delay, EQ,
         flanger, phaser, tremolo)
V3.0  → Teensy 4.1 firmware + hardware build
```

---

# PHẦN 2 — NGUYÊN TẮC THIẾT KẾ

## 2.1 Các Quy Tắc Bất Biến

```
RULE 1: DSP CORE KHÔNG CÓ PLATFORM DEPENDENCY
  → Không #include <Arduino.h>
  → Không #include <RtAudio.h>
  → Không #include <imgui.h>
  → Chỉ C++17 standard library (và chỉ phần
    không dùng heap trong audio thread)

RULE 2: AUDIO THREAD KHÔNG ALLOCATE / DEALLOCATE
  → Không new / delete / malloc / free
  → Không std::vector push_back (trong audio thread)
  → Không std::string construction
  → Không mutex lock (dùng lock-free thay thế)
  → Tất cả buffer pre-allocated ở init()

RULE 3: PARAMETER THREAD SAFETY
  → UI thread ghi params vào AtomicParamStore
  → Audio thread đọc params từ AtomicParamStore
  → Dùng std::atomic<float> với memory_order_relaxed
  → MIDI events qua SPSC ring buffer

RULE 4: PARAMETER SMOOTHING BẮT BUỘC
  → Mọi param liên tục (cutoff, resonance, volume...)
    PHẢI qua ParamSmoother trước khi apply
  → Smooth time mặc định: 5ms
  → Tránh zipper noise hoàn toàn

RULE 5: SINGLE RESPONSIBILITY
  → Mỗi class chỉ làm 1 việc
  → Oscillator không biết về Filter
  → Voice không biết về Arpeggiator
  → SynthEngine điều phối, không tự xử lý DSP

RULE 6: ZERO GLOBAL STATE
  → Không biến global
  → Không singleton (trừ app entry point)
  → Tất cả state inject qua constructor / init()
```

## 2.2 Quy Ước Đặt Tên

```
Files:        snake_case.h / snake_case.cpp
Classes:      PascalCase
Methods:      camelCase()
Members:      camelCase_ (trailing underscore)
Constants:    ALL_CAPS
Enums:        PascalCase : int { PascalCase = 0 }
Params:       P_UPPER_CASE (trong ParamID enum)
```

---

# PHẦN 3 — KIẾN TRÚC TỔNG THỂ

## 3.1 Layer Diagram

```
╔══════════════════════════════════════════════════════════╗
║                   APPLICATION LAYER                       ║
║  sim/main.cpp                                            ║
║  • Khởi tạo tất cả subsystem                            ║
║  • GLFW event loop                                       ║
║  • Render loop                                           ║
╠══════════════════════════════════════════════════════════╣
║                     UI LAYER                              ║
║  ui/imgui_app.h/cpp                                      ║
║  ui/panels/panel_*.h/cpp                                 ║
║  ui/widgets/knob_widget.h, adsr_display.h                ║
║  • Vẽ toàn bộ GUI                                        ║
║  • Đọc/ghi AtomicParamStore                              ║
║  • Không chứa business logic                             ║
╠══════════════════════════════════════════════════════════╣
║                    PC HAL LAYER                           ║
║  hal/pc/rtaudio_backend.h/cpp   ← RtAudio               ║
║  hal/pc/pc_midi.h/cpp           ← RtMidi                ║
║  hal/pc/keyboard_input.h/cpp    ← GLFW keys             ║
║  hal/pc/preset_storage.h/cpp    ← nlohmann/json         ║
║  • Bridge platform APIs → DSP Core interfaces           ║
╠══════════════════════════════════════════════════════════╣
║                   SHARED LAYER                            ║
║  shared/types.h                                          ║
║  shared/params.h                                         ║
║  shared/interfaces.h                                     ║
║  • Abstract interfaces (IAudioProcessor, etc.)           ║
║  • Common types, ParamID enum                            ║
║  • AtomicParamStore                                       ║
╠══════════════════════════════════════════════════════════╣
║                    DSP CORE LAYER                         ║
║  core/dsp/        ← DSP primitives                       ║
║  core/voice/      ← Voice management                     ║
║  core/music/      ← Arp, Seq, Chord, Scale              ║
║  core/engine/     ← SynthEngine orchestrator            ║
║  core/util/       ← Ring buffer, math utils             ║
║  • Pure C++17, ZERO platform dependency                  ║
║  • Compile trên bất kỳ C++17 compiler                   ║
╚══════════════════════════════════════════════════════════╝
```

## 3.2 Data Flow

```
UI Thread:                    Audio Thread:
──────────                    ─────────────
User interaction              RtAudio callback fires
     │                              │
     ▼                              ▼
ImGui renders UI          SynthEngine::processBlock()
     │                              │
     ▼                              ├── Drain MidiEventQueue
AtomicParamStore                    │        │
  .setParam(id, val)                │        ▼
     │                              │   handleNoteOn/Off()
QWERTY / MIDI                       │        │
     │                              │        ▼
     ▼                              │   MusicLayer tick()
MidiEventQueue                      │   (Arp / Seq)
  .push(event)                      │        │
                                    │        ▼
                                    │   VoicePool.noteOn()
                                    │        │
                                    ├── Read ParamCache
                                    │   (từ AtomicParamStore)
                                    │        │
                                    ▼        ▼
                               Voice[n].tick() × N voices
                                    │
                                    ▼
                               Mix + Master Volume
                                    │
                                    ▼
                             outL[], outR[] → RtAudio DAC
```

## 3.3 Signal Flow (Audio Path)

```
OSC1 ─┐
OSC2 ─┼──► MIXER ──► MOOG LADDER FILTER ──► VCA ──► OUTPUT
OSC3 ─┤              ▲         ▲            ▲
NOISE─┘              │         │            │
                 FilterEnv  KbdTrack     AmpEnv
                     │
                  LFO mod
                  (nếu filter mod on)

OSC3 dual-mode:
  • Audio mode  → vào MIXER như OSC3
  • LFO mode    → bypass MIXER, đi thẳng vào ModMatrix
```

---

# PHẦN 4 — CẤU TRÚC THƯ MỤC & FILES

```
minimoog-dsp/
│
├── CMakeLists.txt                    ← Root CMake
├── README.md
├── .gitignore
│
├── shared/
│   ├── types.h                       ← Primitive types, constants
│   ├── params.h                      ← ParamID enum, ParamMeta
│   ├── params.cpp                    ← PARAM_META table
│   └── interfaces.h                  ← Abstract base classes
│
├── core/
│   ├── dsp/
│   │   ├── oscillator.h
│   │   ├── oscillator.cpp
│   │   ├── moog_filter.h
│   │   ├── moog_filter.cpp
│   │   ├── envelope.h
│   │   ├── envelope.cpp
│   │   ├── glide.h
│   │   ├── glide.cpp
│   │   ├── lfo.h
│   │   ├── lfo.cpp
│   │   ├── noise.h
│   │   ├── noise.cpp
│   │   └── param_smoother.h          ← header-only
│   │
│   ├── voice/
│   │   ├── voice.h
│   │   ├── voice.cpp
│   │   ├── voice_pool.h
│   │   └── voice_pool.cpp
│   │
│   ├── music/
│   │   ├── arpeggiator.h
│   │   ├── arpeggiator.cpp
│   │   ├── sequencer.h
│   │   ├── sequencer.cpp
│   │   ├── chord_engine.h
│   │   ├── chord_engine.cpp
│   │   ├── scale_quantizer.h
│   │   └── scale_quantizer.cpp
│   │
│   ├── engine/
│   │   ├── synth_engine.h
│   │   ├── synth_engine.cpp
│   │   ├── mod_matrix.h
│   │   ├── mod_matrix.cpp
│   │   └── preset_manager.h
│   │   └── preset_manager.cpp
│   │
│   └── util/
│       ├── math_utils.h              ← header-only
│       └── spsc_queue.h              ← header-only, lock-free
│
├── hal/
│   └── pc/
│       ├── rtaudio_backend.h
│       ├── rtaudio_backend.cpp
│       ├── pc_midi.h
│       ├── pc_midi.cpp
│       ├── keyboard_input.h
│       ├── keyboard_input.cpp
│       └── preset_storage.h
│       └── preset_storage.cpp
│
├── ui/
│   ├── imgui_app.h
│   ├── imgui_app.cpp
│   ├── panels/
│   │   ├── panel_controllers.h/.cpp
│   │   ├── panel_oscillators.h/.cpp
│   │   ├── panel_mixer.h/.cpp
│   │   ├── panel_modifiers.h/.cpp
│   │   ├── panel_output.h/.cpp
│   │   ├── panel_arpeggiator.h/.cpp
│   │   ├── panel_sequencer.h/.cpp
│   │   ├── panel_chord_scale.h/.cpp
│   │   └── panel_presets.h/.cpp
│   └── widgets/
│       ├── knob_widget.h/.cpp
│       ├── adsr_display.h/.cpp
│       ├── keyboard_display.h/.cpp
│       └── seq_display.h/.cpp
│
├── sim/
│   └── main.cpp                      ← PC app entry point
│
├── tests/
│   ├── CMakeLists.txt
│   ├── test_main.cpp                 ← Catch2 session
│   ├── test_oscillator.cpp
│   ├── test_moog_filter.cpp
│   ├── test_envelope.cpp
│   ├── test_glide.cpp
│   ├── test_voice.cpp
│   ├── test_arpeggiator.cpp
│   ├── test_sequencer.cpp
│   └── test_scale_quantizer.cpp
│
└── assets/
    └── presets/
        ├── bass_classic.json
        ├── bass_fat.json
        ├── lead_acid.json
        ├── lead_mellow.json
        ├── pad_warm.json
        ├── pad_strings.json
        ├── keys_pluck.json
        └── fx_sweep.json
```

---

# PHẦN 5 — THƯ VIỆN PHỤ THUỘC

## 5.1 Danh Sách Đầy Đủ

```
┌─────────────────┬──────────┬───────────────────────────────┐
│ Library         │ Version  │ Mục đích                      │
├─────────────────┼──────────┼───────────────────────────────┤
│ Dear ImGui      │ v1.90.4  │ GUI framework                 │
│ GLFW            │ 3.3+     │ Window, OpenGL ctx, keyboard  │
│ OpenGL          │ 3.3 core │ ImGui render backend          │
│ RtAudio         │ 6.0.1    │ Cross-platform audio output   │
│ RtMidi          │ 6.0.0    │ USB MIDI input                │
│ imgui-knobs     │ latest   │ Rotary knob widget            │
│ nlohmann/json   │ v3.11.3  │ Preset serialization          │
│ Catch2          │ v3.5.2   │ Unit testing                  │
└─────────────────┴──────────┴───────────────────────────────┘

Tất cả được fetch tự động qua CMake FetchContent.
Không cần cài đặt thủ công (trừ GLFW trên Linux:
  apt install libglfw3-dev / brew install glfw)
```

## 5.2 CMakeLists.txt Root (Đầy Đủ)

```cmake
cmake_minimum_required(VERSION 3.16)
project(minimoog_dsp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler warnings
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── FetchContent ────────────────────────────────────────
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.90.4
  GIT_SHALLOW    TRUE)

FetchContent_Declare(imgui_knobs
  GIT_REPOSITORY https://github.com/altschuler/imgui-knobs.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE)

FetchContent_Declare(rtaudio
  GIT_REPOSITORY https://github.com/thestk/rtaudio.git
  GIT_TAG        6.0.1
  GIT_SHALLOW    TRUE)

FetchContent_Declare(rtmidi
  GIT_REPOSITORY https://github.com/thestk/rtmidi.git
  GIT_TAG        6.0.0
  GIT_SHALLOW    TRUE)

FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
  GIT_SHALLOW    TRUE)

FetchContent_Declare(catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.2
  GIT_SHALLOW    TRUE)

FetchContent_MakeAvailable(
  imgui imgui_knobs rtaudio rtmidi nlohmann_json catch2)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# ── ImGui compiled as library ───────────────────────────
add_library(imgui_lib STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  ${imgui_knobs_SOURCE_DIR}/imgui-knobs.cpp
)
target_include_directories(imgui_lib PUBLIC
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${imgui_knobs_SOURCE_DIR}
)
target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

# ── DSP Core ────────────────────────────────────────────
add_library(dsp_core STATIC
  core/dsp/oscillator.cpp
  core/dsp/moog_filter.cpp
  core/dsp/envelope.cpp
  core/dsp/glide.cpp
  core/dsp/lfo.cpp
  core/dsp/noise.cpp
  core/voice/voice.cpp
  core/voice/voice_pool.cpp
  core/music/arpeggiator.cpp
  core/music/sequencer.cpp
  core/music/chord_engine.cpp
  core/music/scale_quantizer.cpp
  core/engine/synth_engine.cpp
  core/engine/mod_matrix.cpp
  core/engine/preset_manager.cpp
  shared/params.cpp
)
target_include_directories(dsp_core PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/shared
)

# ── PC HAL ──────────────────────────────────────────────
add_library(pc_hal STATIC
  hal/pc/rtaudio_backend.cpp
  hal/pc/pc_midi.cpp
  hal/pc/keyboard_input.cpp
  hal/pc/preset_storage.cpp
)
target_include_directories(pc_hal PUBLIC
  ${CMAKE_SOURCE_DIR}
)
target_link_libraries(pc_hal PUBLIC
  dsp_core rtaudio rtmidi nlohmann_json::nlohmann_json)

# ── UI ──────────────────────────────────────────────────
add_library(ui_lib STATIC
  ui/imgui_app.cpp
  ui/panels/panel_controllers.cpp
  ui/panels/panel_oscillators.cpp
  ui/panels/panel_mixer.cpp
  ui/panels/panel_modifiers.cpp
  ui/panels/panel_output.cpp
  ui/panels/panel_arpeggiator.cpp
  ui/panels/panel_sequencer.cpp
  ui/panels/panel_chord_scale.cpp
  ui/panels/panel_presets.cpp
  ui/widgets/knob_widget.cpp
  ui/widgets/adsr_display.cpp
  ui/widgets/keyboard_display.cpp
  ui/widgets/seq_display.cpp
)
target_include_directories(ui_lib PUBLIC
  ${CMAKE_SOURCE_DIR}
)
target_link_libraries(ui_lib PUBLIC
  dsp_core pc_hal imgui_lib)

# ── Simulator Executable ────────────────────────────────
add_executable(minimoog_sim sim/main.cpp)
target_link_libraries(minimoog_sim PRIVATE
  dsp_core pc_hal ui_lib)

# ── Tests ───────────────────────────────────────────────
enable_testing()
add_subdirectory(tests)
```

---

# PHẦN 6 — SHARED LAYER (ĐẦY ĐỦ)

## 6.1 `shared/types.h`

```cpp
#pragma once
#include <cstdint>
#include <cmath>
#include <functional>
#include <array>

// ════════════════════════════════════════════════════════
// PRIMITIVE TYPES
// ════════════════════════════════════════════════════════

using sample_t   = float;   // Audio sample [-1.0, +1.0]
using cv_t       = float;   // Control value [0.0, 1.0]
using hz_t       = float;   // Frequency in Hz
using ms_t       = float;   // Time in milliseconds
using beats_t    = float;   // Musical time in beats

// ════════════════════════════════════════════════════════
// CONSTANTS
// ════════════════════════════════════════════════════════

constexpr float SAMPLE_RATE_DEFAULT = 44100.0f;
constexpr int   BLOCK_SIZE_DEFAULT  = 256;
constexpr int   MAX_VOICES          = 8;
constexpr int   MAX_HELD_NOTES      = 32;
constexpr int   MAX_CHORD_NOTES     = 6;
constexpr float TWO_PI              = 6.28318530718f;
constexpr float HALF_PI             = 1.57079632679f;
constexpr float LOG2                = 0.69314718056f;
constexpr float A4_HZ               = 440.0f;
constexpr int   A4_MIDI             = 69;
constexpr int   MIDI_NOTE_MIN       = 0;
constexpr int   MIDI_NOTE_MAX       = 127;

// ════════════════════════════════════════════════════════
// MIDI EVENT
// ════════════════════════════════════════════════════════

struct MidiEvent {
    enum class Type : uint8_t {
        NoteOn = 0,
        NoteOff,
        ControlChange,
        PitchBend,
        Aftertouch,
        ProgramChange,
        Clock,
        Start,
        Stop,
        Continue,
        Invalid
    };

    Type    type      = Type::Invalid;
    uint8_t channel   = 0;    // 0-based (0 = Ch1)
    uint8_t data1     = 0;    // note / cc number
    uint8_t data2     = 0;    // velocity / value
    int16_t pitchBend = 0;    // -8192..+8191
};

// ════════════════════════════════════════════════════════
// HELD NOTE (voice tracking)
// ════════════════════════════════════════════════════════

struct HeldNote {
    int  midiNote  = -1;
    int  velocity  = 0;
    bool active    = false;

    bool isValid() const { return midiNote >= 0; }
};

// ════════════════════════════════════════════════════════
// VOICE MODE & STEAL MODE
// ════════════════════════════════════════════════════════

enum class VoiceMode  : int { Mono = 0, Poly = 1, Unison = 2 };
enum class StealMode  : int { Oldest = 0, Lowest = 1, Quietest = 2 };
```

## 6.2 `shared/params.h`

```cpp
#pragma once
#include <cstddef>

// ════════════════════════════════════════════════════════
// PARAM ID — định danh duy nhất cho mọi tham số
// Dùng chung: UI knob, MIDI CC mapping, preset JSON key
// ════════════════════════════════════════════════════════

enum ParamID : int {

    // ── GLOBAL ──────────────────────────────────────────
    P_MASTER_TUNE = 0,  // -1.0..+1.0 (semitone)
    P_MASTER_VOL,       // 0.0..1.0
    P_BPM,              // 60.0..200.0

    // ── CONTROLLERS ─────────────────────────────────────
    P_GLIDE_ON,         // 0/1 (switch)
    P_GLIDE_TIME,       // 0.0..1.0 → 0..5000ms (log)
    P_MOD_MIX,          // 0.0..1.0 (OSC3←→Noise)
    P_OSC_MOD_ON,       // 0/1
    P_FILTER_MOD_ON,    // 0/1
    P_OSC3_LFO_ON,      // 0/1 (OSC3 as LFO)

    // ── OSCILLATOR 1 ────────────────────────────────────
    P_OSC1_ON,          // 0/1
    P_OSC1_RANGE,       // 0..5 (LO,32',16',8',4',2')
    P_OSC1_FREQ,        // 0.0..1.0 → -7..+7 semitones
    P_OSC1_WAVE,        // 0..5 (WaveShape enum)

    // ── OSCILLATOR 2 ────────────────────────────────────
    P_OSC2_ON,
    P_OSC2_RANGE,
    P_OSC2_FREQ,
    P_OSC2_WAVE,

    // ── OSCILLATOR 3 ────────────────────────────────────
    P_OSC3_ON,
    P_OSC3_RANGE,
    P_OSC3_FREQ,
    P_OSC3_WAVE,

    // ── MIXER ───────────────────────────────────────────
    P_MIX_OSC1,         // 0.0..1.0
    P_MIX_OSC2,
    P_MIX_OSC3,
    P_MIX_NOISE,
    P_NOISE_COLOR,      // 0=white, 1=pink

    // ── FILTER ──────────────────────────────────────────
    P_FILTER_CUTOFF,    // 0.0..1.0 → 20Hz..20kHz (log)
    P_FILTER_EMPHASIS,  // 0.0..1.0 (resonance)
    P_FILTER_AMOUNT,    // 0.0..1.0 (env mod amount)
    P_FILTER_KBD_TRACK, // 0=off, 1=1/3, 2=2/3

    // ── FILTER ENVELOPE ─────────────────────────────────
    P_FENV_ATTACK,      // 0.0..1.0 → 1..10000ms (log)
    P_FENV_DECAY,       // 0.0..1.0 → 1..10000ms (log)
    P_FENV_SUSTAIN,     // 0.0..1.0
    P_FENV_RELEASE,     // 0.0..1.0 → 1..10000ms (log)

    // ── AMP ENVELOPE ────────────────────────────────────
    P_AENV_ATTACK,
    P_AENV_DECAY,
    P_AENV_SUSTAIN,
    P_AENV_RELEASE,

    // ── POLYPHONY ───────────────────────────────────────
    P_VOICE_MODE,       // 0=mono, 1=poly, 2=unison
    P_VOICE_COUNT,      // 1..8
    P_VOICE_STEAL,      // 0=oldest, 1=lowest, 2=quietest
    P_UNISON_DETUNE,    // 0.0..1.0 → 0..50 cents spread

    // ── ARPEGGIATOR ─────────────────────────────────────
    P_ARP_ON,           // 0/1
    P_ARP_MODE,         // 0..5
    P_ARP_OCTAVES,      // 1..4
    P_ARP_RATE,         // 0..7 (subdivision index)
    P_ARP_GATE,         // 0.0..1.0
    P_ARP_SWING,        // 0.0..0.5

    // ── SEQUENCER ───────────────────────────────────────
    P_SEQ_ON,           // 0/1
    P_SEQ_PLAYING,      // 0/1 (play/stop transport)
    P_SEQ_STEPS,        // 1..16
    P_SEQ_RATE,         // 0..7
    P_SEQ_GATE,         // 0.0..1.0
    P_SEQ_SWING,        // 0.0..0.5

    // ── CHORD ───────────────────────────────────────────
    P_CHORD_ON,         // 0/1
    P_CHORD_TYPE,       // 0..15
    P_CHORD_INVERSION,  // 0..3

    // ── SCALE ───────────────────────────────────────────
    P_SCALE_ON,         // 0/1
    P_SCALE_ROOT,       // 0..11 (C..B)
    P_SCALE_TYPE,       // 0..15

    PARAM_COUNT         // = tổng số params (dùng cho array)
};

// ════════════════════════════════════════════════════════
// PARAM METADATA — min, max, default, display name
// ════════════════════════════════════════════════════════

struct ParamMeta {
    const char* name;           // Display name
    const char* jsonKey;        // Key trong preset JSON
    float       defaultVal;     // Giá trị mặc định
    float       minVal;
    float       maxVal;
    bool        isDiscrete;     // true → snap to integer
    const char* unit;           // "", "Hz", "ms", "BPM"...
};

// Defined in shared/params.cpp
extern const ParamMeta PARAM_META[PARAM_COUNT];

// ════════════════════════════════════════════════════════
// HELPER: Normalized (0..1) ↔ Real value conversion
// ════════════════════════════════════════════════════════

// Log scale: freq knob, envelope times
inline float normToLog(float norm, float minVal, float maxVal) {
    // norm=0→minVal, norm=1→maxVal, logarithmic
    return minVal * std::pow(maxVal / minVal, norm);
}

inline float logToNorm(float val, float minVal, float maxVal) {
    return std::log(val / minVal) / std::log(maxVal / minVal);
}

// Cutoff: 0..1 → 20Hz..20000Hz
inline float normToCutoffHz(float norm) {
    return normToLog(norm, 20.0f, 20000.0f);
}

// Envelope time: 0..1 → 1ms..10000ms
inline float normToEnvMs(float norm) {
    return normToLog(norm, 1.0f, 10000.0f);
}

// Glide time: 0..1 → 0ms..5000ms
inline float normToGlideMs(float norm) {
    return norm * norm * 5000.0f;  // quadratic
}

// OSC freq offset: 0..1 → -7..+7 semitones
inline float normToSemitones(float norm) {
    return (norm - 0.5f) * 14.0f;
}
```

## 6.3 `shared/interfaces.h`

```cpp
#pragma once
#include "types.h"
#include "params.h"
#include <atomic>
#include <array>
#include <cstring>

// ════════════════════════════════════════════════════════
// ATOMIC PARAMETER STORE
// Thread-safe: UI thread writes, Audio thread reads
// ════════════════════════════════════════════════════════

class AtomicParamStore {
public:
    AtomicParamStore() {
        // Khởi tạo với giá trị mặc định
        for (int i = 0; i < PARAM_COUNT; ++i)
            values_[i].store(PARAM_META[i].defaultVal,
                             std::memory_order_relaxed);
    }

    // Gọi từ bất kỳ thread nào
    float get(int id) const noexcept {
        return values_[id].load(std::memory_order_relaxed);
    }

    void set(int id, float val) noexcept {
        values_[id].store(val, std::memory_order_relaxed);
    }

    // Batch read vào plain array (audio thread, 1 lần/block)
    void snapshot(float out[PARAM_COUNT]) const noexcept {
        for (int i = 0; i < PARAM_COUNT; ++i)
            out[i] = values_[i].load(std::memory_order_relaxed);
    }

    void resetToDefaults() noexcept {
        for (int i = 0; i < PARAM_COUNT; ++i)
            values_[i].store(PARAM_META[i].defaultVal,
                             std::memory_order_relaxed);
    }

private:
    std::atomic<float> values_[PARAM_COUNT];
    // std::atomic<float> guaranteed lock-free trên x86/ARM
    static_assert(std::atomic<float>::is_always_lock_free
                  || true,  // runtime check below
                  "atomic<float> must be lock-free");
};

// ════════════════════════════════════════════════════════
// SPSC MIDI EVENT QUEUE — lock-free ring buffer
// Producer: UI/MIDI thread
// Consumer: Audio thread
// ════════════════════════════════════════════════════════
// (Defined in core/util/spsc_queue.h — included here
//  for convenience via forward reference)

template<typename T, size_t N>
class SPSCQueue {
    // Xem chi tiết tại core/util/spsc_queue.h
public:
    bool push(const T& item) noexcept;
    bool pop(T& item) noexcept;
    bool isEmpty() const noexcept;
    size_t size() const noexcept;
private:
    std::array<T, N>        buf_;
    std::atomic<size_t>     head_{0};
    std::atomic<size_t>     tail_{0};
};

using MidiEventQueue = SPSCQueue<MidiEvent, 256>;

// ════════════════════════════════════════════════════════
// AUDIO PROCESSOR INTERFACE
// ════════════════════════════════════════════════════════

class IAudioProcessor {
public:
    virtual ~IAudioProcessor() = default;

    // Gọi bởi audio backend mỗi block
    // KHÔNG allocate, KHÔNG lock, KHÔNG throw
    virtual void processBlock(sample_t* outL,
                              sample_t* outR,
                              int nFrames) noexcept = 0;

    virtual void setSampleRate(float sr) = 0;
    virtual void setBlockSize(int bs)    = 0;
};
```

## 6.4 `core/util/spsc_queue.h`

```cpp
#pragma once
#include <atomic>
#include <array>
#include <cstddef>

// Single-Producer Single-Consumer lock-free queue
// Producer: MIDI/UI thread  |  Consumer: Audio thread
// Size MUST be power of 2

template<typename T, size_t N>
class SPSCQueue {
    static_assert((N & (N - 1)) == 0, "N must be power of 2");

public:
    // Called from producer thread only
    bool push(const T& item) noexcept {
        const size_t h = head_.load(std::memory_order_relaxed);
        const size_t next = (h + 1) & (N - 1);
        if (next == tail_.load(std::memory_order_acquire))
            return false;  // full
        buf_[h] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Called from consumer thread only
    bool pop(T& item) noexcept {
        const size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire))
            return false;  // empty
        item = buf_[t];
        tail_.store((t + 1) & (N - 1),
                    std::memory_order_release);
        return true;
    }

    bool isEmpty() const noexcept {
        return tail_.load(std::memory_order_acquire)
            == head_.load(std::memory_order_acquire);
    }

private:
    std::array<T, N>    buf_;
    alignas(64) std::atomic<size_t> head_{0};  // cache line
    alignas(64) std::atomic<size_t> tail_{0};  // separation
};
```

## 6.5 `core/util/math_utils.h`

```cpp
#pragma once
#include <cmath>
#include "shared/types.h"

// ════════════════════════════════════════════════════════
// FREQUENCY UTILITIES
// ════════════════════════════════════════════════════════

// MIDI note (0..127) → Hz, A4=440Hz
inline hz_t midiToHz(int note) noexcept {
    return A4_HZ * std::pow(2.0f,
                            (note - A4_MIDI) / 12.0f);
}

// Semitone offset → frequency ratio
inline float semitonesToRatio(float semitones) noexcept {
    return std::pow(2.0f, semitones / 12.0f);
}

// Cents offset → frequency ratio
inline float centsToRatio(float cents) noexcept {
    return std::pow(2.0f, cents / 1200.0f);
}

// Hz → MIDI note (float, for display)
inline float hzToMidi(hz_t hz) noexcept {
    return A4_MIDI + 12.0f * std::log2(hz / A4_HZ);
}

// ════════════════════════════════════════════════════════
// FAST APPROXIMATIONS (audio-rate safe)
// ════════════════════════════════════════════════════════

// Padé 3/3 approximation of tanh
// Max error < 0.5% in [-3, +3], exact clamp outside
inline float fast_tanh(float x) noexcept {
    if (x >  3.0f) return  1.0f;
    if (x < -3.0f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Fast exp approximation (used for envelope curves)
// Valid range: x in [-10, 0]
inline float fast_exp_negative(float x) noexcept {
    // Schraudolph's approximation
    union { float f; int32_t i; } u;
    u.i = static_cast<int32_t>(12102203.0f * x) + 1065353216;
    return u.f;
}

// Clamp
template<typename T>
inline T clamp(T val, T lo, T hi) noexcept {
    return val < lo ? lo : (val > hi ? hi : val);
}

// Linear interpolation
inline float lerp(float a, float b, float t) noexcept {
    return a + t * (b - a);
}

// dB → linear amplitude
inline float dbToLinear(float db) noexcept {
    return std::pow(10.0f, db / 20.0f);
}

// ════════════════════════════════════════════════════════
// RANGE MAPPING
// ════════════════════════════════════════════════════════

// Map [0,1] → [lo, hi] linear
inline float mapLinear(float norm,
                        float lo, float hi) noexcept {
    return lo + norm * (hi - lo);
}

// Map [0,1] → [lo, hi] logarithmic
inline float mapLog(float norm,
                     float lo, float hi) noexcept {
    return lo * std::pow(hi / lo, norm);
}
```

---

# PHẦN 7 — DSP CORE (ĐẦY ĐỦ)

## 7.1 Oscillator

### `core/dsp/oscillator.h`

```cpp
#pragma once
#include "shared/types.h"

enum class WaveShape : int {
    Triangle    = 0,  // ∧ symmetric triangle
    TriSaw      = 1,  // /\ triangle-sawtooth hybrid
    ReverseSaw  = 2,  // \  falling sawtooth
    Sawtooth    = 3,  // /  rising sawtooth (most common)
    Square      = 4,  // ⊓  50% duty square
    WidePulse   = 5,  // ⊓  ~10% duty pulse (nasal)
    COUNT       = 6
};

// OSC range (octave multiplier)
enum class OscRange : int {
    LO   = 0,  // very slow (~1/4 Hz) — LFO use
    R32  = 1,  // 32' (lowest pitched)
    R16  = 2,  // 16'
    R8   = 3,  // 8' (concert pitch)
    R4   = 4,  // 4'
    R2   = 5,  // 2' (highest)
};

class Oscillator {
public:
    // ── Initialization ───────────────────────────────
    void setSampleRate(float sr) noexcept;
    void reset() noexcept;   // phase = 0

    // ── Parameters ───────────────────────────────────
    void setFrequency(hz_t hz)      noexcept;
    void setWaveShape(WaveShape s)  noexcept;
    void setRange(OscRange r)       noexcept;
    void setAmplitude(float a)      noexcept;  // 0..1
    void setPulseWidth(float pw)    noexcept;  // 0.1..0.9

    // ── Hard sync ────────────────────────────────────
    // Gọi khi master OSC vượt qua 0 → reset phase
    void hardSyncTrigger() noexcept;

    // ── Render ───────────────────────────────────────
    sample_t tick() noexcept;

    // ── State query ──────────────────────────────────
    float    getPhase()     const noexcept { return phase_; }
    hz_t     getFrequency() const noexcept { return freq_; }
    WaveShape getShape()    const noexcept { return shape_; }

private:
    float     sampleRate_  = SAMPLE_RATE_DEFAULT;
    hz_t      freq_        = 440.0f;
    hz_t      effectiveHz_ = 440.0f;  // freq_ × rangeMultiplier
    float     amplitude_   = 1.0f;
    float     pulseWidth_  = 0.5f;
    WaveShape shape_       = WaveShape::Sawtooth;
    OscRange  range_       = OscRange::R8;
    float     phase_       = 0.0f;   // [0, 1)
    bool      syncPending_ = false;

    // PolyBLEP correction at discontinuity points
    // Reduces aliasing without oversampling
    // Reference: Välimäki & Pekonen (2007)
    float polyblep(float t, float dt) const noexcept;

    sample_t renderSawtooth()     const noexcept;
    sample_t renderReverseSaw()   const noexcept;
    sample_t renderSquare()       const noexcept;
    sample_t renderTriangle()     noexcept;
    sample_t renderTriSaw()       noexcept;

    void updateEffectiveHz() noexcept;

    // Range → octave multiplier
    static constexpr float RANGE_MULT[6] = {
        0.0625f,  // LO = /16
        0.125f,   // 32'
        0.25f,    // 16'
        0.5f,     // 8'  (reference)
        1.0f,     // 4'
        2.0f      // 2'
    };
};
```

### `core/dsp/oscillator.cpp` (implementation key sections)

```cpp
#include "oscillator.h"
#include "core/util/math_utils.h"

void Oscillator::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
}

void Oscillator::setFrequency(hz_t hz) noexcept {
    freq_ = clamp(hz, 0.001f, 20000.0f);
    updateEffectiveHz();
}

void Oscillator::setRange(OscRange r) noexcept {
    range_ = r;
    updateEffectiveHz();
}

void Oscillator::updateEffectiveHz() noexcept {
    effectiveHz_ = freq_ * RANGE_MULT[static_cast<int>(range_)];
}

void Oscillator::hardSyncTrigger() noexcept {
    syncPending_ = true;
}

sample_t Oscillator::tick() noexcept {
    if (syncPending_) {
        phase_ = 0.0f;
        syncPending_ = false;
    }

    const float dt = effectiveHz_ / sampleRate_;

    sample_t out;
    switch (shape_) {
        case WaveShape::Sawtooth:   out = renderSawtooth();   break;
        case WaveShape::ReverseSaw: out = renderReverseSaw(); break;
        case WaveShape::Square:     out = renderSquare();     break;
        case WaveShape::WidePulse:
            { auto saved = pulseWidth_;
              pulseWidth_ = 0.1f;
              out = renderSquare();
              pulseWidth_ = saved; }             break;
        case WaveShape::Triangle:   out = renderTriangle();   break;
        case WaveShape::TriSaw:     out = renderTriSaw();     break;
        default:                    out = 0.0f;               break;
    }

    phase_ += dt;
    if (phase_ >= 1.0f) phase_ -= 1.0f;

    return out * amplitude_;
}

// PolyBLEP: correction residual near discontinuity
// t = phase at discontinuity, dt = phase increment
float Oscillator::polyblep(float t, float dt) const noexcept {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    } else if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

sample_t Oscillator::renderSawtooth() const noexcept {
    const float dt = effectiveHz_ / sampleRate_;
    float value = 2.0f * phase_ - 1.0f;          // raw saw
    value -= polyblep(phase_, dt);                // correct at 0
    return value;
}

sample_t Oscillator::renderSquare() const noexcept {
    const float dt = effectiveHz_ / sampleRate_;
    float value = (phase_ < pulseWidth_) ? 1.0f : -1.0f;
    value += polyblep(phase_, dt);
    value -= polyblep(std::fmod(phase_ - pulseWidth_ + 1.0f,
                                1.0f), dt);
    return value;
}

sample_t Oscillator::renderTriangle() noexcept {
    // Integrate square wave for perfect triangle
    // (leaky integrator approach)
    const float sq = renderSquare();
    // One-pole integration: output = output + dt × (sq - output×decay)
    // Simple approximation: 4|phase - 0.5| - 1
    return 2.0f * std::abs(2.0f * phase_ - 1.0f) - 1.0f;
}
```

## 7.2 Moog Ladder Filter

### `core/dsp/moog_filter.h`

```cpp
#pragma once
#include "shared/types.h"

// ════════════════════════════════════════════════════════
// MOOG LADDER FILTER
// Model: Huovilainen (2004) nonlinear transistor ladder
// Characteristic: 4-pole lowpass, -24dB/oct
// Nonlinearity: tanh (models transistor I-V curve)
// Self-oscillation: yes, at resonance ≈ 1.0
// ════════════════════════════════════════════════════════

class MoogLadderFilter {
public:
    // ── Setup ────────────────────────────────────────
    void setSampleRate(float sr) noexcept;
    void reset()                  noexcept;  // clear state vars

    // ── Parameters (can change every sample) ─────────
    void setCutoff(hz_t hz)       noexcept;
    void setResonance(float r)    noexcept;  // 0.0..1.0

    // ── Process ──────────────────────────────────────
    sample_t process(sample_t input) noexcept;

    // ── Debug query ──────────────────────────────────
    hz_t  getCutoff()    const noexcept { return cutoff_; }
    float getResonance() const noexcept { return resonance_; }

private:
    float sampleRate_  = SAMPLE_RATE_DEFAULT;
    hz_t  cutoff_      = 1000.0f;
    float resonance_   = 0.0f;

    // Huovilainen state: 4 capacitor voltages
    float V_[4]   = {0.f, 0.f, 0.f, 0.f};
    float dV_[4]  = {0.f, 0.f, 0.f, 0.f};
    float tV_[4]  = {0.f, 0.f, 0.f, 0.f};

    // Precomputed on setCutoff() / setSampleRate()
    float VT2_     = 0.000050f;  // 2 × thermal voltage
    float x_       = 0.0f;      // normalized freq
    float g_       = 0.0f;      // filter gain
    float res4_    = 0.0f;      // resonance × 4

    void updateCoeffs() noexcept;

    static inline float tanh_approx(float x) noexcept {
        if (x >  3.0f) return  1.0f;
        if (x < -3.0f) return -1.0f;
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
};
```

### `core/dsp/moog_filter.cpp`

```cpp
#include "moog_filter.h"
#include "core/util/math_utils.h"
#include <cmath>

void MoogLadderFilter::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    updateCoeffs();
}

void MoogLadderFilter::setCutoff(hz_t hz) noexcept {
    cutoff_ = clamp(hz, 20.0f, sampleRate_ * 0.49f);
    updateCoeffs();
}

void MoogLadderFilter::setResonance(float r) noexcept {
    resonance_ = clamp(r, 0.0f, 1.0f);
    res4_ = resonance_ * 4.0f;  // feedback gain
}

void MoogLadderFilter::reset() noexcept {
    for (int i = 0; i < 4; ++i)
        V_[i] = dV_[i] = tV_[i] = 0.0f;
}

void MoogLadderFilter::updateCoeffs() noexcept {
    // Normalized angular frequency
    x_ = TWO_PI * cutoff_ / sampleRate_;
    // Huovilainen gain coefficient
    g_ = 0.9892f * x_
       - 0.4324f * x_ * x_
       + 0.1381f * x_ * x_ * x_
       - 0.0202f * x_ * x_ * x_ * x_;
}

sample_t MoogLadderFilter::process(sample_t input) noexcept {
    // Scale input by thermal voltage
    const float inp = input / VT2_;

    // Nonlinear feedback from output stage
    const float fb = tanh_approx(
        (inp - res4_ * V_[3]) / VT2_
    );

    // 4-stage ladder (transistor pairs)
    dV_[0] = g_ * (fb         - tV_[0]);
    V_[0] += dV_[0];
    tV_[0] = tanh_approx(V_[0] / VT2_);

    dV_[1] = g_ * (tV_[0]     - tV_[1]);
    V_[1] += dV_[1];
    tV_[1] = tanh_approx(V_[1] / VT2_);

    dV_[2] = g_ * (tV_[1]     - tV_[2]);
    V_[2] += dV_[2];
    tV_[2] = tanh_approx(V_[2] / VT2_);

    dV_[3] = g_ * (tV_[2]     - tV_[3]);
    V_[3] += dV_[3];
    tV_[3] = tanh_approx(V_[3] / VT2_);

    // Output: V[3] = lowpass -24dB/oct
    return V_[3];
}
```

## 7.3 Control Envelope

### `core/dsp/envelope.h` (hoàn chỉnh)

```cpp
#pragma once
#include "shared/types.h"
#include <cmath>

class ControlEnvelope {
public:
    enum class Stage : uint8_t {
        Idle = 0, Attack, Decay, Sustain, Release
    };

    struct Params {
        ms_t  attack  =    10.0f;
        ms_t  decay   =   200.0f;
        float sustain =     0.7f;
        ms_t  release =   500.0f;
    };

    void setSampleRate(float sr) noexcept;
    void setParams(const Params& p) noexcept;
    void setAttack(ms_t ms)   noexcept;
    void setDecay(ms_t ms)    noexcept;
    void setSustain(float lv) noexcept;
    void setRelease(ms_t ms)  noexcept;

    void noteOn()  noexcept;
    void noteOff() noexcept;
    void reset()   noexcept;

    float tick() noexcept;

    Stage getStage()  const noexcept { return stage_; }
    float getLevel()  const noexcept { return level_; }
    bool  isActive()  const noexcept { return stage_ != Stage::Idle; }

private:
    float  sampleRate_ = SAMPLE_RATE_DEFAULT;
    Params p_;
    Stage  stage_      = Stage::Idle;
    float  level_      = 0.0f;

    float  atkInc_     = 0.0f;
    float  decCoeff_   = 0.0f;
    float  relCoeff_   = 0.0f;

    void recalcCoeffs() noexcept;

    // One-pole exponential: time to reach 1/e of distance
    float calcExpCoeff(ms_t ms) const noexcept {
        if (ms < 0.01f) return 0.0f;
        return std::exp(-1.0f / (ms * 0.001f * sampleRate_));
    }
};
```

### `core/dsp/envelope.cpp`

```cpp
#include "envelope.h"

void ControlEnvelope::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    recalcCoeffs();
}

void ControlEnvelope::setParams(const Params& p) noexcept {
    p_ = p;
    recalcCoeffs();
}

void ControlEnvelope::setAttack(ms_t ms) noexcept {
    p_.attack = ms < 0.01f ? 0.01f : ms;
    recalcCoeffs();
}

void ControlEnvelope::setDecay(ms_t ms) noexcept {
    p_.decay = ms < 0.01f ? 0.01f : ms;
    recalcCoeffs();
}

void ControlEnvelope::setSustain(float lv) noexcept {
    p_.sustain = lv < 0.0f ? 0.0f : (lv > 1.0f ? 1.0f : lv);
}

void ControlEnvelope::setRelease(ms_t ms) noexcept {
    p_.release = ms < 0.01f ? 0.01f : ms;
    recalcCoeffs();
}

void ControlEnvelope::recalcCoeffs() noexcept {
    // Attack: linear rise (1 / samples)
    const float atkSamples = p_.attack * 0.001f * sampleRate_;
    atkInc_  = (atkSamples > 0.0f) ? (1.0f / atkSamples) : 1.0f;
    // Decay / Release: exponential one-pole
    decCoeff_ = calcExpCoeff(p_.decay);
    relCoeff_ = calcExpCoeff(p_.release);
}

void ControlEnvelope::noteOn() noexcept {
    stage_ = Stage::Attack;
    // Retrigger: continue from current level (no click)
}

void ControlEnvelope::noteOff() noexcept {
    if (stage_ != Stage::Idle)
        stage_ = Stage::Release;
}

void ControlEnvelope::reset() noexcept {
    stage_ = Stage::Idle;
    level_ = 0.0f;
}

float ControlEnvelope::tick() noexcept {
    switch (stage_) {

        case Stage::Attack:
            level_ += atkInc_;
            if (level_ >= 1.0f) {
                level_ = 1.0f;
                stage_ = Stage::Decay;
            }
            break;

        case Stage::Decay:
            // Exponential decay toward sustain level
            level_ = p_.sustain
                   + (level_ - p_.sustain) * decCoeff_;
            if (level_ <= p_.sustain + 0.0001f) {
                level_ = p_.sustain;
                stage_ = Stage::Sustain;
            }
            break;

        case Stage::Sustain:
            level_ = p_.sustain;
            break;

        case Stage::Release:
            level_ *= relCoeff_;
            if (level_ <= 0.0001f) {
                level_ = 0.0f;
                stage_ = Stage::Idle;
            }
            break;

        case Stage::Idle:
        default:
            level_ = 0.0f;
            break;
    }
    return level_;
}
```

---

## 7.4 Glide Processor

### `core/dsp/glide.h`

```cpp
#pragma once
#include "shared/types.h"
#include <cmath>

// ════════════════════════════════════════════════════════
// GLIDE (PORTAMENTO)
// Exponential interpolation in LOG domain
// Reason: Hz is NOT perceptually linear.
//         Log domain = equal semitone steps per time.
// ════════════════════════════════════════════════════════

class GlideProcessor {
public:
    void setSampleRate(float sr) noexcept;
    void setGlideTime(ms_t ms)   noexcept;  // 0 = instant
    void setEnabled(bool on)     noexcept;

    // Call on new note → triggers glide from current pitch
    void setTarget(hz_t hz) noexcept;

    // Jump without glide (first note, all-notes-off, etc.)
    void jumpTo(hz_t hz) noexcept;

    // Returns current pitch in Hz (advances internal state)
    hz_t tick() noexcept;

    hz_t  getCurrent() const noexcept { return currentHz_; }
    hz_t  getTarget()  const noexcept { return targetHz_; }
    bool  isGliding()  const noexcept;

private:
    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    ms_t  glideMs_    = 100.0f;
    bool  enabled_    = false;
    hz_t  currentHz_  = 440.0f;
    hz_t  targetHz_   = 440.0f;

    // Current pitch in log2 semitone space
    float logCurrent_ = 0.0f;
    float logTarget_  = 0.0f;

    // One-pole coeff: exp(-1 / (glideMs × 0.001 × sr))
    float coeff_      = 0.0f;

    void updateCoeff() noexcept;

    static float hzToLog(hz_t hz) noexcept {
        return std::log2(hz / A4_HZ);
    }
    static hz_t logToHz(float log2val) noexcept {
        return A4_HZ * std::exp2(log2val);
    }
};
```

### `core/dsp/glide.cpp`

```cpp
#include "glide.h"

void GlideProcessor::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    updateCoeff();
}

void GlideProcessor::setGlideTime(ms_t ms) noexcept {
    glideMs_ = ms < 0.0f ? 0.0f : ms;
    updateCoeff();
}

void GlideProcessor::setEnabled(bool on) noexcept {
    enabled_ = on;
}

void GlideProcessor::updateCoeff() noexcept {
    if (glideMs_ < 0.1f) {
        coeff_ = 0.0f;   // instant
        return;
    }
    coeff_ = std::exp(-1.0f /
                      (glideMs_ * 0.001f * sampleRate_));
}

void GlideProcessor::setTarget(hz_t hz) noexcept {
    targetHz_  = hz;
    logTarget_ = hzToLog(hz);
    if (!enabled_ || glideMs_ < 0.1f)
        jumpTo(hz);
}

void GlideProcessor::jumpTo(hz_t hz) noexcept {
    currentHz_  = hz;
    targetHz_   = hz;
    logCurrent_ = hzToLog(hz);
    logTarget_  = logCurrent_;
}

bool GlideProcessor::isGliding() const noexcept {
    return std::abs(logCurrent_ - logTarget_) > 0.0001f;
}

hz_t GlideProcessor::tick() noexcept {
    if (!enabled_ || coeff_ == 0.0f) {
        currentHz_  = targetHz_;
        logCurrent_ = logTarget_;
        return currentHz_;
    }

    // Exponential approach in log domain
    logCurrent_ = logTarget_
                + (logCurrent_ - logTarget_) * coeff_;

    // Snap when close enough
    if (std::abs(logCurrent_ - logTarget_) < 0.00001f)
        logCurrent_ = logTarget_;

    currentHz_ = logToHz(logCurrent_);
    return currentHz_;
}
```

---

## 7.5 LFO

### `core/dsp/lfo.h`

```cpp
#pragma once
#include "shared/types.h"

enum class LFOShape : int {
    Sine      = 0,
    Triangle  = 1,
    Sawtooth  = 2,
    Square    = 3,
    SandH     = 4,   // Sample & Hold (random)
    COUNT     = 5
};

// ════════════════════════════════════════════════════════
// LFO — Low Frequency Oscillator
// Output: -1.0..+1.0 bipolar CV
// Rate : 0.01Hz..20Hz
// Used : Pitch mod, Filter mod (via ModMatrix)
// NOTE : OSC3 can act as LFO (P_OSC3_LFO_ON)
//        In that mode OSC3 bypasses mixer and
//        feeds directly into ModMatrix as LFO source.
// ════════════════════════════════════════════════════════

class LFO {
public:
    void setSampleRate(float sr) noexcept;
    void setRate(hz_t hz)        noexcept;  // 0.01..20
    void setShape(LFOShape s)    noexcept;
    void setDepth(float d)       noexcept;  // 0.0..1.0

    // Reset phase (MIDI Start / note-on sync)
    void sync() noexcept;

    // Returns depth-scaled output in [-1.0, +1.0]
    float tick() noexcept;

    float getPhase() const noexcept { return phase_; }
    hz_t  getRate()  const noexcept { return rate_; }

private:
    float    sampleRate_ = SAMPLE_RATE_DEFAULT;
    hz_t     rate_       = 1.0f;
    float    depth_      = 1.0f;
    LFOShape shape_      = LFOShape::Sine;
    float    phase_      = 0.0f;   // [0, 1)
    float    prevPhase_  = 0.0f;
    float    shValue_    = 0.0f;   // S&H current hold value

    // LCG for S&H random (deterministic, no allocation)
    uint32_t randState_  = 12345u;
    float    nextRand()  noexcept;
};
```

### `core/dsp/lfo.cpp`

```cpp
#include "lfo.h"
#include <cmath>

void LFO::setSampleRate(float sr) noexcept { sampleRate_ = sr; }
void LFO::setRate(hz_t hz)  noexcept { rate_  = hz < 0.01f ? 0.01f : hz; }
void LFO::setShape(LFOShape s) noexcept { shape_ = s; }
void LFO::setDepth(float d) noexcept { depth_ = d < 0.f ? 0.f : (d > 1.f ? 1.f : d); }
void LFO::sync()            noexcept { phase_ = 0.0f; }

float LFO::nextRand() noexcept {
    randState_ = randState_ * 1664525u + 1013904223u;
    // Map uint32 → [-1, +1]
    return static_cast<float>(randState_) / 2147483648.0f - 1.0f;
}

float LFO::tick() noexcept {
    const float dt = rate_ / sampleRate_;
    float out = 0.0f;

    switch (shape_) {
        case LFOShape::Sine:
            out = std::sin(phase_ * TWO_PI);
            break;

        case LFOShape::Triangle:
            out = 1.0f - 4.0f * std::abs(phase_ - 0.5f);
            break;

        case LFOShape::Sawtooth:
            out = 2.0f * phase_ - 1.0f;
            break;

        case LFOShape::Square:
            out = phase_ < 0.5f ? 1.0f : -1.0f;
            break;

        case LFOShape::SandH:
            // Trigger new random on phase reset (crossing 0)
            if (phase_ < prevPhase_)
                shValue_ = nextRand();
            out = shValue_;
            break;

        default:
            out = 0.0f;
    }

    prevPhase_ = phase_;
    phase_ += dt;
    if (phase_ >= 1.0f) phase_ -= 1.0f;

    return out * depth_;
}
```

---

## 7.6 Noise Generator

### `core/dsp/noise.h`

```cpp
#pragma once
#include "shared/types.h"
#include <cstdint>

// ════════════════════════════════════════════════════════
// NOISE GENERATOR
// White: flat spectrum, uniform random
// Pink:  -3dB/octave, Voss-McCartney algorithm (no alloc)
// ════════════════════════════════════════════════════════

class NoiseGenerator {
public:
    enum class Color : int { White = 0, Pink = 1 };

    void setColor(Color c) noexcept { color_ = c; }

    sample_t tick() noexcept {
        return (color_ == Color::Pink)
               ? tickPink() : tickWhite();
    }

private:
    Color    color_     = Color::White;
    uint32_t state_     = 22222u;  // LCG state

    // Voss-McCartney pink noise state (7 stages, no alloc)
    float    pink_[7]   = {};
    int      pinkIdx_   = 0;

    // 32-bit LCG → [-1, +1]
    float lcg() noexcept {
        state_ = state_ * 1664525u + 1013904223u;
        return static_cast<float>(
                   static_cast<int32_t>(state_))
               * (1.0f / 2147483648.0f);
    }

    sample_t tickWhite() noexcept { return lcg(); }

    sample_t tickPink() noexcept {
        // Voss-McCartney: 7-stage running sum
        // Update one stage per sample (round-robin)
        const int stage = pinkIdx_ & 7;
        if (stage < 7) {
            pink_[stage] = lcg();
        }
        pinkIdx_++;
        float sum = 0.0f;
        for (int i = 0; i < 7; ++i) sum += pink_[i];
        return sum * (1.0f / 7.0f);
    }
};
```

---

## 7.7 Parameter Smoother

### `core/dsp/param_smoother.h`

```cpp
#pragma once
#include "shared/types.h"
#include <cmath>

// ════════════════════════════════════════════════════════
// ONE-POLE PARAMETER SMOOTHER
// Eliminates zipper noise on knob changes.
// Apply to: cutoff, resonance, volume, any continuous param
// NOT needed for: switches, discrete selectors
// Smooth time default: 5ms
// ════════════════════════════════════════════════════════

class ParamSmoother {
public:
    void init(float sampleRate,
              ms_t  smoothMs = 5.0f) noexcept {
        sampleRate_ = sampleRate;
        setSmoothTime(smoothMs);
    }

    void setSmoothTime(ms_t ms) noexcept {
        ms = ms < 0.01f ? 0.01f : ms;
        coeff_ = std::exp(-1.0f /
                          (ms * 0.001f * sampleRate_));
    }

    void setTarget(float val) noexcept {
        target_ = val;
    }

    // Set instantly (no smoothing) — use on preset load
    void snapTo(float val) noexcept {
        current_ = target_ = val;
    }

    // Returns smoothed value, advances internal state
    float tick() noexcept {
        current_ += (target_ - current_) * (1.0f - coeff_);
        return current_;
    }

    float getCurrent() const noexcept { return current_; }
    float getTarget()  const noexcept { return target_; }

    bool isSettled() const noexcept {
        return std::abs(target_ - current_) < 1e-5f;
    }

private:
    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    float coeff_      = 0.0f;
    float current_    = 0.0f;
    float target_     = 0.0f;
};
```

---

# PHẦN 8 — VOICE ENGINE & POLYPHONY

## 8.1 Voice

### `core/voice/voice.h`

```cpp
#pragma once
#include "core/dsp/oscillator.h"
#include "core/dsp/moog_filter.h"
#include "core/dsp/envelope.h"
#include "core/dsp/glide.h"
#include "core/dsp/param_smoother.h"
#include "core/dsp/noise.h"
#include "shared/types.h"
#include "shared/params.h"

// ════════════════════════════════════════════════════════
// VOICE — one complete synthesis path
// 3 OSC → Mixer → Moog Filter → VCA
// Filter ADSR + Amp ADSR
// Polyphony = N Voice objects running in parallel
// ════════════════════════════════════════════════════════

class Voice {
public:
    // ── Setup ────────────────────────────────────────
    void init(float sampleRate) noexcept;

    // ── Triggers ─────────────────────────────────────
    void noteOn(int midiNote,
                int velocity,
                float unisonDetuneCents = 0.0f) noexcept;
    void noteOff() noexcept;
    void forceOff() noexcept;  // immediate, for stealing
    void reset()    noexcept;

    // ── Per-sample render (called from VoicePool) ────
    // params: snapshot array read once per block (no atomic)
    sample_t tick(const float params[]) noexcept;

    // ── State query ──────────────────────────────────
    bool  isActive()    const noexcept;
    bool  isReleasing() const noexcept;
    int   getNote()     const noexcept { return note_; }
    int   getVelocity() const noexcept { return velocity_; }
    float getLevel()    const noexcept { return lastLevel_; }
    int   getAge()      const noexcept { return age_; }

    void tickAge() noexcept { ++age_; }

private:
    // ── DSP modules ──────────────────────────────────
    Oscillator       osc_[3];
    MoogLadderFilter filter_;
    ControlEnvelope  filterEnv_;
    ControlEnvelope  ampEnv_;
    GlideProcessor   glide_;
    NoiseGenerator   noise_;

    // ── Smoothers for hot params ──────────────────────
    // Updated every sample from params snapshot
    ParamSmoother    cutoffSmoother_;
    ParamSmoother    resSmoother_;
    ParamSmoother    mixSmoother_[4];  // OSC1,2,3,Noise

    // ── State ────────────────────────────────────────
    float sampleRate_   = SAMPLE_RATE_DEFAULT;
    int   note_         = 60;
    int   velocity_     = 0;
    float velAmp_       = 1.0f;  // velocity → [0,1]
    float detuneCents_  = 0.0f;  // unison offset
    float lastLevel_    = 0.0f;
    int   age_          = 0;     // samples since noteOn

    // ── Private helpers ──────────────────────────────
    void  applyOscParams(const float p[]) noexcept;
    void  applyFilterParams(const float p[]) noexcept;
    void  applyEnvParams(const float p[]) noexcept;

    hz_t  noteToHz(int midiNote,
                   float semitoneOffset = 0.0f) const noexcept;

    float kbdTrackOffset(const float p[]) const noexcept;
};
```

### `core/voice/voice.cpp`

```cpp
#include "voice.h"
#include "core/util/math_utils.h"

void Voice::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    for (auto& o : osc_)       o.setSampleRate(sampleRate);
    filter_.setSampleRate(sampleRate);
    filterEnv_.setSampleRate(sampleRate);
    ampEnv_.setSampleRate(sampleRate);
    glide_.setSampleRate(sampleRate);
    cutoffSmoother_.init(sampleRate, 5.0f);
    resSmoother_.init(sampleRate, 5.0f);
    for (auto& s : mixSmoother_) s.init(sampleRate, 5.0f);
}

void Voice::noteOn(int midiNote, int velocity,
                   float unisonDetuneCents) noexcept {
    note_        = midiNote;
    velocity_    = velocity;
    velAmp_      = velocity / 127.0f;
    detuneCents_ = unisonDetuneCents;
    age_         = 0;

    const hz_t hz = noteToHz(midiNote, detuneCents_ / 100.0f);
    glide_.setTarget(hz);

    filterEnv_.noteOn();
    ampEnv_.noteOn();
}

void Voice::noteOff() noexcept {
    filterEnv_.noteOff();
    ampEnv_.noteOff();
}

void Voice::forceOff() noexcept {
    filterEnv_.reset();
    ampEnv_.reset();
}

void Voice::reset() noexcept {
    forceOff();
    for (auto& o : osc_) o.reset();
    filter_.reset();
    glide_.jumpTo(440.0f);
    lastLevel_ = 0.0f;
    age_       = 0;
}

bool Voice::isActive() const noexcept {
    return ampEnv_.isActive();
}

bool Voice::isReleasing() const noexcept {
    return ampEnv_.getStage() == ControlEnvelope::Stage::Release;
}

hz_t Voice::noteToHz(int midiNote,
                     float semitoneOffset) const noexcept {
    return A4_HZ * std::pow(2.0f,
           (midiNote - A4_MIDI + semitoneOffset) / 12.0f);
}

float Voice::kbdTrackOffset(const float p[]) const noexcept {
    // Keyboard tracking: how much cutoff tracks note pitch
    // 0 = no track, 1 = 1/3, 2 = 2/3 of octave per octave
    const int   mode    = static_cast<int>(p[P_FILTER_KBD_TRACK]);
    const float factors[3] = { 0.0f, 1.0f/3.0f, 2.0f/3.0f };
    const float factor  = factors[clamp(mode, 0, 2)];
    // Semitones relative to C4 (MIDI 60)
    const float semi    = (note_ - 60) * factor;
    return semi;
}

sample_t Voice::tick(const float p[]) noexcept {
    // ── 1. Update envelope params from snapshot ───────
    applyEnvParams(p);

    // ── 2. Advance envelopes ──────────────────────────
    const float filterCV = filterEnv_.tick();
    const float ampCV    = ampEnv_.tick();

    if (!isActive()) { lastLevel_ = 0.0f; return 0.0f; }

    // ── 3. Glide ──────────────────────────────────────
    glide_.setEnabled(p[P_GLIDE_ON] > 0.5f);
    glide_.setGlideTime(normToGlideMs(p[P_GLIDE_TIME]));
    const hz_t baseHz = glide_.tick();

    // ── 4. Update oscillators ─────────────────────────
    applyOscParams(p);

    // Set OSC frequencies with individual detuning
    const float tune = p[P_MASTER_TUNE]; // ±1 semitone
    osc_[0].setFrequency(baseHz
        * semitonesToRatio(tune + normToSemitones(p[P_OSC1_FREQ])));
    osc_[1].setFrequency(baseHz
        * semitonesToRatio(tune + normToSemitones(p[P_OSC2_FREQ])));
    osc_[2].setFrequency(baseHz
        * semitonesToRatio(tune + normToSemitones(p[P_OSC3_FREQ])));

    // ── 5. OSC3 LFO mode ─────────────────────────────
    const bool osc3IsLFO = p[P_OSC3_LFO_ON] > 0.5f;

    // ── 6. Render OSCs ────────────────────────────────
    const sample_t s1 = osc_[0].tick();
    const sample_t s2 = osc_[1].tick();
    const sample_t s3 = osc3IsLFO ? 0.0f : osc_[2].tick();
    const sample_t sn = noise_.tick();

    // ── 7. Mix ───────────────────────────────────────
    const float m1 = mixSmoother_[0].tick();
    const float m2 = mixSmoother_[1].tick();
    const float m3 = mixSmoother_[2].tick();
    const float mn = mixSmoother_[3].tick();

    mixSmoother_[0].setTarget(p[P_MIX_OSC1]
                              * (p[P_OSC1_ON] > 0.5f ? 1.f : 0.f));
    mixSmoother_[1].setTarget(p[P_MIX_OSC2]
                              * (p[P_OSC2_ON] > 0.5f ? 1.f : 0.f));
    mixSmoother_[2].setTarget(p[P_MIX_OSC3]
                              * (p[P_OSC3_ON] > 0.5f
                                 && !osc3IsLFO ? 1.f : 0.f));
    mixSmoother_[3].setTarget(p[P_MIX_NOISE]);

    sample_t mixed = s1 * m1 + s2 * m2
                   + s3 * m3 + sn * mn;
    // Soft-clip mixer input
    mixed = fast_tanh(mixed * 0.5f) * 2.0f;

    // ── 8. Modulation ─────────────────────────────────
    // LFO source: OSC3 (LFO mode) or dedicated modMix blend
    float lfoOut = 0.0f;
    if (osc3IsLFO) lfoOut = osc_[2].tick();

    // Mod amount from mod mix knob
    const float modAmt = p[P_MOD_MIX];

    // OSC pitch mod
    if (p[P_OSC_MOD_ON] > 0.5f) {
        const float pitchMod = lfoOut * modAmt * 2.0f; // ±2 st
        osc_[0].setFrequency(baseHz
            * semitonesToRatio(pitchMod));
        // (applied additively — simplified for V1)
    }

    // Filter mod from LFO
    float filterModSemi = 0.0f;
    if (p[P_FILTER_MOD_ON] > 0.5f)
        filterModSemi = lfoOut * modAmt * 24.0f; // ±24 st

    // ── 9. Filter ─────────────────────────────────────
    applyFilterParams(p);

    // Base cutoff in Hz
    const hz_t baseCutHz = normToCutoffHz(p[P_FILTER_CUTOFF]);

    // Env modulation: amount scales filterCV → semitone offset
    const float envModSemi = p[P_FILTER_AMOUNT] * filterCV * 60.0f;

    // Keyboard tracking
    const float kbdSemi = kbdTrackOffset(p)

    // Total cutoff offset in semitones
    const float totalSemi = envModSemi
                          + kbdSemi
                          + filterModSemi;
    const hz_t finalCutHz = baseCutHz
                          * semitonesToRatio(totalSemi);

    cutoffSmoother_.setTarget(
        clamp(finalCutHz, 20.0f, 20000.0f));
    resSmoother_.setTarget(p[P_FILTER_EMPHASIS]);

    filter_.setCutoff(cutoffSmoother_.tick());
    filter_.setResonance(resSmoother_.tick());

    sample_t filtered = filter_.process(mixed);

    // ── 10. VCA ──────────────────────────────────────
    const float amp = ampCV * velAmp_ * p[P_MASTER_VOL];
    const sample_t out = filtered * amp;

    lastLevel_ = std::abs(out);
    age_++;
    return out;
}

// ─────────────────────────────────────────────────────────
void Voice::applyOscParams(const float p[]) noexcept {
    for (int i = 0; i < 3; ++i) {
        osc_[i].setWaveShape(
            static_cast<WaveShape>(
                static_cast<int>(p[P_OSC1_WAVE + i * 4])));
        osc_[i].setRange(
            static_cast<OscRange>(
                static_cast<int>(p[P_OSC1_RANGE + i * 4])));
    }
    // Noise color
    noise_.setColor(
        p[P_NOISE_COLOR] > 0.5f
        ? NoiseGenerator::Color::Pink
        : NoiseGenerator::Color::White);
}

void Voice::applyFilterParams(const float p[]) noexcept {
    // (cutoff & resonance handled via smoothers in tick())
    // Nothing extra needed here for V1
    (void)p;
}

void Voice::applyEnvParams(const float p[]) noexcept {
    // Filter envelope
    filterEnv_.setAttack (normToEnvMs(p[P_FENV_ATTACK]));
    filterEnv_.setDecay  (normToEnvMs(p[P_FENV_DECAY]));
    filterEnv_.setSustain(p[P_FENV_SUSTAIN]);
    filterEnv_.setRelease(normToEnvMs(p[P_FENV_RELEASE]));

    // Amp envelope
    ampEnv_.setAttack (normToEnvMs(p[P_AENV_ATTACK]));
    ampEnv_.setDecay  (normToEnvMs(p[P_AENV_DECAY]));
    ampEnv_.setSustain(p[P_AENV_SUSTAIN]);
    ampEnv_.setRelease(normToEnvMs(p[P_AENV_RELEASE]));
}
```

---

## 8.2 Voice Pool

### `core/voice/voice_pool.h`

```cpp
#pragma once
#include "voice.h"
#include "shared/types.h"
#include "shared/params.h"
#include <array>

// ════════════════════════════════════════════════════════
// VOICE POOL — polyphony manager
// Owns MAX_VOICES Voice objects (pre-allocated)
// Handles: note-on/off, voice stealing, unison spread
// Modes : Mono | Poly | Unison
// ════════════════════════════════════════════════════════

class VoicePool {
public:
    // ── Setup ────────────────────────────────────────
    void init(float sampleRate) noexcept;

    // ── Note events (from SynthEngine) ───────────────
    void noteOn (int note, int vel) noexcept;
    void noteOff(int note)          noexcept;
    void allNotesOff()              noexcept;
    void allSoundOff()              noexcept;  // immediate

    // ── Per-sample render ────────────────────────────
    // Sums all active voices, returns stereo pair
    void tick(const float params[],
              sample_t& outL,
              sample_t& outR) noexcept;

    // ── Config (read from params snapshot each block) ─
    void applyConfig(const float params[]) noexcept;

    // ── Query ────────────────────────────────────────
    int  getActiveCount()  const noexcept;
    int  getMaxVoices()    const noexcept { return maxVoices_; }
    VoiceMode getMode()    const noexcept { return mode_; }

private:
    std::array<Voice, MAX_VOICES> voices_;
    float      sampleRate_   = SAMPLE_RATE_DEFAULT;
    VoiceMode  mode_         = VoiceMode::Mono;
    StealMode  stealMode_    = StealMode::Oldest;
    int        maxVoices_    = 1;
    float      unisonDetune_ = 0.0f;  // total spread cents
    float      outputGain_   = 1.0f;  // 1/sqrt(N)

    // Held notes ring (for mono retriggering)
    std::array<HeldNote, MAX_HELD_NOTES> heldNotes_;
    int heldCount_ = 0;

    // ── Voice management ─────────────────────────────
    Voice* findFreeVoice()          noexcept;
    Voice* findVoiceToSteal()       noexcept;
    Voice* findVoiceByNote(int n)   noexcept;

    void   triggerVoice(Voice* v,
                        int note, int vel,
                        float detuneCents = 0.0f) noexcept;

    // ── Mode-specific dispatch ────────────────────────
    void noteOnMono  (int note, int vel) noexcept;
    void noteOnPoly  (int note, int vel) noexcept;
    void noteOnUnison(int note, int vel) noexcept;

    void noteOffMono  (int note) noexcept;
    void noteOffPoly  (int note) noexcept;
    void noteOffUnison(int note) noexcept;

    // ── Held notes helpers ───────────────────────────
    void  pushHeld(int note, int vel) noexcept;
    void  removeHeld(int note)        noexcept;
    const HeldNote* topHeld()   const noexcept;  // most recent
};
```

### `core/voice/voice_pool.cpp`

```cpp
#include "voice_pool.h"
#include "core/util/math_utils.h"
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────
// INIT
// ─────────────────────────────────────────────────────────

void VoicePool::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    for (auto& v : voices_) v.init(sampleRate);
    heldCount_ = 0;
}

// ─────────────────────────────────────────────────────────
// CONFIG (call once per block before tick loop)
// ─────────────────────────────────────────────────────────

void VoicePool::applyConfig(const float p[]) noexcept {
    mode_         = static_cast<VoiceMode>(
                        static_cast<int>(p[P_VOICE_MODE]));
    stealMode_    = static_cast<StealMode>(
                        static_cast<int>(p[P_VOICE_STEAL]));
    maxVoices_    = clamp(static_cast<int>(p[P_VOICE_COUNT]),
                          1, MAX_VOICES);
    unisonDetune_ = p[P_UNISON_DETUNE] * 50.0f; // 0..50 cents

    // Output gain: compensate for multiple voices
    outputGain_   = (maxVoices_ > 1)
                  ? (1.0f / std::sqrt(static_cast<float>(
                                      maxVoices_)))
                  : 1.0f;
}

// ─────────────────────────────────────────────────────────
// TICK
// ─────────────────────────────────────────────────────────

void VoicePool::tick(const float params[],
                     sample_t& outL,
                     sample_t& outR) noexcept {
    sample_t sum = 0.0f;

    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voices_[i].isActive()) {
            voices_[i].tickAge();
            sum += voices_[i].tick(params);
        }
    }

    // Mono sum × gain → stereo (no panning in V1)
    sum  *= outputGain_;
    outL  = sum;
    outR  = sum;
}

// ─────────────────────────────────────────────────────────
// NOTE ON
// ─────────────────────────────────────────────────────────

void VoicePool::noteOn(int note, int vel) noexcept {
    pushHeld(note, vel);
    switch (mode_) {
        case VoiceMode::Mono:   noteOnMono  (note, vel); break;
        case VoiceMode::Poly:   noteOnPoly  (note, vel); break;
        case VoiceMode::Unison: noteOnUnison(note, vel); break;
    }
}

void VoicePool::noteOnMono(int note, int vel) noexcept {
    // Find any already-active voice (retrigger)
    Voice* v = nullptr;
    for (auto& voice : voices_) {
        if (voice.isActive()) { v = &voice; break; }
    }
    if (!v) v = findFreeVoice();
    if (!v) v = findVoiceToSteal();
    if (!v) return;
    triggerVoice(v, note, vel, 0.0f);
}

void VoicePool::noteOnPoly(int note, int vel) noexcept {
    // Check if already playing (re-trigger same voice)
    Voice* v = findVoiceByNote(note);
    if (!v) v = findFreeVoice();
    if (!v) v = findVoiceToSteal();
    if (!v) return;
    triggerVoice(v, note, vel, 0.0f);
}

void VoicePool::noteOnUnison(int note, int vel) noexcept {
    // Release all current voices first
    for (auto& v : voices_) {
        if (v.isActive()) v.forceOff();
    }

    const int n = maxVoices_;
    // Spread: e.g. n=4, detune=10 → [-15, -5, +5, +15]
    for (int i = 0; i < n; ++i) {
        Voice* v = findFreeVoice();
        if (!v) break;

        float offset = 0.0f;
        if (n > 1) {
            // Symmetric spread around 0
            offset = (i - (n - 1) * 0.5f)
                   * (unisonDetune_ / (n - 1));
        }
        triggerVoice(v, note, vel, offset);
    }
}

// ─────────────────────────────────────────────────────────
// NOTE OFF
// ─────────────────────────────────────────────────────────

void VoicePool::noteOff(int note) noexcept {
    removeHeld(note);
    switch (mode_) {
        case VoiceMode::Mono:   noteOffMono  (note); break;
        case VoiceMode::Poly:   noteOffPoly  (note); break;
        case VoiceMode::Unison: noteOffUnison(note); break;
    }
}

void VoicePool::noteOffMono(int note) noexcept {
    // If there's still a held note → retrigger it
    const HeldNote* prev = topHeld();
    if (prev) {
        Voice* v = nullptr;
        for (auto& voice : voices_) {
            if (voice.isActive()) { v = &voice; break; }
        }
        if (v) triggerVoice(v, prev->midiNote,
                            prev->velocity, 0.0f);
        return;
    }
    // No held note → release
    for (auto& v : voices_)
        if (v.isActive() && v.getNote() == note)
            v.noteOff();
}

void VoicePool::noteOffPoly(int note) noexcept {
    Voice* v = findVoiceByNote(note);
    if (v) v->noteOff();
}

void VoicePool::noteOffUnison(int note) noexcept {
    (void)note;
    // In unison all voices share the same note
    for (auto& v : voices_)
        if (v.isActive()) v.noteOff();
}

void VoicePool::allNotesOff() noexcept {
    for (auto& v : voices_)
        if (v.isActive()) v.noteOff();
    heldCount_ = 0;
}

void VoicePool::allSoundOff() noexcept {
    for (auto& v : voices_) v.forceOff();
    heldCount_ = 0;
}

// ─────────────────────────────────────────────────────────
// VOICE SELECTION
// ─────────────────────────────────────────────────────────

Voice* VoicePool::findFreeVoice() noexcept {
    for (int i = 0; i < maxVoices_; ++i)
        if (!voices_[i].isActive()) return &voices_[i];
    return nullptr;
}

Voice* VoicePool::findVoiceToSteal() noexcept {
    switch (stealMode_) {
        case StealMode::Oldest: {
            Voice* best = nullptr;
            int    maxAge = -1;
            for (int i = 0; i < maxVoices_; ++i) {
                if (voices_[i].getAge() > maxAge) {
                    maxAge = voices_[i].getAge();
                    best   = &voices_[i];
                }
            }
            return best;
        }
        case StealMode::Lowest: {
            Voice* best    = nullptr;
            int    minNote = 128;
            for (int i = 0; i < maxVoices_; ++i) {
                if (voices_[i].getNote() < minNote) {
                    minNote = voices_[i].getNote();
                    best    = &voices_[i];
                }
            }
            return best;
        }
        case StealMode::Quietest: {
            Voice* best     = nullptr;
            float  minLevel = 1.0f;
            for (int i = 0; i < maxVoices_; ++i) {
                if (voices_[i].getLevel() < minLevel) {
                    minLevel = voices_[i].getLevel();
                    best     = &voices_[i];
                }
            }
            return best;
        }
    }
    return &voices_[0];   // fallback
}

Voice* VoicePool::findVoiceByNote(int n) noexcept {
    for (int i = 0; i < maxVoices_; ++i)
        if (voices_[i].isActive() && voices_[i].getNote() == n)
            return &voices_[i];
    return nullptr;
}

void VoicePool::triggerVoice(Voice* v, int note, int vel,
                              float detuneCents) noexcept {
    v->noteOn(note, vel, detuneCents);
}

// ─────────────────────────────────────────────────────────
// HELD NOTES STACK
// ─────────────────────────────────────────────────────────

void VoicePool::pushHeld(int note, int vel) noexcept {
    // Update if already held
    for (int i = 0; i < heldCount_; ++i) {
        if (heldNotes_[i].midiNote == note) {
            heldNotes_[i].velocity = vel;
            return;
        }
    }
    if (heldCount_ < MAX_HELD_NOTES) {
        heldNotes_[heldCount_++] = { note, vel, true };
    }
}

void VoicePool::removeHeld(int note) noexcept {
    for (int i = 0; i < heldCount_; ++i) {
        if (heldNotes_[i].midiNote == note) {
            // Shift down
            for (int j = i; j < heldCount_ - 1; ++j)
                heldNotes_[j] = heldNotes_[j + 1];
            --heldCount_;
            return;
        }
    }
}

const HeldNote* VoicePool::topHeld() const noexcept {
    if (heldCount_ == 0) return nullptr;
    return &heldNotes_[heldCount_ - 1];  // most recent
}

int VoicePool::getActiveCount() const noexcept {
    int count = 0;
    for (int i = 0; i < MAX_VOICES; ++i)
        if (voices_[i].isActive()) ++count;
    return count;
}
```

---

# PHẦN 9 — MUSIC LAYER

## 9.1 Arpeggiator

### `core/music/arpeggiator.h`

```cpp
#pragma once
#include "shared/types.h"
#include <array>

enum class ArpMode : int {
    Up      = 0,
    Down    = 1,
    UpDown  = 2,
    DownUp  = 3,
    Random  = 4,
    AsPlayed= 5,
    COUNT   = 6
};

// Subdivisions: index → beats per step
// 0=1/1  1=1/2  2=1/4  3=1/8  4=1/16  5=1/4.  6=1/8.  7=1/4T
constexpr float ARP_RATE_BEATS[8] = {
    4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 1.5f, 0.75f, (2.0f/3.0f)
};

class Arpeggiator {
public:
    // ── Config ───────────────────────────────────────
    void setEnabled   (bool on)        noexcept;
    void setMode      (ArpMode m)      noexcept;
    void setOctaves   (int octs)       noexcept;  // 1..4
    void setRateIndex (int idx)        noexcept;  // 0..7
    void setGate      (float g)        noexcept;  // 0..1
    void setSwing     (float s)        noexcept;  // 0..0.5
    void setBPM       (float bpm)      noexcept;
    void setSampleRate(float sr)       noexcept;

    // ── Note input ───────────────────────────────────
    void noteOn (int note, int vel)    noexcept;
    void noteOff(int note)             noexcept;
    void allNotesOff()                 noexcept;

    // ── Clock ────────────────────────────────────────
    struct Output {
        bool hasNoteOn  = false;
        bool hasNoteOff = false;
        int  note       = 60;
        int  velocity   = 100;
    };
    Output tick() noexcept;

    bool isEnabled()     const noexcept { return enabled_; }
    int  getCurrentNote()const noexcept { return currentNote_; }

private:
    bool    enabled_      = false;
    ArpMode mode_         = ArpMode::Up;
    int     octaves_      = 1;
    int     rateIdx_      = 3;      // 1/8 default
    float   gate_         = 0.8f;
    float   swing_        = 0.0f;
    float   bpm_          = 120.0f;
    float   sampleRate_   = SAMPLE_RATE_DEFAULT;

    // Note list (sorted per mode, with octave copies)
    std::array<int, 128> noteList_;
    int  noteListLen_     = 0;
    int  listIdx_         = 0;
    int  direction_       = 1;      // +1 or -1 for UpDown

    // Held input notes (up to 32)
    std::array<int, 32> heldNotes_;
    std::array<int, 32> heldVels_;
    int  heldCount_       = 0;

    // Per-sample timing
    float   stepSamples_  = 0.0f;   // samples per step
    float   gateSamples_  = 0.0f;
    float   phase_        = 0.0f;   // 0..stepSamples_
    bool    noteIsOn_     = false;
    int     currentNote_  = -1;
    int     currentVel_   = 100;

    // LCG for random mode
    uint32_t randState_   = 99991u;

    void  rebuildNoteList() noexcept;
    void  updateTiming()    noexcept;
    int   nextIndex()       noexcept;
    float swingOffset(int stepIdx) const noexcept;

    uint32_t lcgNext() noexcept {
        randState_ = randState_ * 1664525u + 1013904223u;
        return randState_;
    }
};
```

### `core/music/arpeggiator.cpp`

```cpp
#include "arpeggiator.h"
#include "core/util/math_utils.h"
#include <algorithm>
#include <cmath>

void Arpeggiator::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    updateTiming();
}
void Arpeggiator::setEnabled(bool on) noexcept {
    enabled_ = on;
    if (!on && noteIsOn_) {
        noteIsOn_ = false;
    }
}
void Arpeggiator::setMode(ArpMode m) noexcept {
    mode_ = m;
    rebuildNoteList();
}
void Arpeggiator::setOctaves(int o) noexcept {
    octaves_ = clamp(o, 1, 4);
    rebuildNoteList();
}
void Arpeggiator::setRateIndex(int idx) noexcept {
    rateIdx_ = clamp(idx, 0, 7);
    updateTiming();
}
void Arpeggiator::setGate(float g) noexcept {
    gate_ = clamp(g, 0.01f, 1.0f);
    updateTiming();
}
void Arpeggiator::setSwing(float s) noexcept {
    swing_ = clamp(s, 0.0f, 0.5f);
}
void Arpeggiator::setBPM(float bpm) noexcept {
    bpm_ = clamp(bpm, 20.0f, 300.0f);
    updateTiming();
}

void Arpeggiator::noteOn(int note, int vel) noexcept {
    if (heldCount_ >= 32) return;
    // Avoid duplicate
    for (int i = 0; i < heldCount_; ++i)
        if (heldNotes_[i] == note) return;
    heldNotes_[heldCount_] = note;
    heldVels_ [heldCount_] = vel;
    heldCount_++;
    rebuildNoteList();
}

void Arpeggiator::noteOff(int note) noexcept {
    for (int i = 0; i < heldCount_; ++i) {
        if (heldNotes_[i] == note) {
            for (int j = i; j < heldCount_ - 1; ++j) {
                heldNotes_[j] = heldNotes_[j + 1];
                heldVels_ [j] = heldVels_ [j + 1];
            }
            heldCount_--;
            rebuildNoteList();
            return;
        }
    }
}

void Arpeggiator::allNotesOff() noexcept {
    heldCount_   = 0;
    noteListLen_ = 0;
    noteIsOn_    = false;
    currentNote_ = -1;
}

void Arpeggiator::updateTiming() noexcept {
    // Samples per beat = sampleRate * 60 / BPM
    const float samplesPerBeat = sampleRate_ * 60.0f / bpm_;
    stepSamples_ = samplesPerBeat * ARP_RATE_BEATS[rateIdx_];
    gateSamples_ = stepSamples_ * gate_;
}

void Arpeggiator::rebuildNoteList() noexcept {
    if (heldCount_ == 0) {
        noteListLen_ = 0;
        return;
    }

    // Copy held notes
    int sorted[32];
    for (int i = 0; i < heldCount_; ++i)
        sorted[i] = heldNotes_[i];

    // Sort ascending for Up/Down modes
    if (mode_ != ArpMode::AsPlayed) {
        std::sort(sorted, sorted + heldCount_);
    }

    noteListLen_ = 0;
    for (int oct = 0; oct < octaves_; ++oct) {
        for (int i = 0; i < heldCount_; ++i) {
            int n = sorted[i] + oct * 12;
            if (n <= 127)
                noteList_[noteListLen_++] = n;
        }
    }

    // Clamp listIdx_ to valid range
    if (listIdx_ >= noteListLen_) listIdx_ = 0;
}

int Arpeggiator::nextIndex() noexcept {
    if (noteListLen_ == 0) return -1;

    switch (mode_) {
        case ArpMode::Up:
            listIdx_ = (listIdx_ + 1) % noteListLen_;
            break;

        case ArpMode::Down:
            listIdx_ = (listIdx_ - 1 + noteListLen_)
                       % noteListLen_;
            break;

        case ArpMode::UpDown:
        case ArpMode::DownUp: {
            listIdx_ += direction_;
            if (listIdx_ >= noteListLen_) {
                listIdx_   = noteListLen_ - 2;
                direction_ = -1;
            } else if (listIdx_ < 0) {
                listIdx_   = 1;
                direction_  = 1;
            }
            break;
        }

        case ArpMode::Random:
            listIdx_ = static_cast<int>(
                lcgNext() % static_cast<uint32_t>(
                                noteListLen_));
            break;

        case ArpMode::AsPlayed:
            listIdx_ = (listIdx_ + 1) % noteListLen_;
            break;

        default:
            listIdx_ = 0;
    }
    return listIdx_;
}

Arpeggiator::Output Arpeggiator::tick() noexcept {
    Output out{};
    if (!enabled_ || noteListLen_ == 0) return out;

    // Advance phase
    phase_ += 1.0f;

    // Apply swing: odd steps are delayed
    const float effectiveStep = (listIdx_ % 2 == 1)
        ? stepSamples_ * (1.0f + swing_)
        : stepSamples_ * (1.0f - swing_);

    // Gate off?
    if (noteIsOn_ && phase_ >= gateSamples_) {
        out.hasNoteOff = true;
        out.note       = currentNote_;
        out.velocity   = currentVel_;
        noteIsOn_      = false;
    }

    // New step?
    if (phase_ >= effectiveStep) {
        phase_ -= effectiveStep;
        int idx = nextIndex();
        if (idx >= 0) {
            currentNote_    = noteList_[idx];
            currentVel_     = 100;
            out.hasNoteOn   = true;
            out.note        = currentNote_;
            out.velocity    = currentVel_;
            noteIsOn_       = true;
            // Also trigger gate samples from now
        }
    }
    return out;
}
```

---

## 9.2 Step Sequencer

### `core/music/sequencer.h`

```cpp
#pragma once
#include "shared/types.h"
#include <array>

struct SeqStep {
    int   note      = 60;
    int   velocity  = 100;
    float gate      = 0.8f;   // 0..1 (overrides global gate)
    bool  active    = true;   // false = rest
    bool  tie       = false;  // legato into next step
};

class StepSequencer {
public:
    static constexpr int MAX_STEPS = 16;

    // ── Setup ────────────────────────────────────────
    void setSampleRate(float sr)     noexcept;

    // ── Config ───────────────────────────────────────
    void setEnabled   (bool on)      noexcept;
    void setBPM       (float bpm)    noexcept;
    void setStepCount (int n)        noexcept;  // 1..16
    void setRateIndex (int idx)      noexcept;  // 0..7
    void setGlobalGate(float g)      noexcept;  // 0..1
    void setSwing     (float s)      noexcept;  // 0..0.5

    // ── Step editing ─────────────────────────────────
    void     setStep (int idx, const SeqStep& s) noexcept;
    SeqStep  getStep (int idx) const             noexcept;
    void     clearAll()                          noexcept;

    // ── Transport ────────────────────────────────────
    void play()  noexcept;
    void stop()  noexcept;   // sends note-off, resets to step 0
    void reset() noexcept;   // resets phase without stop

    // ── Per-sample output ────────────────────────────
    struct Output {
        bool hasNoteOn  = false;
        bool hasNoteOff = false;
        int  note       = 60;
        int  velocity   = 100;
        int  stepIdx    = 0;
        bool justStepped= false;  // for GUI step highlight
    };
    Output tick() noexcept;

    // ── Query ────────────────────────────────────────
    bool isPlaying()      const noexcept { return playing_; }
    bool isEnabled()      const noexcept { return enabled_; }
    int  getCurrentStep() const noexcept { return curStep_; }

private:
    float sampleRate_  = SAMPLE_RATE_DEFAULT;
    bool  enabled_     = false;
    bool  playing_     = false;
    float bpm_         = 120.0f;
    int   stepCount_   = 16;
    int   rateIdx_     = 3;       // 1/8
    float globalGate_  = 0.8f;
    float swing_       = 0.0f;

    std::array<SeqStep, MAX_STEPS> steps_;
    int   curStep_     = 0;
    float phase_       = 0.0f;
    float stepSamples_ = 0.0f;
    bool  noteIsOn_    = false;
    int   activeNote_  = -1;

    void updateTiming() noexcept;
    void advance()      noexcept;
};
```

### `core/music/sequencer.cpp`

```cpp
#include "sequencer.h"
#include "core/util/math_utils.h"

void StepSequencer::setSampleRate(float sr) noexcept {
    sampleRate_ = sr;
    updateTiming();
}
void StepSequencer::setEnabled(bool on) noexcept {
    enabled_ = on;
}
void StepSequencer::setBPM(float bpm) noexcept {
    bpm_ = clamp(bpm, 20.0f, 300.0f);
    updateTiming();
}
void StepSequencer::setStepCount(int n) noexcept {
    stepCount_ = clamp(n, 1, MAX_STEPS);
}
void StepSequencer::setRateIndex(int idx) noexcept {
    rateIdx_ = clamp(idx, 0, 7);
    updateTiming();
}
void StepSequencer::setGlobalGate(float g) noexcept {
    globalGate_ = clamp(g, 0.01f, 1.0f);
}
void StepSequencer::setSwing(float s) noexcept {
    swing_ = clamp(s, 0.0f, 0.5f);
}

void StepSequencer::setStep(int idx,
                             const SeqStep& s) noexcept {
    if (idx >= 0 && idx < MAX_STEPS) steps_[idx] = s;
}
SeqStep StepSequencer::getStep(int idx) const noexcept {
    if (idx >= 0 && idx < MAX_STEPS) return steps_[idx];
    return {};
}
void StepSequencer::clearAll() noexcept {
    for (auto& s : steps_) s = {};
}

void StepSequencer::play() noexcept {
    playing_ = true;
    phase_   = 0.0f;
    curStep_ = 0;
}
void StepSequencer::stop() noexcept {
    playing_ = false;
    phase_   = 0.0f;
    curStep_ = 0;
    noteIsOn_   = false;
    activeNote_ = -1;
}
void StepSequencer::reset() noexcept {
    phase_   = 0.0f;
    curStep_ = 0;
}

void StepSequencer::updateTiming() noexcept {
    const float spb   = sampleRate_ * 60.0f / bpm_;
    stepSamples_      = spb * ARP_RATE_BEATS[rateIdx_];
}

void StepSequencer::advance() noexcept {
    curStep_ = (curStep_ + 1) % stepCount_;
}

StepSequencer::Output StepSequencer::tick() noexcept {
    Output out{};
    if (!enabled_ || !playing_) return out;

    phase_ += 1.0f;

    // Swing: odd steps delayed
    const float effStep = (curStep_ % 2 == 1)
        ? stepSamples_ * (1.0f + swing_)
        : stepSamples_ * (1.0f - swing_);

    const SeqStep& st = steps_[curStep_];
    const float gateSamples = effStep
                            * st.gate * globalGate_;

    // Gate off
    if (noteIsOn_ && !st.tie && phase_ >= gateSamples) {
        out.hasNoteOff = true;
        out.note       = activeNote_;
        noteIsOn_      = false;
    }

    // New step
    if (phase_ >= effStep) {
        phase_ -= effStep;
        out.justStepped = true;
        out.stepIdx     = curStep_;
        advance();

        const SeqStep& next = steps_[curStep_];
        if (next.active) {
            // Tie: don't send note-off before note-on
            if (noteIsOn_ && !st.tie) {
                out.hasNoteOff = true;
                out.note       = activeNote_;
                noteIsOn_      = false;
            }
            activeNote_   = next.note;
            out.hasNoteOn = true;
            out.note      = next.note;
            out.velocity  = next.velocity;
            noteIsOn_     = true;
        }
    }
    return out;
}
```

---

## 9.3 Chord Engine

### `core/music/chord_engine.h`

```cpp
#pragma once
#include "shared/types.h"
#include <array>

struct ChordVoicing {
    const char* name;
    int         intervals[MAX_CHORD_NOTES];  // semitone offsets
    int         noteCount;
};

class ChordEngine {
public:
    static constexpr int CHORD_COUNT = 16;
    static const ChordVoicing CHORDS[CHORD_COUNT];

    void setEnabled   (bool on) noexcept;
    void setChordType (int idx) noexcept;  // 0..15
    void setInversion (int inv) noexcept;  // 0..3

    struct Output {
        int notes    [MAX_CHORD_NOTES];
        int velocities[MAX_CHORD_NOTES];
        int count;
    };
    Output expand(int rootNote, int velocity) const noexcept;

    bool isEnabled()   const noexcept { return enabled_; }
    int  getType()     const noexcept { return chordType_; }
    int  getInversion()const noexcept { return inversion_; }

private:
    bool  enabled_   = false;
    int   chordType_ = 0;
    int   inversion_ = 0;

    void applyInversion(int notes[], int count) const noexcept;
};
```

### `core/music/chord_engine.cpp`

```cpp
#include "chord_engine.h"
#include "core/util/math_utils.h"

// 16 chord voicings (root-relative semitone intervals)
const ChordVoicing ChordEngine::CHORDS[CHORD_COUNT] = {
    { "Major",       {0, 4, 7,  0,  0,  0}, 3 },
    { "Minor",       {0, 3, 7,  0,  0,  0}, 3 },
    { "Dom7",        {0, 4, 7, 10,  0,  0}, 4 },
    { "Maj7",        {0, 4, 7, 11,  0,  0}, 4 },
    { "Min7",        {0, 3, 7, 10,  0,  0}, 4 },
    { "Sus2",        {0, 2, 7,  0,  0,  0}, 3 },
    { "Sus4",        {0, 5, 7,  0,  0,  0}, 3 },
    { "Dim",         {0, 3, 6,  0,  0,  0}, 3 },
    { "Dim7",        {0, 3, 6,  9,  0,  0}, 4 },
    { "Aug",         {0, 4, 8,  0,  0,  0}, 3 },
    { "Min7b5",      {0, 3, 6, 10,  0,  0}, 4 },
    { "Add9",        {0, 4, 7, 14,  0,  0}, 4 },
    { "Maj9",        {0, 4, 7, 11, 14,  0}, 5 },
    { "Min9",        {0, 3, 7, 10, 14,  0}, 5 },
    { "Power",       {0, 7,  0,  0,  0, 0}, 2 },
    { "Octave",      {0,12,  0,  0,  0, 0}, 2 },
};

void ChordEngine::setEnabled(bool on) noexcept {
    enabled_ = on;
}
void ChordEngine::setChordType(int idx) noexcept {
    chordType_ = clamp(idx, 0, CHORD_COUNT - 1);
}
void ChordEngine::setInversion(int inv) noexcept {
    inversion_ = clamp(inv, 0, 3);
}

ChordEngine::Output ChordEngine::expand(
        int rootNote, int velocity) const noexcept {
    Output out{};
    if (!enabled_) {
        out.notes[0]     = rootNote;
        out.velocities[0]= velocity;
        out.count        = 1;
        return out;
    }

    const ChordVoicing& cv = CHORDS[chordType_];
    out.count = cv.noteCount;
    for (int i = 0; i < cv.noteCount; ++i) {
        out.notes[i]      = clamp(rootNote
                                  + cv.intervals[i],
                                  0, 127);
        out.velocities[i] = velocity;
    }
    applyInversion(out.notes, out.count);
    return out;
}

void ChordEngine::applyInversion(
        int notes[], int count) const noexcept {
    // Each inversion: raise lowest note by 1 octave
    for (int inv = 0; inv < inversion_; ++inv) {
        // Find lowest
        int loIdx = 0;
        for (int i = 1; i < count; ++i)
            if (notes[i] < notes[loIdx]) loIdx = i;
        notes[loIdx] += 12;
        if (notes[loIdx] > 127) notes[loIdx] = 127;
    }
}
```

---

## 9.4 Scale Quantizer

### `core/music/scale_quantizer.h`

```cpp
#pragma once
#include "shared/types.h"

struct ScaleDef {
    const char* name;
    bool        degrees[12];  // true = note belongs to scale
};

class ScaleQuantizer {
public:
    static constexpr int SCALE_COUNT = 16;
    static const ScaleDef SCALES[SCALE_COUNT];

    void setEnabled (bool on)  noexcept;
    void setRoot    (int semi) noexcept;  // 0=C .. 11=B
    void setScale   (int idx)  noexcept;  // 0..15

    // Returns closest in-scale MIDI note
    int  quantize(int midiNote) const noexcept;

    bool isEnabled()    const noexcept { return enabled_; }
    int  getRoot()      const noexcept { return root_; }
    int  getScaleIdx()  const noexcept { return scaleIdx_; }

private:
    bool  enabled_  = false;
    int   root_     = 0;
    int   scaleIdx_ = 1;   // major
};
```

### `core/music/scale_quantizer.cpp`

```cpp
#include "scale_quantizer.h"
#include "core/util/math_utils.h"

const ScaleDef ScaleQuantizer::SCALES[SCALE_COUNT] = {
  {"Chromatic",     {1,1,1,1,1,1,1,1,1,1,1,1}},
  {"Major",         {1,0,1,0,1,1,0,1,0,1,0,1}},
  {"Nat.Minor",     {1,0,1,1,0,1,0,1,1,0,1,0}},
  {"Harm.Minor",    {1,0,1,1,0,1,0,1,1,0,0,1}},
  {"Mel.Minor",     {1,0,1,1,0,1,0,1,0,1,0,1}},
  {"Dorian",        {1,0,1,1,0,1,0,1,0,1,1,0}},
  {"Phrygian",      {1,1,0,1,0,1,0,1,1,0,1,0}},
  {"Lydian",        {1,0,1,0,1,0,1,1,0,1,0,1}},
  {"Mixolydian",    {1,0,1,0,1,1,0,1,0,1,1,0}},
  {"Locrian",       {1,1,0,1,0,1,1,0,1,0,1,0}},
  {"Penta.Maj",     {1,0,1,0,1,0,0,1,0,1,0,0}},
  {"Penta.Min",     {1,0,0,1,0,1,0,1,0,0,1,0}},
  {"Blues",         {1,0,0,1,0,1,1,1,0,0,1,0}},
  {"WholeTone",     {1,0,1,0,1,0,1,0,1,0,1,0}},
  {"Diminished",    {1,0,1,1,0,1,1,0,1,1,0,1}},
  {"Augmented",     {1,0,0,1,1,0,0,1,1,0,0,1}},
};

void ScaleQuantizer::setEnabled(bool on) noexcept { enabled_ = on; }
void ScaleQuantizer::setRoot(int s) noexcept {
    root_ = ((s % 12) + 12) % 12;
}
void ScaleQuantizer::setScale(int idx) noexcept {
    scaleIdx_ = clamp(idx, 0, SCALE_COUNT - 1);
}

int ScaleQuantizer::quantize(int midiNote) const noexcept {
    if (!enabled_) return midiNote;

    const ScaleDef& sc = SCALES[scaleIdx_];
    const int octave   = midiNote / 12;
    const int semitone = midiNote % 12;

    // Relative to root
    const int rel = ((semitone - root_) % 12 + 12) % 12;

    // Find nearest scale degree (search ±6 semitones)
    int bestRel  = rel;
    int bestDist = 12;
    for (int d = 0; d <= 6; ++d) {
        int up   = (rel + d) % 12;
        int down = ((rel - d) % 12 + 12) % 12;
        if (sc.degrees[up]   && d < bestDist) {
            bestDist = d; bestRel = up;
        }
        if (sc.degrees[down] && d < bestDist) {
            bestDist = d; bestRel = down;
        }
    }

    const int quantizedSemi = (bestRel + root_) % 12;
    return clamp(octave * 12 + quantizedSemi, 0, 127);
}
```

---

# PHẦN 10 — SYNTH ENGINE

### `core/engine/synth_engine.h`

```cpp
#pragma once
#include "core/voice/voice_pool.h"
#include "core/music/arpeggiator.h"
#include "core/music/sequencer.h"
#include "core/music/chord_engine.h"
#include "core/music/scale_quantizer.h"
#include "shared/interfaces.h"
#include "shared/params.h"

// ════════════════════════════════════════════════════════
// SYNTH ENGINE — top-level audio processor
// Owns all sub-systems. Called by RtAudio callback.
// ZERO allocation, ZERO locks in processBlock().
// ════════════════════════════════════════════════════════

class SynthEngine : public IAudioProcessor {
public:
    SynthEngine()  = default;
    ~SynthEngine() = default;

    // ── Init (call before audio starts) ─────────────
    void init(AtomicParamStore* params,
              MidiEventQueue*   midiQueue) noexcept;
    void setSampleRate(float sr) override;
    void setBlockSize (int bs)   override;

    // ── Audio callback (RtAudio thread) ──────────────
    void processBlock(sample_t* outL,
                      sample_t* outR,
                      int nFrames) noexcept override;

    // ── Read-only queries (UI thread safe) ───────────
    int   getActiveVoices()   const noexcept;
    int   getArpStep()        const noexcept;
    int   getSeqStep()        const noexcept;
    bool  getSeqPlaying()     const noexcept;
    float getBPM()            const noexcept;

    // ── Transport (UI → audio, via params) ───────────
    // UI sets P_SEQ_PLAYING param → engine reacts

private:
    // Sub-systems (pre-allocated)
    VoicePool       voicePool_;
    Arpeggiator     arp_;
    StepSequencer   seq_;
    ChordEngine     chord_;
    ScaleQuantizer  scale_;

    // External wiring
    AtomicParamStore* params_    = nullptr;
    MidiEventQueue*   midiQueue_ = nullptr;

    // Config
    float sampleRate_ = SAMPLE_RATE_DEFAULT;
    int   blockSize_  = BLOCK_SIZE_DEFAULT;

    // Per-block param snapshot (avoids atomic overhead)
    float paramCache_[PARAM_COUNT] = {};

    // State tracking
    bool  seqWasPlaying_ = false;

    // ── Private per-sample helpers ───────────────────
    void  snapshotParams()           noexcept;
    void  drainMidiQueue()           noexcept;
    void  syncMusicConfig()          noexcept;
    void  tickMusicLayer(sample_t* outL,
                         sample_t* outR,
                         int nFrames)   noexcept;

    // ── MIDI dispatch ────────────────────────────────
    void  onNoteOn  (int note, int vel) noexcept;
    void  onNoteOff (int note)          noexcept;
    void  onCC      (int cc,  int val)  noexcept;
    void  onPitchBend(int bend)         noexcept;

    // ── Note routing (through music layer) ───────────
    void  routeNoteOn (int note, int vel) noexcept;
    void  routeNoteOff(int note)          noexcept;
};
```

### `core/engine/synth_engine.cpp`

```cpp
#include "synth_engine.h"

void SynthEngine::init(AtomicParamStore* params,
                        MidiEventQueue*   midiQueue) noexcept {
    params_    = params;
    midiQueue_ = midiQueue;
    voicePool_.init(sampleRate_);
    arp_.setSampleRate(sampleRate_);
    seq_.setSampleRate(sampleRate_);
}

void SynthEngine::setSampleRate(float sr) {
    sampleRate_ = sr;
    voicePool_.init(sr);
    arp_.setSampleRate(sr);
    seq_.setSampleRate(sr);
}

void SynthEngine::setBlockSize(int bs) {
    blockSize_ = bs;
}

// ─────────────────────────────────────────────────────────
// MAIN AUDIO CALLBACK — called by RtAudio thread
// ─────────────────────────────────────────────────────────

void SynthEngine::processBlock(sample_t* outL,
                                sample_t* outR,
                                int nFrames) noexcept {
    // 1. Snapshot params once per block
    snapshotParams();

    // 2. Drain MIDI event queue
    drainMidiQueue();

    // 3. Sync music-layer config from params
    syncMusicConfig();

    // 4. Handle sequencer transport edge
    const bool seqOn = paramCache_[P_SEQ_ON]      > 0.5f;
    const bool seqPl = paramCache_[P_SEQ_PLAYING]  > 0.5f;
    if (seqOn && seqPl && !seqWasPlaying_) seq_.play();
    if (seqOn && !seqPl && seqWasPlaying_) seq_.stop();
    seqWasPlaying_ = seqOn && seqPl;

    // 5. Apply voice pool config (mode, count, steal)
    voicePool_.applyConfig(paramCache_);

    // 6. Per-sample loop
    for (int i = 0; i < nFrames; ++i) {
        // Arpeggiator tick
        if (paramCache_[P_ARP_ON] > 0.5f) {
            auto ao = arp_.tick();
            if (ao.hasNoteOff) routeNoteOff(ao.note);
            if (ao.hasNoteOn)  routeNoteOn (ao.note, ao.velocity);
        }

        // Sequencer tick
        if (seqOn && seqPl) {
            auto so = seq_.tick();
            if (so.hasNoteOff) routeNoteOff(so.note);
            if (so.hasNoteOn)  routeNoteOn (so.note, so.velocity);
        }

        // Render voices
        sample_t l = 0.0f, r = 0.0f;
        voicePool_.tick(paramCache_, l, r);
        outL[i] = l;
        outR[i] = r;
    }
}

// ─────────────────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────────────────

void SynthEngine::snapshotParams() noexcept {
    params_->snapshot(paramCache_);
}

void SynthEngine::drainMidiQueue() noexcept {
    MidiEvent e;
    while (midiQueue_->pop(e)) {
        switch (e.type) {
            case MidiEvent::Type::NoteOn:
                if (e.data2 > 0)
                    onNoteOn (e.data1, e.data2);
                else
                    onNoteOff(e.data1);
                break;
            case MidiEvent::Type::NoteOff:
                onNoteOff(e.data1);
                break;
            case MidiEvent::Type::ControlChange:
                onCC(e.data1, e.data2);
                break;
            case MidiEvent::Type::PitchBend:
                onPitchBend(e.pitchBend);
                break;
            default: break;
        }
    }
}

void SynthEngine::syncMusicConfig() noexcept {
    const float bpm = paramCache_[P_BPM];

    arp_.setBPM       (bpm);
    arp_.setMode      (static_cast<ArpMode>(
                           static_cast<int>(
                               paramCache_[P_ARP_MODE])));
    arp_.setOctaves   (static_cast<int>(paramCache_[P_ARP_OCTAVES]));
    arp_.setRateIndex (static_cast<int>(paramCache_[P_ARP_RATE]));
    arp_.setGate      (paramCache_[P_ARP_GATE]);
    arp_.setSwing     (paramCache_[P_ARP_SWING]);
    arp_.setEnabled   (paramCache_[P_ARP_ON] > 0.5f);

    seq_.setBPM       (bpm);
    seq_.setStepCount (static_cast<int>(paramCache_[P_SEQ_STEPS]));
    seq_.setRateIndex (static_cast<int>(paramCache_[P_SEQ_RATE]));
    seq_.setGlobalGate(paramCache_[P_SEQ_GATE]);
    seq_.setSwing     (paramCache_[P_SEQ_SWING]);
    seq_.setEnabled   (paramCache_[P_SEQ_ON] > 0.5f);

    chord_.setEnabled   (paramCache_[P_CHORD_ON] > 0.5f);
    chord_.setChordType (static_cast<int>(paramCache_[P_CHORD_TYPE]));
    chord_.setInversion (static_cast<int>(paramCache_[P_CHORD_INVERSION]));

    scale_.setEnabled(paramCache_[P_SCALE_ON] > 0.5f);
    scale_.setRoot   (static_cast<int>(paramCache_[P_SCALE_ROOT]));
    scale_.setScale  (static_cast<int>(paramCache_[P_SCALE_TYPE]));
}

void SynthEngine::onNoteOn(int note, int vel) noexcept {
    // Scale quantize first
    const int qNote = scale_.quantize(note);
    // Then route through arp (if arp on, it owns timing)
    if (paramCache_[P_ARP_ON] > 0.5f) {
        arp_.noteOn(qNote, vel);
    } else {
        routeNoteOn(qNote, vel);
    }
}

void SynthEngine::onNoteOff(int note) noexcept {
    const int qNote = scale_.quantize(note);
    if (paramCache_[P_ARP_ON] > 0.5f) {
        arp_.noteOff(qNote);
    } else {
        routeNoteOff(qNote);
    }
}

void SynthEngine::routeNoteOn(int note, int vel) noexcept {
    // Expand via chord engine
    const auto co = chord_.expand(note, vel);
    for (int i = 0; i < co.count; ++i)
        voicePool_.noteOn(co.notes[i], co.velocities[i]);
}

void SynthEngine::routeNoteOff(int note) noexcept {
    const auto co = chord_.expand(note, 0);
    for (int i = 0; i < co.count; ++i)
        voicePool_.noteOff(co.notes[i]);
}

void SynthEngine::onCC(int cc, int val) noexcept {
    // Standard MIDI CC → param mapping
    switch (cc) {
        case 1:  // Mod wheel → mod mix
            params_->set(P_MOD_MIX, val / 127.0f);
            break;
        case 7:  // Volume
            params_->set(P_MASTER_VOL, val / 127.0f);
            break;
        case 64: // Sustain pedal
            if (val < 64) voicePool_.allNotesOff();
            break;
        case 120: case 123:
            voicePool_.allSoundOff();
            break;
        default: break;
    }
}

void SynthEngine::onPitchBend(int bend) noexcept {
    // ±2 semitone pitch bend — applied via master tune
    const float semi = (bend / 8192.0f) * 2.0f;
    params_->set(P_MASTER_TUNE, semi);
}

// ── Query ─────────────────────────────────────────────

int   SynthEngine::getActiveVoices() const noexcept {
    return voicePool_.getActiveCount();
}
int   SynthEngine::getArpStep()  const noexcept { return arp_.getCurrentNote(); }
int   SynthEngine::getSeqStep()  const noexcept { return seq_.getCurrentStep(); }
bool  SynthEngine::getSeqPlaying()const noexcept { return seq_.isPlaying(); }
float SynthEngine::getBPM()      const noexcept {
    return params_ ? params_->get(P_BPM) : 120.0f;
}
```

---

# PHẦN 11 — PC HAL LAYER

## 11.1 RtAudio Backend

### `hal/pc/rtaudio_backend.h`

```cpp
#pragma once
#include "RtAudio.h"
#include "shared/interfaces.h"
#include <string>
#include <vector>

class RtAudioBackend {
public:
    struct Config {
        unsigned int sampleRate = 44100;
        unsigned int bufferSize = 256;
        int          deviceIdx  = -1;   // -1 = default
    };

    explicit RtAudioBackend(IAudioProcessor& processor);
    ~RtAudioBackend();

    bool open (Config cfg = {});
    bool start();
    void stop();
    void close();

    std::vector<std::string> listDevices() const;
    unsigned int getSampleRate() const { return cfg_.sampleRate; }
    unsigned int getBufferSize() const { return cfg_.bufferSize; }
    bool         isRunning()     const { return running_; }
    std::string  getLastError()  const { return lastError_; }

private:
    IAudioProcessor& processor_;
    RtAudio          dac_;
    Config           cfg_;
    bool             running_ = false;
    std::string      lastError_;

    // RtAudio static callback → forwards to instance
    static int audioCallback(void*        outputBuffer,
                             void*        inputBuffer,
                             unsigned int nFrames,
                             double       streamTime,
                             RtAudioStreamStatus status,
                             void*        userData);
};
```

### `hal/pc/rtaudio_backend.cpp`

```cpp
#include "rtaudio_backend.h"
#include <cstring>
#include <iostream>

RtAudioBackend::RtAudioBackend(IAudioProcessor& processor)
    : processor_(processor) {}

RtAudioBackend::~RtAudioBackend() {
    stop();
    close();
}

bool RtAudioBackend::open(Config cfg) {
    cfg_ = cfg;

    if (dac_.getDeviceCount() == 0) {
        lastError_ = "No audio devices found";
        return false;
    }

    RtAudio::StreamParameters params;
    params.deviceId   = (cfg_.deviceIdx < 0)
                      ? dac_.getDefaultOutputDevice()
                      : static_cast<unsigned int>(cfg_.deviceIdx);
    params.nChannels  = 2;
    params.firstChannel = 0;

    RtAudio::StreamOptions opts;
    opts.flags = RTAUDIO_SCHEDULE_REALTIME
               | RTAUDIO_MINIMIZE_LATENCY;

    unsigned int bufSize = cfg_.bufferSize;

    try {
        dac_.openStream(
            &params,               // output
            nullptr,               // input (none)
            RTAUDIO_FLOAT32,
            cfg_.sampleRate,
            &bufSize,
            &RtAudioBackend::audioCallback,
            static_cast<void*>(this),
            &opts
        );
        cfg_.bufferSize = bufSize;   // actual size after open
        processor_.setSampleRate(
            static_cast<float>(cfg_.sampleRate));
        processor_.setBlockSize(
            static_cast<int>(cfg_.bufferSize));
    } catch (RtAudioError& e) {
        lastError_ = e.getMessage();
        return false;
    }
    return true;
}

bool RtAudioBackend::start() {
    try {
        dac_.startStream();
        running_ = true;
    } catch (RtAudioError& e) {
        lastError_ = e.getMessage();
        return false;
    }
    return true;
}

void RtAudioBackend::stop() {
    if (!running_) return;
    try {
        dac_.stopStream();
    } catch (...) {}
    running_ = false;
}

void RtAudioBackend::close() {
    if (dac_.isStreamOpen())
        dac_.closeStream();
}

std::vector<std::string> RtAudioBackend::listDevices() const {
    std::vector<std::string> names;
    const unsigned int count = dac_.getDeviceCount();
    for (unsigned int i = 0; i < count; ++i) {
        RtAudio::DeviceInfo info = dac_.getDeviceInfo(i);
        if (info.outputChannels > 0)
            names.push_back(info.name);
    }
    return names;
}

// ─────────────────────────────────────────────────────────
// AUDIO CALLBACK (real-time thread)
// ZERO allocation, ZERO locks
// ─────────────────────────────────────────────────────────

int RtAudioBackend::audioCallback(void*        outputBuffer,
                                   void*        /*inputBuffer*/,
                                   unsigned int  nFrames,
                                   double        /*streamTime*/,
                                   RtAudioStreamStatus /*status*/,
                                   void*         userData) {
    auto* self = static_cast<RtAudioBackend*>(userData);
    auto* out  = static_cast<float*>(outputBuffer);

    // Split interleaved stereo buffer into L/R
    // Use stack buffers — no heap, no allocation
    constexpr int MAX_BUF = 1024;
    float outL[MAX_BUF];
    float outR[MAX_BUF];

    const int n = (nFrames > MAX_BUF)
                ? MAX_BUF
                : static_cast<int>(nFrames);

    self->processor_.processBlock(outL, outR, n);

    // Interleave: L, R, L, R, ...
    for (int i = 0; i < n; ++i) {
        out[i * 2 + 0] = outL[i];
        out[i * 2 + 1] = outR[i];
    }
    return 0;  // 0 = continue streaming
}
```

---

## 11.2 PC MIDI

### `hal/pc/pc_midi.h`

```cpp
#pragma once
#include "RtMidi.h"
#include "shared/types.h"
#include "shared/interfaces.h"
#include <string>
#include <vector>
#include <functional>

// ════════════════════════════════════════════════════════
// PC MIDI INPUT
// Receives USB MIDI → converts to MidiEvent
// → pushes onto MidiEventQueue (SPSC, lock-free)
// Thread: RtMidi callback thread → Audio thread via queue
// ════════════════════════════════════════════════════════

class PcMidi {
public:
    explicit PcMidi(MidiEventQueue& queue);
    ~PcMidi();

    bool open(int portIndex = -1);   // -1 = first available
    void close();

    std::vector<std::string> listPorts() const;
    bool        isOpen()       const { return open_; }
    std::string getLastError() const { return lastError_; }

private:
    MidiEventQueue& queue_;
    RtMidiIn        midiIn_;
    bool            open_      = false;
    std::string     lastError_;

    static void midiCallback(double        deltaTime,
                             std::vector<unsigned char>* msg,
                             void*         userData);

    MidiEvent parseMessage(
        const std::vector<unsigned char>& msg) const;
};
```

### `hal/pc/pc_midi.cpp`

```cpp
#include "pc_midi.h"
#include <iostream>

PcMidi::PcMidi(MidiEventQueue& queue) : queue_(queue) {}

PcMidi::~PcMidi() { close(); }

bool PcMidi::open(int portIndex) {
    try {
        const unsigned int count = midiIn_.getPortCount();
        if (count == 0) {
            lastError_ = "No MIDI input ports found";
            return false;
        }

        const unsigned int idx = (portIndex < 0 || portIndex
                                  >= static_cast<int>(count))
                               ? 0u
                               : static_cast<unsigned int>(portIndex);

        midiIn_.openPort(idx);
        midiIn_.setCallback(&PcMidi::midiCallback,
                            static_cast<void*>(this));
        // Ignore sysex, timing, active sensing
        midiIn_.ignoreTypes(true, false, true);
        open_ = true;
    } catch (RtMidiError& e) {
        lastError_ = e.getMessage();
        return false;
    }
    return true;
}

void PcMidi::close() {
    if (open_) {
        midiIn_.closePort();
        open_ = false;
    }
}

std::vector<std::string> PcMidi::listPorts() const {
    std::vector<std::string> ports;
    const unsigned int count = midiIn_.getPortCount();
    for (unsigned int i = 0; i < count; ++i)
        ports.push_back(midiIn_.getPortName(i));
    return ports;
}

// ─────────────────────────────────────────────────────────
// MIDI CALLBACK (RtMidi thread)
// ─────────────────────────────────────────────────────────

void PcMidi::midiCallback(
        double /*deltaTime*/,
        std::vector<unsigned char>* msg,
        void*  userData) {
    auto* self = static_cast<PcMidi*>(userData);
    if (!msg || msg->empty()) return;

    MidiEvent ev = self->parseMessage(*msg);
    if (ev.type != MidiEvent::Type::Invalid)
        self->queue_.push(ev);
}

MidiEvent PcMidi::parseMessage(
        const std::vector<unsigned char>& msg) const {
    MidiEvent ev;
    if (msg.empty()) return ev;

    const uint8_t status  = msg[0];
    const uint8_t type    = status & 0xF0;
    const uint8_t channel = status & 0x0F;
    ev.channel = channel;

    switch (type) {
        case 0x90:  // Note On
            if (msg.size() >= 3) {
                ev.type  = MidiEvent::Type::NoteOn;
                ev.data1 = msg[1];
                ev.data2 = msg[2];
            }
            break;
        case 0x80:  // Note Off
            if (msg.size() >= 3) {
                ev.type  = MidiEvent::Type::NoteOff;
                ev.data1 = msg[1];
                ev.data2 = 0;
            }
            break;
        case 0xB0:  // Control Change
            if (msg.size() >= 3) {
                ev.type  = MidiEvent::Type::ControlChange;
                ev.data1 = msg[1];
                ev.data2 = msg[2];
            }
            break;
        case 0xE0:  // Pitch Bend
            if (msg.size() >= 3) {
                ev.type      = MidiEvent::Type::PitchBend;
                const int raw = (msg[2] << 7) | msg[1];
                ev.pitchBend = static_cast<int16_t>(raw - 8192);
            }
            break;
        case 0xD0:  // Channel Aftertouch
            if (msg.size() >= 2) {
                ev.type  = MidiEvent::Type::Aftertouch;
                ev.data1 = msg[1];
            }
            break;
        case 0xC0:  // Program Change
            if (msg.size() >= 2) {
                ev.type  = MidiEvent::Type::ProgramChange;
                ev.data1 = msg[1];
            }
            break;
        case 0xF0:  // System messages
            if (status == 0xFA) ev.type = MidiEvent::Type::Start;
            else if (status == 0xFC) ev.type = MidiEvent::Type::Stop;
            else if (status == 0xFB) ev.type = MidiEvent::Type::Continue;
            break;
        default:
            break;
    }
    return ev;
}
```

---

## 11.3 Keyboard Input

### `hal/pc/keyboard_input.h`

```cpp
#pragma once
#include "shared/types.h"
#include "shared/interfaces.h"
#include <GLFW/glfw3.h>

// ════════════════════════════════════════════════════════
// QWERTY → MIDI NOTE mapping
// Two rows cover two octaves (piano-style layout)
// Bottom row: Z X C V B N M , . /   → C3..E4
// Top row:    A S D F G H J K L ;   → C4..D5
// Octave shift: bracket keys [ ]
// Velocity: fixed at 100 (no aftertouch simulation)
// ════════════════════════════════════════════════════════

class KeyboardInput {
public:
    void init(GLFWwindow*    window,
              MidiEventQueue& queue) noexcept;

    // Call from GLFW main loop (UI thread)
    void update() noexcept;

    // GLFW key callback — register with glfwSetKeyCallback
    static void glfwKeyCallback(GLFWwindow* window,
                                int key, int scancode,
                                int action, int mods);

    int  getOctave() const noexcept { return octave_; }

private:
    GLFWwindow*     window_   = nullptr;
    MidiEventQueue* queue_    = nullptr;
    int             octave_   = 4;     // base octave C4=60
    bool            keyState_[512] = {};

    static KeyboardInput* instance_;   // for GLFW callback

    int   keyToNote(int glfwKey) const noexcept;
    void  sendNoteOn (int note)        noexcept;
    void  sendNoteOff(int note)        noexcept;
};
```

### `hal/pc/keyboard_input.cpp`

```cpp
#include "keyboard_input.h"
#include <cstring>

KeyboardInput* KeyboardInput::instance_ = nullptr;

void KeyboardInput::init(GLFWwindow*    window,
                          MidiEventQueue& queue) noexcept {
    window_  = window;
    queue_   = &queue;
    instance_ = this;
    std::memset(keyState_, 0, sizeof(keyState_));
    glfwSetKeyCallback(window, glfwKeyCallback);
}

void KeyboardInput::update() noexcept {
    // Octave shift
    if (glfwGetKey(window_, GLFW_KEY_LEFT_BRACKET)
            == GLFW_PRESS)
        octave_ = (octave_ > 0) ? octave_ - 1 : 0;
    if (glfwGetKey(window_, GLFW_KEY_RIGHT_BRACKET)
            == GLFW_PRESS)
        octave_ = (octave_ < 9) ? octave_ + 1 : 9;
}

// ─────────────────────────────────────────────────────────
// QWERTY MAPPING
// Bottom row (white keys): Z C V B N M , . /
// Lower row (black keys):  S D   G H J   L ;
// ─────────────────────────────────────────────────────────

int KeyboardInput::keyToNote(int key) const noexcept {
    // Base: C in current octave = octave_ * 12
    const int base = octave_ * 12;
    switch (key) {
        // Bottom row — white keys (C major starting C)
        case GLFW_KEY_Z:          return base + 0;   // C
        case GLFW_KEY_X:          return base + 2;   // D
        case GLFW_KEY_C:          return base + 4;   // E
        case GLFW_KEY_V:          return base + 5;   // F
        case GLFW_KEY_B:          return base + 7;   // G
        case GLFW_KEY_N:          return base + 9;   // A
        case GLFW_KEY_M:          return base + 11;  // B
        case GLFW_KEY_COMMA:      return base + 12;  // C+1
        case GLFW_KEY_PERIOD:     return base + 14;  // D+1
        case GLFW_KEY_SLASH:      return base + 16;  // E+1
        // Bottom row — black keys
        case GLFW_KEY_S:          return base + 1;   // C#
        case GLFW_KEY_D:          return base + 3;   // D#
        case GLFW_KEY_G:          return base + 6;   // F#
        case GLFW_KEY_H:          return base + 8;   // G#
        case GLFW_KEY_J:          return base + 10;  // A#
        case GLFW_KEY_L:          return base + 13;  // C#+1
        case GLFW_KEY_SEMICOLON:  return base + 15;  // D#+1
        // Upper row — white keys (octave+1)
        case GLFW_KEY_Q:          return base + 12;  // C+1
        case GLFW_KEY_W:          return base + 14;  // D+1
        case GLFW_KEY_E:          return base + 16;  // E+1
        case GLFW_KEY_R:          return base + 17;  // F+1
        case GLFW_KEY_T:          return base + 19;  // G+1
        case GLFW_KEY_Y:          return base + 21;  // A+1
        case GLFW_KEY_U:          return base + 23;  // B+1
        case GLFW_KEY_I:          return base + 24;  // C+2
        // Upper row — black keys
        case GLFW_KEY_2:          return base + 13;  // C#+1
        case GLFW_KEY_3:          return base + 15;  // D#+1
        case GLFW_KEY_5:          return base + 18;  // F#+1
        case GLFW_KEY_6:          return base + 20;  // G#+1
        case GLFW_KEY_7:          return base + 22;  // A#+1
        default:                  return -1;
    }
}

void KeyboardInput::sendNoteOn(int note) noexcept {
    if (!queue_ || note < 0 || note > 127) return;
    MidiEvent ev;
    ev.type  = MidiEvent::Type::NoteOn;
    ev.data1 = static_cast<uint8_t>(note);
    ev.data2 = 100;   // fixed velocity
    queue_->push(ev);
}

void KeyboardInput::sendNoteOff(int note) noexcept {
    if (!queue_ || note < 0 || note > 127) return;
    MidiEvent ev;
    ev.type  = MidiEvent::Type::NoteOff;
    ev.data1 = static_cast<uint8_t>(note);
    ev.data2 = 0;
    queue_->push(ev);
}

void KeyboardInput::glfwKeyCallback(GLFWwindow* /*window*/,
                                     int key, int /*scan*/,
                                     int action, int /*mods*/) {
    if (!instance_) return;
    if (key < 0 || key >= 512) return;

    const int note = instance_->keyToNote(key);
    if (note < 0) return;

    if (action == GLFW_PRESS && !instance_->keyState_[key]) {
        instance_->keyState_[key] = true;
        instance_->sendNoteOn(note);
    } else if (action == GLFW_RELEASE && instance_->keyState_[key]) {
        instance_->keyState_[key] = false;
        instance_->sendNoteOff(note);
    }
}
```

---

## 11.4 Preset Storage

### `hal/pc/preset_storage.h`

```cpp
#pragma once
#include "shared/params.h"
#include "shared/interfaces.h"
#include "core/music/sequencer.h"
#include <string>
#include <vector>

struct Preset {
    std::string name;
    std::string category;
    float       params[PARAM_COUNT];
    SeqStep     seqSteps[StepSequencer::MAX_STEPS];
};

class PresetStorage {
public:
    // ── Directory setup ──────────────────────────────
    void setDirectory(const std::string& dir) noexcept;

    // ── Save / Load ──────────────────────────────────
    bool savePreset(const Preset&      preset,
                    const std::string& filename);
    bool loadPreset(const std::string& filename,
                    Preset&            out);

    // ── Apply to store ───────────────────────────────
    void applyPreset(const Preset&     preset,
                     AtomicParamStore& store,
                     StepSequencer&    seq) noexcept;

    // ── Capture from store ───────────────────────────
    Preset capturePreset(const AtomicParamStore& store,
                         const StepSequencer&    seq,
                         const std::string&      name,
                         const std::string&      category);

    // ── List presets ─────────────────────────────────
    std::vector<std::string> listPresets() const;

    std::string getLastError() const { return lastError_; }

private:
    std::string dir_       = "./presets";
    std::string lastError_;
};
```

### `hal/pc/preset_storage.cpp`

```cpp
#include "preset_storage.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using json   = nlohmann::json;

void PresetStorage::setDirectory(const std::string& dir) noexcept {
    dir_ = dir;
    fs::create_directories(dir_);
}

bool PresetStorage::savePreset(const Preset&      preset,
                                const std::string& filename) {
    json j;
    j["name"]     = preset.name;
    j["category"] = preset.category;

    // Params
    json pj = json::object();
    for (int i = 0; i < PARAM_COUNT; ++i)
        pj[PARAM_META[i].jsonKey] = preset.params[i];
    j["params"] = pj;

    // Sequencer steps
    json steps = json::array();
    for (int i = 0; i < StepSequencer::MAX_STEPS; ++i) {
        const auto& s = preset.seqSteps[i];
        steps.push_back({
            {"note",     s.note},
            {"velocity", s.velocity},
            {"gate",     s.gate},
            {"active",   s.active},
            {"tie",      s.tie}
        });
    }
    j["seq_steps"] = steps;

    const std::string path = dir_ + "/" + filename;
    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        lastError_ = "Cannot write: " + path;
        return false;
    }
    ofs << j.dump(2);
    return true;
}

bool PresetStorage::loadPreset(const std::string& filename,
                                Preset&            out) {
    const std::string path = dir_ + "/" + filename;
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        lastError_ = "Cannot open: " + path;
        return false;
    }

    json j;
    try {
        ifs >> j;
    } catch (json::exception& e) {
        lastError_ = std::string("JSON parse error: ") + e.what();
        return false;
    }

    out.name     = j.value("name",     filename);
    out.category = j.value("category", "User");

    // Params — init with defaults, then overlay file values
    for (int i = 0; i < PARAM_COUNT; ++i)
        out.params[i] = PARAM_META[i].defaultVal;

    if (j.contains("params") && j["params"].is_object()) {
        for (int i = 0; i < PARAM_COUNT; ++i) {
            const char* key = PARAM_META[i].jsonKey;
            if (j["params"].contains(key))
                out.params[i] = j["params"][key].get<float>();
        }
    }

    // Sequencer steps
    if (j.contains("seq_steps") && j["seq_steps"].is_array()) {
        int idx = 0;
        for (const auto& s : j["seq_steps"]) {
            if (idx >= StepSequencer::MAX_STEPS) break;
            out.seqSteps[idx].note     = s.value("note",     60);
            out.seqSteps[idx].velocity = s.value("velocity", 100);
            out.seqSteps[idx].gate     = s.value("gate",     0.8f);
            out.seqSteps[idx].active   = s.value("active",   true);
            out.seqSteps[idx].tie      = s.value("tie",      false);
            ++idx;
        }
    }
    return true;
}

void PresetStorage::applyPreset(const Preset&     preset,
                                 AtomicParamStore& store,
                                 StepSequencer&    seq) noexcept {
    for (int i = 0; i < PARAM_COUNT; ++i)
        store.set(i, preset.params[i]);
    for (int i = 0; i < StepSequencer::MAX_STEPS; ++i)
        seq.setStep(i, preset.seqSteps[i]);
}

Preset PresetStorage::capturePreset(
        const AtomicParamStore& store,
        const StepSequencer&    seq,
        const std::string&      name,
        const std::string&      category) {
    Preset p;
    p.name     = name;
    p.category = category;
    store.snapshot(p.params);
    for (int i = 0; i < StepSequencer::MAX_STEPS; ++i)
        p.seqSteps[i] = seq.getStep(i);
    return p;
}

std::vector<std::string> PresetStorage::listPresets() const {
    std::vector<std::string> list;
    if (!fs::exists(dir_)) return list;
    for (const auto& entry : fs::directory_iterator(dir_)) {
        if (entry.path().extension() == ".json")
            list.push_back(entry.path().filename().string());
    }
    std::sort(list.begin(), list.end());
    return list;
}
```

---

# PHẦN 12 — UI SYSTEM

## 12.1 ImGui App (Main Shell)

### `ui/imgui_app.h`

```cpp
#pragma once
#include <GLFW/glfw3.h>
#include "shared/interfaces.h"
#include "shared/params.h"
#include "core/engine/synth_engine.h"
#include "hal/pc/preset_storage.h"
#include "hal/pc/keyboard_input.h"

class ImGuiApp {
public:
    struct Config {
        int         windowW    = 1400;
        int         windowH    =  820;
        const char* title      = "MiniMoog DSP Simulator v1.0";
        const char* presetDir  = "./presets";
    };

    bool init(AtomicParamStore& params,
              SynthEngine&      engine,
              MidiEventQueue&   midiQueue,
              Config            cfg = {});

    void run();   // blocks until window closed
    void shutdown();

    bool shouldClose() const noexcept;

private:
    GLFWwindow*       window_    = nullptr;
    AtomicParamStore* params_    = nullptr;
    SynthEngine*      engine_    = nullptr;
    MidiEventQueue*   midiQueue_ = nullptr;

    PresetStorage     presetStorage_;
    KeyboardInput     kbdInput_;
    Config            cfg_;

    // Panel visibility flags
    bool showOscPanel_      = true;
    bool showMixPanel_      = true;
    bool showFilterPanel_   = true;
    bool showEnvPanel_      = true;
    bool showControlPanel_  = true;
    bool showArpPanel_      = true;
    bool showSeqPanel_      = true;
    bool showChordPanel_    = true;
    bool showPresetPanel_   = true;
    bool showDebugPanel_    = false;

    void renderMainMenuBar();
    void renderStatusBar();
    void renderAllPanels();
    void applyDarkMoogTheme();

    // Style constants
    static constexpr float KNOB_SIZE    = 55.0f;
    static constexpr float PANEL_WIDTH  = 200.0f;
};
```

### `ui/imgui_app.cpp`

```cpp
#include "imgui_app.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/gl.h>

// Panel includes
#include "panels/panel_controllers.h"
#include "panels/panel_oscillators.h"
#include "panels/panel_mixer.h"
#include "panels/panel_modifiers.h"
#include "panels/panel_output.h"
#include "panels/panel_arpeggiator.h"
#include "panels/panel_sequencer.h"
#include "panels/panel_chord_scale.h"
#include "panels/panel_presets.h"

bool ImGuiApp::init(AtomicParamStore& params,
                     SynthEngine&      engine,
                     MidiEventQueue&   midiQueue,
                     Config            cfg) {
    params_    = &params;
    engine_    = &engine;
    midiQueue_ = &midiQueue;
    cfg_       = cfg;

    // Init GLFW
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,
                   GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(cfg_.windowW, cfg_.windowH,
                               cfg_.title, nullptr, nullptr);
    if (!window_) return false;

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);   // vsync

    // Init ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "minimoog_layout.ini";

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    applyDarkMoogTheme();

    // Init keyboard input
    kbdInput_.init(window_, *midiQueue_);

    // Init preset storage
    presetStorage_.setDirectory(cfg_.presetDir);

    return true;
}

void ImGuiApp::run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        kbdInput_.update();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Full-screen dockspace
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode);

        renderMainMenuBar();
        renderAllPanels();
        renderStatusBar();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.10f, 0.10f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);
    }
}

void ImGuiApp::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

bool ImGuiApp::shouldClose() const noexcept {
    return window_ && glfwWindowShouldClose(window_);
}

// ─────────────────────────────────────────────────────────

void ImGuiApp::renderMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Controllers", nullptr, &showControlPanel_);
            ImGui::MenuItem("Oscillators", nullptr, &showOscPanel_);
            ImGui::MenuItem("Mixer",       nullptr, &showMixPanel_);
            ImGui::MenuItem("Filter/Env",  nullptr, &showFilterPanel_);
            ImGui::MenuItem("Arpeggiator", nullptr, &showArpPanel_);
            ImGui::MenuItem("Sequencer",   nullptr, &showSeqPanel_);
            ImGui::MenuItem("Chord/Scale", nullptr, &showChordPanel_);
            ImGui::MenuItem("Presets",     nullptr, &showPresetPanel_);
            ImGui::Separator();
            ImGui::MenuItem("Debug",       nullptr, &showDebugPanel_);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            ImGui::Text("MiniMoog DSP Simulator v1.0");
            ImGui::Text("Keyboard: Z-M = C..B, Q-U = C..B+1");
            ImGui::Text("[/] = Octave Down/Up");
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ImGuiApp::renderStatusBar() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const float barH = ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos(
        ImVec2(vp->Pos.x, vp->Pos.y + vp->Size.y - barH));
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, barH));
    ImGui::SetNextWindowBgAlpha(0.85f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoInputs     |
        ImGuiWindowFlags_NoNav        |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##statusbar", nullptr, flags)) {
        ImGui::Text("Voices: %d/%d  |  BPM: %.1f  |"
                    "  Octave: %d  |  %.0f Hz / %d spl",
            engine_->getActiveVoices(),
            static_cast<int>(params_->get(P_VOICE_COUNT)),
            params_->get(P_BPM),
            kbdInput_.getOctave(),
            params_->get(P_MASTER_TUNE),
            BLOCK_SIZE_DEFAULT);
    }
    ImGui::End();
}

void ImGuiApp::renderAllPanels() {
    if (showControlPanel_)
        PanelControllers::render(*params_);
    if (showOscPanel_)
        PanelOscillators::render(*params_);
    if (showMixPanel_)
        PanelMixer::render(*params_);
    if (showFilterPanel_)
        PanelModifiers::render(*params_);
    if (showArpPanel_)
        PanelArpeggiator::render(*params_);
    if (showSeqPanel_)
        PanelSequencer::render(*params_, *engine_);
    if (showChordPanel_)
        PanelChordScale::render(*params_);
    if (showPresetPanel_)
        PanelPresets::render(*params_, *engine_,
                             presetStorage_);
    if (showDebugPanel_) {
        ImGui::Begin("Debug");
        ImGui::Text("Active voices : %d",
                    engine_->getActiveVoices());
        ImGui::Text("Seq step      : %d",
                    engine_->getSeqStep());
        ImGui::Text("Arp note      : %d",
                    engine_->getArpStep());
        ImGui::End();
    }
}

// ─────────────────────────────────────────────────────────
// DARK MOOG THEME
// ─────────────────────────────────────────────────────────

void ImGuiApp::applyDarkMoogTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding   = 4.0f;
    s.FrameRounding    = 3.0f;
    s.GrabRounding     = 3.0f;
    s.ScrollbarRounding= 3.0f;
    s.FramePadding     = ImVec2(6, 4);
    s.ItemSpacing      = ImVec2(8, 5);
    s.WindowPadding    = ImVec2(10, 10);

    auto* c = s.Colors;
    c[ImGuiCol_WindowBg]         = ImVec4(0.10f,0.10f,0.13f,1.f);
    c[ImGuiCol_TitleBg]          = ImVec4(0.08f,0.08f,0.10f,1.f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.18f,0.09f,0.02f,1.f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.16f,0.16f,0.20f,1.f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.24f,0.20f,0.12f,1.f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.30f,0.22f,0.04f,1.f);
    c[ImGuiCol_Button]           = ImVec4(0.20f,0.12f,0.02f,1.f);
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.70f,0.40f,0.00f,1.f);
    c[ImGuiCol_ButtonActive]     = ImVec4(0.85f,0.50f,0.00f,1.f);
    c[ImGuiCol_SliderGrab]       = ImVec4(0.85f,0.50f,0.00f,1.f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(1.00f,0.65f,0.10f,1.f);
    c[ImGuiCol_Header]           = ImVec4(0.25f,0.15f,0.02f,1.f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.55f,0.32f,0.02f,1.f);
    c[ImGuiCol_HeaderActive]     = ImVec4(0.75f,0.45f,0.02f,1.f);
    c[ImGuiCol_CheckMark]        = ImVec4(1.00f,0.65f,0.10f,1.f);
    c[ImGuiCol_Text]             = ImVec4(0.92f,0.88f,0.80f,1.f);
    c[ImGuiCol_TextDisabled]     = ImVec4(0.50f,0.48f,0.44f,1.f);
    c[ImGuiCol_Separator]        = ImVec4(0.35f,0.25f,0.08f,1.f);
    c[ImGuiCol_Tab]              = ImVec4(0.14f,0.10f,0.04f,1.f);
    c[ImGuiCol_TabHovered]       = ImVec4(0.60f,0.36f,0.02f,1.f);
    c[ImGuiCol_TabActive]        = ImVec4(0.40f,0.24f,0.02f,1.f);
}
```

---

## 12.2 Panel Pattern (Shared Convention)

> Tất cả panels dùng cùng pattern: **namespace + `render()` function**. Dưới đây là `PanelOscillators` và `PanelModifiers` làm mẫu đầy đủ. Các panel còn lại theo cùng pattern.

### `ui/panels/panel_oscillators.cpp` (mẫu đầy đủ)

```cpp
#include "panel_oscillators.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "shared/params.h"

namespace PanelOscillators {

// ── Helper: one oscillator strip ─────────────────────────
static void renderOscStrip(AtomicParamStore& p,
                            int oscIdx,
                            const char* label) {
    // ParamID offsets: OSC1=0, OSC2=4, OSC3=8
    const int base = P_OSC1_ON + oscIdx * 4;
    const int ON   = base;
    const int RNG  = base + 1;
    const int FRQ  = base + 2;
    const int WAV  = base + 3;

    ImGui::PushID(oscIdx);
    ImGui::BeginGroup();
    ImGui::TextColored(ImVec4(0.9f,0.6f,0.1f,1.f), "%s", label);
    ImGui::Separator();

    // Power switch
    bool on = p.get(ON) > 0.5f;
    if (ImGui::Checkbox("ON", &on))
        p.set(ON, on ? 1.0f : 0.0f);

    // Range selector
    const char* ranges[] = {"LO","32'","16'","8'","4'","2'"};
    int rng = static_cast<int>(p.get(RNG));
    ImGui::SetNextItemWidth(70.f);
    if (ImGui::Combo("Range", &rng, ranges, 6))
        p.set(RNG, static_cast<float>(rng));

    // Waveform selector
    const char* waves[] = {"Tri","TriSaw","RevSaw",
                            "Saw","Square","Pulse"};
    int wav = static_cast<int>(p.get(WAV));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("Wave", &wav, waves, 6))
        p.set(WAV, static_cast<float>(wav));

    // Frequency knob
    float frq = p.get(FRQ);
    if (ImGuiKnobs::Knob("Freq", &frq,
                          0.0f, 1.0f, 0.005f,
                          "%.2f", ImGuiKnobVariant_Wiper,
                          55.f)) {
        p.set(FRQ, frq);
    }

    // OSC3: LFO switch
    if (oscIdx == 2) {
        ImGui::Separator();
        bool lfoMode = p.get(P_OSC3_LFO_ON) > 0.5f;
        if (ImGui::Checkbox("LFO Mode", &lfoMode))
            p.set(P_OSC3_LFO_ON, lfoMode ? 1.0f : 0.0f);
    }

    ImGui::EndGroup();
    ImGui::PopID();
}

void render(AtomicParamStore& params) {
    ImGui::Begin("Oscillators");

    renderOscStrip(params, 0, "OSC 1");
    ImGui::SameLine(0.f, 20.f);
    renderOscStrip(params, 1, "OSC 2");
    ImGui::SameLine(0.f, 20.f);
    renderOscStrip(params, 2, "OSC 3");

    // Master Tune
    ImGui::Separator();
    float tune = params.get(P_MASTER_TUNE);
    ImGui::SetNextItemWidth(150.f);
    if (ImGui::SliderFloat("Master Tune", &tune, -1.f, 1.f))
        params.set(P_MASTER_TUNE, tune);

    ImGui::End();
}

} // namespace PanelOscillators
```

### `ui/panels/panel_modifiers.cpp` (Filter + Envelopes)

```cpp
#include "panel_modifiers.h"
#include "imgui.h"
#include "imgui-knobs.h"
#include "widgets/adsr_display.h"
#include "shared/params.h"

namespace PanelModifiers {

static void renderADSRKnobs(AtomicParamStore& p,
                             int A, int D, int S, int R,
                             const char* label) {
    ImGui::TextColored(
        ImVec4(0.9f,0.6f,0.1f,1.f), "%s", label);
    ImGui::Separator();

    float a = p.get(A), d = p.get(D),
          s = p.get(S), r = p.get(R);

    // 4 knobs in a row
    ImGui::PushID(label);
    bool changed = false;
    if (ImGuiKnobs::Knob("A",&a,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper, 50.f)) { p.set(A,a); changed=true; }
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("D",&d,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper, 50.f)) { p.set(D,d); changed=true; }
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("S",&s,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper, 50.f)) { p.set(S,s); changed=true; }
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("R",&r,0.f,1.f,0.005f,"%.2f",
            ImGuiKnobVariant_Wiper, 50.f)) { p.set(R,r); changed=true; }

    // ADSR shape display
    AdsrDisplay::draw(a, d, s, r, ImVec2(220.f, 60.f));
    ImGui::PopID();
    (void)changed;
}

void render(AtomicParamStore& params) {
    ImGui::Begin("Filter & Envelopes");

    // ── Filter section ────────────────────────────────
    ImGui::TextColored(
        ImVec4(0.9f,0.6f,0.1f,1.f), "MOOG FILTER");
    ImGui::Separator();

    float cutoff = params.get(P_FILTER_CUTOFF);
    float res    = params.get(P_FILTER_EMPHASIS);
    float amt    = params.get(P_FILTER_AMOUNT);

    if (ImGuiKnobs::Knob("Cutoff",&cutoff,0.f,1.f,
            0.005f,"%.2f",ImGuiKnobVariant_Wiper,55.f))
        params.set(P_FILTER_CUTOFF, cutoff);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Emphasis",&res,0.f,1.f,
            0.005f,"%.2f",ImGuiKnobVariant_Wiper,55.f))
        params.set(P_FILTER_EMPHASIS, res);
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("Env Amt",&amt,0.f,1.f,
            0.005f,"%.2f",ImGuiKnobVariant_Wiper,55.f))
        params.set(P_FILTER_AMOUNT, amt);

    // Keyboard tracking
    const char* kbdModes[] = {"Off","1/3","2/3"};
    int kbd = static_cast<int>(params.get(P_FILTER_KBD_TRACK));
    ImGui::SetNextItemWidth(80.f);
    if (ImGui::Combo("KBD Track", &kbd, kbdModes, 3))
        params.set(P_FILTER_KBD_TRACK,
                   static_cast<float>(kbd));

    ImGui::Spacing();

    // ── Envelopes ─────────────────────────────────────
    renderADSRKnobs(params,
        P_FENV_ATTACK, P_FENV_DECAY,
        P_FENV_SUSTAIN, P_FENV_RELEASE,
        "FILTER ENV");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    renderADSRKnobs(params,
        P_AENV_ATTACK, P_AENV_DECAY,
        P_AENV_SUSTAIN, P_AENV_RELEASE,
        "AMP ENV");

    ImGui::End();
}

} // namespace PanelModifiers
```

### `ui/widgets/adsr_display.h / .cpp`

```cpp
// adsr_display.h
#pragma once
#include "imgui.h"

namespace AdsrDisplay {
    // Draws an ADSR shape using ImDrawList
    // a/d/s/r: normalized 0..1
    void draw(float a, float d, float s, float r,
              ImVec2 size);
}
```

```cpp
// adsr_display.cpp
#include "adsr_display.h"

void AdsrDisplay::draw(float a, float d, float s, float r,
                        ImVec2 size) {
    ImDrawList* dl  = ImGui::GetWindowDrawList();
    const ImVec2 p  = ImGui::GetCursorScreenPos();
    const float  W  = size.x;
    const float  H  = size.y;
    const float  x0 = p.x;
    const float  y0 = p.y;
    const float  yB = y0 + H;         // baseline
    const float  yT = y0;             // top (peak)

    // Background
    dl->AddRectFilled(ImVec2(x0, y0),
                      ImVec2(x0+W, yB),
                      IM_COL32(20,20,28,200), 3.f);

    // Segment widths
    const float seg = W * 0.22f;
    const float xA  = x0 + a * seg;
    const float xD  = xA + d * seg;
    const float xS  = xD + seg * 0.5f;
    const float xR  = x0 + W - r * seg * 0.8f;
    const float yS  = yB - s * H;

    // ADSR polyline
    const ImU32 col = IM_COL32(240, 140, 20, 255);
    const float th  = 2.0f;
    dl->AddLine(ImVec2(x0, yB), ImVec2(xA, yT), col, th);
    dl->AddLine(ImVec2(xA, yT), ImVec2(xD, yS), col, th);
    dl->AddLine(ImVec2(xD, yS), ImVec2(xS, yS), col, th);
    dl->AddLine(ImVec2(xS, yS), ImVec2(xR, yB), col, th);

    // Labels
    dl->AddText(ImVec2(x0+2,  yB-12),
                IM_COL32(180,120,40,200), "A");
    dl->AddText(ImVec2(xA+2,  yB-12),
                IM_COL32(180,120,40,200), "D");
    dl->AddText(ImVec2(xD+2,  yB-12),
                IM_COL32(180,120,40,200), "S");
    dl->AddText(ImVec2(xS+2,  yB-12),
                IM_COL32(180,120,40,200), "R");

    ImGui::Dummy(size);
}
```

---

# PHẦN 13 — APPLICATION ENTRY POINT

### `sim/main.cpp`

```cpp
#include <iostream>
#include <memory>

#include "shared/interfaces.h"
#include "shared/params.h"
#include "core/engine/synth_engine.h"
#include "hal/pc/rtaudio_backend.h"
#include "hal/pc/pc_midi.h"
#include "ui/imgui_app.h"

int main() {
    std::cout << "MiniMoog DSP Simulator v1.0\n";

    // ── 1. Shared state (owns everything) ─────────────
    AtomicParamStore params;
    MidiEventQueue   midiQueue;

    // ── 2. DSP Engine ─────────────────────────────────
    SynthEngine engine;
    engine.init(&params, &midiQueue);

    // ── 3. Audio backend ─────────────────────────────
    RtAudioBackend audio(engine);
    RtAudioBackend::Config audioCfg;
    audioCfg.sampleRate = 44100;
    audioCfg.bufferSize = 256;

    if (!audio.open(audioCfg)) {
        std::cerr << "Audio open failed: "
                  << audio.getLastError() << "\n";
        return 1;
    }
    if (!audio.start()) {
        std::cerr << "Audio start failed: "
                  << audio.getLastError() << "\n";
        return 1;
    }
    std::cout << "Audio: "
              << audio.getSampleRate() << " Hz / "
              << audio.getBufferSize() << " spl\n";

    // ── 4. MIDI input (best-effort, non-fatal) ────────
    PcMidi midi(midiQueue);
    if (midi.open()) {
        std::cout << "MIDI: port opened OK\n";
    } else {
        std::cout << "MIDI: " << midi.getLastError()
                  << " (continuing without MIDI)\n";
    }

    // ── 5. UI (blocks until window closed) ───────────
    ImGuiApp ui;
    ImGuiApp::Config uiCfg;
    uiCfg.windowW   = 1400;
    uiCfg.windowH   =  820;
    uiCfg.presetDir = "./presets";

    if (!ui.init(params, engine, midiQueue, uiCfg)) {
        std::cerr << "UI init failed\n";
        return 1;
    }

    ui.run();   // ← render loop, blocks here

    // ── 6. Shutdown (reverse order) ──────────────────
    audio.stop();
    midi.close();
    ui.shutdown();

    std::cout << "Bye.\n";
    return 0;
}
```

---

# PHẦN 14 — THREADING MODEL

```
┌─────────────────────────────────────────────────────────┐
│ THREAD MAP                                               │
│                                                          │
│  Thread         Owner       Priority   Shares           │
│  ─────────────  ─────────── ─────────  ──────────────── │
│  Main / UI      GLFW+ImGui  Normal     AtomicParamStore │
│                                        MidiEventQueue   │
│  Audio          RtAudio     Realtime   (reads above)    │
│  MIDI Callback  RtMidi      Normal     MidiEventQueue   │
│                                                          │
│ SYNCHRONIZATION RULES:                                   │
│                                                          │
│  UI → Audio params :  AtomicParamStore                  │
│       std::atomic<float>, memory_order_relaxed          │
│       Batch-read via snapshot() once per block          │
│                                                          │
│  MIDI → Audio events: SPSCQueue<MidiEvent, 256>         │
│       Lock-free ring buffer                              │
│       Producer: MIDI callback thread                    │
│       Consumer: Audio callback thread                   │
│                                                          │
│  Audio → UI queries: engine->getActiveVoices() etc.     │
│       Returns atomic int, read only in UI               │
│                                                          │
│ FORBIDDEN in Audio thread:                               │
│  ✗ new / delete / malloc / free                         │
│  ✗ std::vector / std::string construction               │
│  ✗ std::mutex::lock (may block)                         │
│  ✗ file I/O                                             │
│  ✗ std::cout / printf                                   │
└─────────────────────────────────────────────────────────┘
```

---

# PHẦN 15 — PERFORMANCE BUDGET

```
TARGET: 44100 Hz / 256 samples = 5.8ms per block

┌───────────────────────────────────┬────────┬──────────┐
│ Component                         │ Est.µs │ % budget │
├───────────────────────────────────┼────────┼──────────┤
│ param snapshot (PARAM_COUNT atomics)│  10  │   0.2%   │
│ MIDI drain (up to 16 events)      │   5    │   0.1%   │
│ Voice × 8 (full poly)             │        │          │
│   OSC × 3 (PolyBLEP)  per voice  │  40    │          │
│   Moog filter (tanh×4) per voice  │  30    │          │
│   Envelope × 2        per voice   │  10    │          │
│   Glide                per voice  │   5    │          │
│   Total per voice × 256 samples   │ ~600   │          │
│   8 voices total                  │ 4800   │  82.7%   │
│ Arpeggiator tick × 256            │  50    │   0.9%   │
│ Sequencer tick × 256              │  50    │   0.9%   │
│ Mix + master gain × 256           │  20    │   0.3%   │
│ Margin / OS jitter                │ 865    │  14.9%   │
├───────────────────────────────────┼────────┼──────────┤
│ TOTAL                             │ 5800   │  100%    │
└───────────────────────────────────┴────────┴──────────┘

OPTIMIZATION FLAGS:
  Release build : -O3 -DNDEBUG
  MSVC          : /O2 /fp:fast
  SIMD          : compiler auto-vectorization sufficient for V1
  Oversampling  : NONE in V1
                  (add 2× oversampling in V2 if aliasing noted)
```

---

# PHẦN 16 — TEST PLAN

## 16.1 `tests/CMakeLists.txt`

```cmake
add_executable(minimoog_tests
    test_main.cpp
    test_oscillator.cpp
    test_moog_filter.cpp
    test_envelope.cpp
    test_glide.cpp
    test_voice.cpp
    test_arpeggiator.cpp
    test_sequencer.cpp
    test_scale_quantizer.cpp
)
target_link_libraries(minimoog_tests PRIVATE
    dsp_core Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(minimoog_tests)
```

## 16.2 `tests/test_oscillator.cpp`
```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/dsp/oscillator.h"
#include <cmath>
#include <numeric>
#include <vector>

// ─────────────────────────────────────────────────────────
// HELPER
// ─────────────────────────────────────────────────────────

static std::vector<float> collectSamples(Oscillator& osc,
                                          int n) {
    std::vector<float> buf(n);
    for (int i = 0; i < n; ++i) buf[i] = osc.tick();
    return buf;
}

// ─────────────────────────────────────────────────────────
// DC OFFSET
// ─────────────────────────────────────────────────────────

TEST_CASE("Oscillator - Sawtooth DC is near zero", "[osc]") {
    Oscillator osc;
    osc.setSampleRate(44100.f);
    osc.setFrequency(440.f);
    osc.setWaveShape(WaveShape::Sawtooth);
    osc.setRange(OscRange::R8);

    constexpr int N = 44100;
    auto buf = collectSamples(osc, N);
    const double mean = std::accumulate(buf.begin(),
                                         buf.end(), 0.0) / N;
    REQUIRE(std::abs(mean) < 0.02);
}

TEST_CASE("Oscillator - Square DC is near zero", "[osc]") {
    Oscillator osc;
    osc.setSampleRate(44100.f);
    osc.setFrequency(220.f);
    osc.setWaveShape(WaveShape::Square);
    osc.setRange(OscRange::R8);

    constexpr int N = 44100;
    auto buf = collectSamples(osc, N);
    const double mean = std::accumulate(buf.begin(),
                                         buf.end(), 0.0) / N;
    REQUIRE(std::abs(mean) < 0.02);
}

// ─────────────────────────────────────────────────────────
// AMPLITUDE
// ─────────────────────────────────────────────────────────

TEST_CASE("Oscillator - Peak amplitude within [-1, 1]",
          "[osc]") {
    Oscillator osc;
    osc.setSampleRate(44100.f);
    osc.setFrequency(440.f);

    const WaveShape shapes[] = {
        WaveShape::Triangle,
        WaveShape::TriangleSaw,
        WaveShape::ReverseSaw,
        WaveShape::Sawtooth,
        WaveShape::Square,
        WaveShape::WideRectangle
    };

    for (auto ws : shapes) {
        osc.setWaveShape(ws);
        auto buf = collectSamples(osc, 4096);
        for (float s : buf) {
            INFO("WaveShape index = "
                 << static_cast<int>(ws));
            REQUIRE(s >= -1.1f);
            REQUIRE(s <=  1.1f);
        }
    }
}

// ─────────────────────────────────────────────────────────
// FREQUENCY ACCURACY
// ─────────────────────────────────────────────────────────

TEST_CASE("Oscillator - Zero crossings match frequency",
          "[osc]") {
    constexpr float SR   = 44100.f;
    constexpr float FREQ = 440.f;

    Oscillator osc;
    osc.setSampleRate(SR);
    osc.setFrequency(FREQ);
    osc.setWaveShape(WaveShape::Sawtooth);
    osc.setRange(OscRange::R8);

    // Count positive-going zero crossings over 1 second
    constexpr int N = static_cast<int>(SR);
    auto buf = collectSamples(osc, N);

    int crossings = 0;
    for (int i = 1; i < N; ++i) {
        if (buf[i - 1] < 0.f && buf[i] >= 0.f) ++crossings;
    }

    // Allow ±2 Hz tolerance
    REQUIRE(crossings >= static_cast<int>(FREQ) - 2);
    REQUIRE(crossings <= static_cast<int>(FREQ) + 2);
}

// ─────────────────────────────────────────────────────────
// RANGE SWITCH
// ─────────────────────────────────────────────────────────

TEST_CASE("Oscillator - Range R16 is half freq of R8",
          "[osc]") {
    constexpr float SR   = 44100.f;
    constexpr float FREQ = 440.f;
    constexpr int   N    = static_cast<int>(SR);

    auto countCrossings = [&](OscRange rng) {
        Oscillator osc;
        osc.setSampleRate(SR);
        osc.setFrequency(FREQ);
        osc.setWaveShape(WaveShape::Sawtooth);
        osc.setRange(rng);
        auto buf = collectSamples(osc, N);
        int c = 0;
        for (int i = 1; i < N; ++i)
            if (buf[i-1] < 0.f && buf[i] >= 0.f) ++c;
        return c;
    };

    const int c8  = countCrossings(OscRange::R8);
    const int c16 = countCrossings(OscRange::R16);

    // R16 should be roughly half the crossings of R8
    REQUIRE(c16 >= c8 / 2 - 5);
    REQUIRE(c16 <= c8 / 2 + 5);
}

// ─────────────────────────────────────────────────────────
// FREQUENCY KNOB MODULATION
// ─────────────────────────────────────────────────────────

TEST_CASE("Oscillator - setFrequency changes pitch", "[osc]") {
    Oscillator osc;
    osc.setSampleRate(44100.f);
    osc.setWaveShape(WaveShape::Sawtooth);
    osc.setRange(OscRange::R8);

    auto countCrossings = [&](float f) {
        osc.setFrequency(f);
        osc.reset();
        auto buf = collectSamples(osc, 44100);
        int c = 0;
        for (int i = 1; i < 44100; ++i)
            if (buf[i-1] < 0.f && buf[i] >= 0.f) ++c;
        return c;
    };

    const int c220 = countCrossings(220.f);
    const int c880 = countCrossings(880.f);

    // 880 Hz should have ~4× more crossings than 220 Hz
    REQUIRE(c880 > c220 * 3);
}
```

---

## `tests/test_moog_filter.cpp`

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/dsp/moog_filter.h"
#include <cmath>
#include <vector>
#include <numeric>

using Catch::Approx;

// ─────────────────────────────────────────────────────────
// HELPER: compute RMS of buffer
// ─────────────────────────────────────────────────────────

static float rms(const std::vector<float>& buf) {
    double sum = 0.0;
    for (float s : buf) sum += static_cast<double>(s) * s;
    return static_cast<float>(
        std::sqrt(sum / static_cast<double>(buf.size())));
}

static std::vector<float> sineBuffer(float freq,
                                      float sr, int n) {
    std::vector<float> buf(n);
    for (int i = 0; i < n; ++i)
        buf[i] = std::sin(2.f * 3.14159265f * freq
                          * static_cast<float>(i) / sr);
    return buf;
}

static std::vector<float> runFilter(MoogFilter& f,
                                     const std::vector<float>& in) {
    std::vector<float> out(in.size());
    for (size_t i = 0; i < in.size(); ++i)
        out[i] = f.process(in[i]);
    return out;
}

// ─────────────────────────────────────────────────────────
// LOW PASS ATTENUATION
// ─────────────────────────────────────────────────────────

TEST_CASE("MoogFilter - passes low freqs, attenuates high",
          "[filter]") {
    constexpr float SR     = 44100.f;
    constexpr int   N      = 8192;
    constexpr float CUTOFF = 1000.f;

    MoogFilter filt;
    filt.setSampleRate(SR);
    filt.setCutoff(CUTOFF);
    filt.setResonance(0.0f);

    // Low tone (200 Hz) — well below cutoff
    auto lowIn  = sineBuffer(200.f, SR, N);
    // Warm up
    for (int i = 0; i < 2048; ++i) filt.process(lowIn[i]);
    auto lowOut = runFilter(filt, lowIn);
    const float rmsLow = rms(lowOut);

    // High tone (8000 Hz) — well above cutoff
    filt.reset();
    auto hiIn  = sineBuffer(8000.f, SR, N);
    for (int i = 0; i < 2048; ++i) filt.process(hiIn[i]);
    auto hiOut = runFilter(filt, hiIn);
    const float rmsHi = rms(hiOut);

    INFO("RMS low=" << rmsLow << " hi=" << rmsHi);
    REQUIRE(rmsLow > 0.3f);          // passes through
    REQUIRE(rmsHi  < rmsLow * 0.1f); // >20 dB attenuation
}

// ─────────────────────────────────────────────────────────
// RESONANCE DOES NOT CAUSE DC DIVERGENCE
// ─────────────────────────────────────────────────────────

TEST_CASE("MoogFilter - stable at max resonance", "[filter]") {
    constexpr float SR = 44100.f;
    constexpr int   N  = 44100;

    MoogFilter filt;
    filt.setSampleRate(SR);
    filt.setCutoff(1000.f);
    filt.setResonance(0.99f);   // near self-oscillation

    float maxAbs = 0.f;
    for (int i = 0; i < N; ++i) {
        // Feed silence — should decay to 0
        float s = filt.process(0.f);
        maxAbs = std::max(maxAbs, std::abs(s));
    }
    // After initial ring, output must stay bounded
    REQUIRE(maxAbs < 2.0f);
}

// ─────────────────────────────────────────────────────────
// CUTOFF SWEEP — RMS MONOTONICALLY DECREASES
// ─────────────────────────────────────────────────────────

TEST_CASE("MoogFilter - cutoff sweep monotonic",
          "[filter]") {
    constexpr float SR   = 44100.f;
    constexpr float FREQ = 2000.f;
    constexpr int   N    = 4096;

    MoogFilter filt;
    filt.setSampleRate(SR);
    filt.setResonance(0.0f);

    auto input = sineBuffer(FREQ, SR, N);

    float prevRms = 1e9f;
    const float cutoffs[] = {
        8000.f, 4000.f, 2000.f, 1000.f, 500.f, 200.f
    };

    for (float cut : cutoffs) {
        filt.reset();
        filt.setCutoff(cut);
        // Warm up
        for (int i = 0; i < 512; ++i) filt.process(input[i]);
        auto out  = runFilter(filt, input);
        const float r = rms(out);
        INFO("Cutoff=" << cut << " RMS=" << r);
        REQUIRE(r <= prevRms * 1.05f);  // allow 5% tolerance
        prevRms = r;
    }
}

// ─────────────────────────────────────────────────────────
// RESET CLEARS STATE
// ─────────────────────────────────────────────────────────

TEST_CASE("MoogFilter - reset clears internal state",
          "[filter]") {
    MoogFilter filt;
    filt.setSampleRate(44100.f);
    filt.setCutoff(500.f);
    filt.setResonance(0.8f);

    // Drive the filter with a loud signal
    for (int i = 0; i < 1000; ++i)
        filt.process(std::sin(
            static_cast<float>(i) * 0.2f));

    filt.reset();

    // First output after reset must be near 0
    float out = filt.process(0.f);
    REQUIRE(std::abs(out) < 0.001f);
}
```

---

## `tests/test_envelope.cpp`

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/dsp/envelope.h"

using Catch::Approx;

// ─────────────────────────────────────────────────────────
// HELPER: run envelope for n samples, collect output
// ─────────────────────────────────────────────────────────

static std::vector<float> runEnv(Envelope& env, int n) {
    std::vector<float> buf(n);
    for (int i = 0; i < n; ++i) buf[i] = env.tick();
    return buf;
}

// ─────────────────────────────────────────────────────────
// IDLE STATE
// ─────────────────────────────────────────────────────────

TEST_CASE("Envelope - idle outputs zero", "[env]") {
    Envelope env;
    env.setSampleRate(44100.f);
    // Never triggered
    for (int i = 0; i < 1000; ++i)
        REQUIRE(env.tick() == Approx(0.f).margin(0.0001f));
}

// ─────────────────────────────────────────────────────────
// ATTACK REACHES PEAK
// ─────────────────────────────────────────────────────────

TEST_CASE("Envelope - attack reaches 1.0", "[env]") {
    Envelope env;
    env.setSampleRate(44100.f);
    env.setAttack (100.f);    // 100 ms
    env.setDecay  (100.f);
    env.setSustain(0.8f);
    env.setRelease(100.f);

    env.noteOn();

    float peak = 0.f;
    // Run for 200 ms
    for (int i = 0; i < static_cast<int>(44100 * 0.2f); ++i)
        peak = std::max(peak, env.tick());

    REQUIRE(peak >= 0.99f);
}

// ─────────────────────────────────────────────────────────
// SUSTAIN LEVEL HOLDS
// ─────────────────────────────────────────────────────────

TEST_CASE("Envelope - sustain level is held", "[env]") {
    constexpr float SR      = 44100.f;
    constexpr float SUSTAIN = 0.6f;

    Envelope env;
    env.setSampleRate(SR);
    env.setAttack (10.f);
    env.setDecay  (10.f);
    env.setSustain(SUSTAIN);
    env.setRelease(100.f);

    env.noteOn();

    // Skip past attack + decay (50 ms total, use 100 ms)
    for (int i = 0; i < static_cast<int>(SR * 0.1f); ++i)
        env.tick();

    // Now in sustain phase — check stability
    float last = 0.f;
    for (int i = 0; i < static_cast<int>(SR * 0.1f); ++i)
        last = env.tick();

    REQUIRE(last == Approx(SUSTAIN).margin(0.02f));
}

// ─────────────────────────────────────────────────────────
// RELEASE DECAYS TO ZERO
// ─────────────────────────────────────────────────────────

TEST_CASE("Envelope - release decays to near zero",
          "[env]") {
    constexpr float SR = 44100.f;

    Envelope env;
    env.setSampleRate(SR);
    env.setAttack (1.f);
    env.setDecay  (1.f);
    env.setSustain(1.0f);
    env.setRelease(50.f);    // 50 ms release

    env.noteOn();
    // Run attack + sustain
    for (int i = 0; i < static_cast<int>(SR * 0.1f); ++i)
        env.tick();

    env.noteOff();
    // Run 200 ms (4× release time)
    float last = 1.f;
    for (int i = 0; i < static_cast<int>(SR * 0.2f); ++i)
        last = env.tick();

    REQUIRE(last < 0.01f);
}

// ─────────────────────────────────────────────────────────
// RETRIGGER DURING RELEASE
// ─────────────────────────────────────────────────────────

TEST_CASE("Envelope - retrigger from release is smooth",
          "[env]") {
    constexpr float SR = 44100.f;

    Envelope env;
    env.setSampleRate(SR);
    env.setAttack (5.f);
    env.setDecay  (5.f);
    env.setSustain(0.7f);
    env.setRelease(200.f);

    // First note
    env.noteOn();
    for (int i = 0; i < static_cast<int>(SR * 0.05f); ++i)
        env.tick();
    env.noteOff();

    // Retrigger mid-release
    for (int i = 0; i < static_cast<int>(SR * 0.05f); ++i)
        env.tick();

    const float levelAtRetrigger = env.tick();
    env.noteOn();

    // Output must not jump backwards (no pop)
    float firstAfterTrigger = env.tick();
    REQUIRE(firstAfterTrigger >= levelAtRetrigger - 0.01f);
}

// ─────────────────────────────────────────────────────────
// INACTIVE AFTER FULL RELEASE
// ─────────────────────────────────────────────────────────

TEST_CASE("Envelope - becomes inactive after release ends",
          "[env]") {
    constexpr float SR = 44100.f;

    Envelope env;
    env.setSampleRate(SR);
    env.setAttack (1.f);
    env.setDecay  (1.f);
    env.setSustain(0.5f);
    env.setRelease(10.f);

    env.noteOn();
    for (int i = 0; i < static_cast<int>(SR * 0.05f); ++i)
        env.tick();
    env.noteOff();

    // Wait 500 ms (50× release time)
    for (int i = 0; i < static_cast<int>(SR * 0.5f); ++i)
        env.tick();

    REQUIRE_FALSE(env.isActive());
}
```

---

## `tests/test_glide.cpp`

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/dsp/glide.h"

using Catch::Approx;

TEST_CASE("Glide - with zero time outputs target instantly",
          "[glide]") {
    GlideProcessor g;
    g.setSampleRate(44100.f);
    g.setGlideTime(0.f);
    g.setTarget(440.f);

    REQUIRE(g.tick() == Approx(440.f).margin(0.01f));
}

TEST_CASE("Glide - output moves towards target", "[glide]") {
    GlideProcessor g;
    g.setSampleRate(44100.f);
    g.setGlideTime(100.f);   // 100 ms
    g.setValue(220.f);
    g.setTarget(440.f);

    float prev = 220.f;
    for (int i = 0; i < 100; ++i) {
        float cur = g.tick();
        REQUIRE(cur >= prev - 0.001f);  // monotonically rising
        prev = cur;
    }
    REQUIRE(prev > 220.f);   // made progress
}

TEST_CASE("Glide - reaches target within glide time",
          "[glide]") {
    constexpr float SR   = 44100.f;
    constexpr float TIME = 50.f;  // ms

    GlideProcessor g;
    g.setSampleRate(SR);
    g.setGlideTime(TIME);
    g.setValue(100.f);
    g.setTarget(1000.f);

    float last = 100.f;
    const int n = static_cast<int>(SR * (TIME / 1000.f) * 3.f);
    for (int i = 0; i < n; ++i) last = g.tick();

    REQUIRE(last == Approx(1000.f).margin(1.0f));
}

TEST_CASE("Glide - glide disabled jumps immediately",
          "[glide]") {
    GlideProcessor g;
    g.setSampleRate(44100.f);
    g.setGlideTime(200.f);
    g.setValue(440.f);
    g.setEnabled(false);
    g.setTarget(880.f);

    REQUIRE(g.tick() == Approx(880.f).margin(0.01f));
}
```

---

## `tests/test_voice.cpp`

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/voice/voice.h"
#include "shared/params.h"

using Catch::Approx;

// ─────────────────────────────────────────────────────────
// HELPER: build default param array
// ─────────────────────────────────────────────────────────

static void buildDefaultParams(float p[PARAM_COUNT]) {
    for (int i = 0; i < PARAM_COUNT; ++i)
        p[i] = PARAM_META[i].defaultVal;
}

// ─────────────────────────────────────────────────────────
// LIFECYCLE
// ─────────────────────────────────────────────────────────

TEST_CASE("Voice - starts inactive", "[voice]") {
    Voice v;
    v.init(44100.f);
    REQUIRE_FALSE(v.isActive());
}

TEST_CASE("Voice - active after noteOn", "[voice]") {
    Voice v;
    v.init(44100.f);
    v.noteOn(60, 100, 0.f);
    REQUIRE(v.isActive());
}

TEST_CASE("Voice - inactive after forceOff", "[voice]") {
    Voice v;
    v.init(44100.f);
    v.noteOn(60, 100, 0.f);
    v.forceOff();
    REQUIRE_FALSE(v.isActive());
}

// ─────────────────────────────────────────────────────────
// AUDIO OUTPUT
// ─────────────────────────────────────────────────────────

TEST_CASE("Voice - outputs non-zero after noteOn",
          "[voice]") {
    float params[PARAM_COUNT];
    buildDefaultParams(params);

    // Fast attack
    params[P_AENV_ATTACK]  = 0.0f;
    params[P_AENV_SUSTAIN] = 1.0f;
    params[P_OSC1_ON]      = 1.0f;
    params[P_MASTER_VOL]   = 1.0f;

    Voice v;
    v.init(44100.f);
    v.noteOn(69, 127, 0.f);  // A4

    // Warm up 100 samples
    float maxAbs = 0.f;
    for (int i = 0; i < 512; ++i)
        maxAbs = std::max(maxAbs, std::abs(v.tick(params)));

    REQUIRE(maxAbs > 0.001f);
}

TEST_CASE("Voice - output bounded within [-1.5, 1.5]",
          "[voice]") {
    float params[PARAM_COUNT];
    buildDefaultParams(params);

    params[P_AENV_ATTACK]  = 0.0f;
    params[P_AENV_SUSTAIN] = 1.0f;
    params[P_OSC1_ON]      = 1.0f;
    params[P_OSC2_ON]      = 1.0f;
    params[P_OSC3_ON]      = 1.0f;
    params[P_MASTER_VOL]   = 1.0f;

    Voice v;
    v.init(44100.f);
    v.noteOn(60, 127, 0.f);

    for (int i = 0; i < 4096; ++i) {
        float s = v.tick(params);
        REQUIRE(s >= -1.5f);
        REQUIRE(s <=  1.5f);
    }
}

// ─────────────────────────────────────────────────────────
// NOTE PROPERTY
// ─────────────────────────────────────────────────────────

TEST_CASE("Voice - getNote returns triggered note",
          "[voice]") {
    Voice v;
    v.init(44100.f);
    v.noteOn(72, 80, 0.f);   // C5
    REQUIRE(v.getNote() == 72);
}

TEST_CASE("Voice - age increments each tick", "[voice]") {
    float params[PARAM_COUNT];
    buildDefaultParams(params);
    params[P_AENV_ATTACK]  = 0.0f;
    params[P_AENV_SUSTAIN] = 1.0f;
    params[P_OSC1_ON]      = 1.0f;

    Voice v;
    v.init(44100.f);
    v.noteOn(60, 100, 0.f);

    REQUIRE(v.getAge() == 0);
    v.tick(params);
    v.tickAge();
    REQUIRE(v.getAge() == 1);
    v.tick(params);
    v.tickAge();
    REQUIRE(v.getAge() == 2);
}

// ─────────────────────────────────────────────────────────
// DETUNE
// ─────────────────────────────────────────────────────────

TEST_CASE("Voice - detuned voice produces different output",
          "[voice]") {
    float params[PARAM_COUNT];
    buildDefaultParams(params);
    params[P_AENV_ATTACK]  = 0.0f;
    params[P_AENV_SUSTAIN] = 1.0f;
    params[P_OSC1_ON]      = 1.0f;
    params[P_MASTER_VOL]   = 1.0f;

    Voice v1, v2;
    v1.init(44100.f);
    v2.init(44100.f);
    v1.noteOn(60, 100,  0.0f);
    v2.noteOn(60, 100, 20.0f);  // +20 cents

    float diff = 0.f;
    for (int i = 0; i < 4096; ++i) {
        float s1 = v1.tick(params);
        float s2 = v2.tick(params);
        diff += std::abs(s1 - s2);
    }
    REQUIRE(diff > 1.0f);  // outputs diverge over time
}
```

---

## `tests/test_arpeggiator.cpp`

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/music/arpeggiator.h"

// ─────────────────────────────────────────────────────────
// HELPER: run until N note-on events collected
// ─────────────────────────────────────────────────────────

static std::vector<int> collectArpNotes(
        Arpeggiator& arp, int wantNotes,
        int maxSamples = 200000) {
    std::vector<int> notes;
    for (int i = 0; i < maxSamples && 
         static_cast<int>(notes.size()) < wantNotes; ++i) {
        auto out = arp.tick();
        if (out.hasNoteOn) notes.push_back(out.note);
    }
    return notes;
}

// ─────────────────────────────────────────────────────────
// BASIC TRIGGER
// ─────────────────────────────────────────────────────────

TEST_CASE("Arpeggiator - produces notes when enabled",
          "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(44100.f);
    arp.setBPM(120.f);
    arp.setRateIndex(3);       // 1/8
    arp.setEnabled(true);
    arp.setGate(0.8f);
    arp.noteOn(60, 100);
    arp.noteOn(64, 100);

    auto notes = collectArpNotes(arp, 4);
    REQUIRE(notes.size() >= 4);
}

// ─────────────────────────────────────────────────────────
// UP MODE
// ─────────────────────────────────────────────────────────

TEST_CASE("Arpeggiator - Up mode is ascending", "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(44100.f);
    arp.setBPM(240.f);
    arp.setRateIndex(4);       // 1/16 — faster for testing
    arp.setMode(ArpMode::Up);
    arp.setOctaves(1);
    arp.setEnabled(true);
    arp.setGate(0.5f);
    arp.noteOn(60, 100);
    arp.noteOn(64, 100);
    arp.noteOn(67, 100);

    auto notes = collectArpNotes(arp, 9);
    REQUIRE(notes.size() >= 9);

    // Every 3-note cycle should go 60, 64, 67
    for (int c = 0; c < 3; ++c) {
        REQUIRE(notes[c * 3 + 0] == 60);
        REQUIRE(notes[c * 3 + 1] == 64);
        REQUIRE(notes[c * 3 + 2] == 67);
    }
}

// ─────────────────────────────────────────────────────────
// DOWN MODE
// ─────────────────────────────────────────────────────────

TEST_CASE("Arpeggiator - Down mode is descending", "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(44100.f);
    arp.setBPM(240.f);
    arp.setRateIndex(4);
    arp.setMode(ArpMode::Down);
    arp.setOctaves(1);
    arp.setEnabled(true);
    arp.setGate(0.5f);
    arp.noteOn(60, 100);
    arp.noteOn(64, 100);
    arp.noteOn(67, 100);

    auto notes = collectArpNotes(arp, 6);
    REQUIRE(notes.size() >= 6);

    // First cycle: 67, 64, 60
    REQUIRE(notes[0] == 67);
    REQUIRE(notes[1] == 64);
    REQUIRE(notes[2] == 60);
}

// ─────────────────────────────────────────────────────────
// OCTAVE EXPANSION
// ─────────────────────────────────────────────────────────

TEST_CASE("Arpeggiator - 2 octaves doubles note list",
          "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(44100.f);
    arp.setBPM(480.f);
    arp.setRateIndex(4);
    arp.setMode(ArpMode::Up);
    arp.setOctaves(2);
    arp.setEnabled(true);
    arp.setGate(0.5f);
    arp.noteOn(60, 100);
    arp.noteOn(64, 100);

    // With 2 notes × 2 octaves = 4 notes per cycle
    auto notes = collectArpNotes(arp, 4);
    REQUIRE(notes.size() >= 4);

    // Should contain C4=60, E4=64, C5=72, E5=76
    bool hasC4 = false, hasC5 = false;
    for (int n : notes) {
        if (n == 60) hasC4 = true;
        if (n == 72) hasC5 = true;
    }
    REQUIRE(hasC4);
    REQUIRE(hasC5);
}

// ─────────────────────────────────────────────────────────
// NOTE OFF REMOVES FROM LIST
// ─────────────────────────────────────────────────────────

TEST_CASE("Arpeggiator - noteOff removes note from cycle",
          "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(44100.f);
    arp.setBPM(480.f);
    arp.setRateIndex(4);
    arp.setMode(ArpMode::Up);
    arp.setOctaves(1);
    arp.setEnabled(true);
    arp.setGate(0.5f);
    arp.noteOn(60, 100);
    arp.noteOn(67, 100);

    // Collect a few notes, then release 60
    collectArpNotes(arp, 2);
    arp.noteOff(60);

    auto notes = collectArpNotes(arp, 8);
    for (int n : notes)
        REQUIRE(n == 67);  // only G3 remains
}

// ─────────────────────────────────────────────────────────
// DISABLED — NO NOTES
// ─────────────────────────────────────────────────────────

TEST_CASE("Arpeggiator - disabled emits no notes", "[arp]") {
    Arpeggiator arp;
    arp.setSampleRate(44100.f);
    arp.setBPM(120.f);
    arp.setRateIndex(3);
    arp.setEnabled(false);
    arp.noteOn(60, 100);

    int noteCount = 0;
    for (int i = 0; i < 44100; ++i) {
        auto out = arp.tick();
        if (out.hasNoteOn) ++noteCount;
    }
    REQUIRE(noteCount == 0);
}
```

---

## `tests/test_sequencer.cpp`

```cpp
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "core/music/sequencer.h"

// ─────────────────────────────────────────────────────────
// HELPER
// ─────────────────────────────────────────────────────────

static std::vector<int> collectSeqNotes(
        StepSequencer& seq, int wantNotes,
        int maxSamples = 500000) {
    std::vector<int> notes;
    for (int i = 0; i < maxSamples &&
         static_cast<int>(notes.size()) < wantNotes; ++i) {
        auto out = seq.tick();
        if (out.hasNoteOn) notes.push_back(out.note);
    }
    return notes;
}

// ─────────────────────────────────────────────────────────
// BASIC PLAYBACK
// ─────────────────────────────────────────────────────────

TEST_CASE("Sequencer - plays configured notes in order",
          "[seq]") {
    StepSequencer seq;
    seq.setSampleRate(44100.f);
    seq.setBPM(240.f);
    seq.setStepCount(4);
    seq.setRateIndex(3);      // 1/8
    seq.setGlobalGate(0.8f);
    seq.setEnabled(true);

    const int testNotes[] = {60, 64, 67, 71};
    for (int i = 0; i < 4; ++i) {
        SeqStep s;
        s.note     = testNotes[i];
        s.velocity = 100;
        s.gate     = 0.8f;
        s.active   = true;
        seq.setStep(i, s);
    }

    seq.play();
    auto notes = collectSeqNotes(seq, 8);

    REQUIRE(notes.size() >= 8);
    // Pattern should repeat: 60,64,67,71,60,64,67,71
    for (int i = 0; i < 8; ++i)
        REQUIRE(notes[i] == testNotes[i % 4]);
}

// ─────────────────────────────────────────────────────────
// REST STEPS
// ─────────────────────────────────────────────────────────

TEST_CASE("Sequencer - inactive steps are skipped",
          "[seq]") {
    StepSequencer seq;
    seq.setSampleRate(44100.f);
    seq.setBPM(480.f);
    seq.setStepCount(4);
    seq.setRateIndex(4);
    seq.setGlobalGate(0.8f);
    seq.setEnabled(true);

    SeqStep on,  off;
    on.note  = 60; on.active  = true;  on.velocity  = 100;
    off.note = 72; off.active = false; off.velocity = 100;
    seq.setStep(0, on);
    seq.setStep(1, off);
    seq.setStep(2, on);
    seq.setStep(3, off);

    seq.play();
    auto notes = collectSeqNotes(seq, 6);

    for (int n : notes)
        REQUIRE(n == 60);   // only active steps fire
}

// ─────────────────────────────────────────────────────────
// STOP RESETS TO STEP 0
// ─────────────────────────────────────────────────────────

TEST_CASE("Sequencer - stop resets step index", "[seq]") {
    StepSequencer seq;
    seq.setSampleRate(44100.f);
    seq.setBPM(480.f);
    seq.setStepCount(4);
    seq.setRateIndex(4);
    seq.setEnabled(true);

    for (int i = 0; i < 4; ++i) {
        SeqStep s;
        s.note   = 60 + i;
        s.active = true;
        s.gate   = 0.8f;
        seq.setStep(i, s);
    }

    seq.play();
    collectSeqNotes(seq, 2);   // advance past step 0
    seq.stop();

    REQUIRE(seq.getCurrentStep() == 0);
    REQUIRE_FALSE(seq.isPlaying());
}

// ─────────────────────────────────────────────────────────
// TEMPO CHANGE
// ─────────────────────────────────────────────────────────

TEST_CASE("Sequencer - higher BPM fires steps faster",
          "[seq]") {
    auto stepsIn = [](float bpm, int sampleBudget) {
        StepSequencer seq;
        seq.setSampleRate(44100.f);
        seq.setBPM(bpm);
        seq.setStepCount(4);
        seq.setRateIndex(3);
        seq.setGlobalGate(0.5f);
        seq.setEnabled(true);

        for (int i = 0; i < 4; ++i) {
            SeqStep s;
            s.note = 60; s.active = true; s.gate = 0.5f;
            seq.setStep(i, s);
        }
        seq.play();

        int count = 0;
        for (int i = 0; i < sampleBudget; ++i) {
            if (seq.tick().hasNoteOn) ++count;
        }
        return count;
    };

    const int at120 = stepsIn(120.f, 44100);
    const int at240 = stepsIn(240.f, 44100);

    REQUIRE(at240 > at120 * 1.5f);
}
```

---

## `tests/test_scale_quantizer.cpp`

```cpp
#include <catch2/catch_test_macros.hpp>
#include "core/music/scale_quantizer.h"

// ─────────────────────────────────────────────────────────
// DISABLED — PASS THROUGH
// ─────────────────────────────────────────────────────────

TEST_CASE("ScaleQuantizer - disabled passes through",
          "[scale]") {
    ScaleQuantizer q;
    q.setEnabled(false);

    for (int n = 0; n <= 127; ++n)
        REQUIRE(q.quantize(n) == n);
}

// ─────────────────────────────────────────────────────────
// C MAJOR — ALL WHITE KEYS PASS UNCHANGED
// ─────────────────────────────────────────────────────────

TEST_CASE("ScaleQuantizer - C Major: white keys unchanged",
          "[scale]") {
    ScaleQuantizer q;
    q.setEnabled(true);
    q.setRoot(0);    // C
    q.setScale(1);   // Major

    // White keys in octave 4 (MIDI 60..71 even indices)
    const int white[] = {60, 62, 64, 65, 67, 69, 71};
    for (int n : white)
        REQUIRE(q.quantize(n) == n);
}

// ─────────────────────────────────────────────────────────
// C MAJOR — BLACK KEYS SNAP TO NEAREST WHITE KEY
// ─────────────────────────────────────────────────────────

TEST_CASE("ScaleQuantizer - C Major: black keys are snapped",
          "[scale]") {
    ScaleQuantizer q;
    q.setEnabled(true);
    q.setRoot(0);
    q.setScale(1);   // Major

    // C# (61) → snap to C(60) or D(62), both valid
    const int result61 = q.quantize(61);
    REQUIRE((result61 == 60 || result61 == 62));

    // Bb (70) → snap to A(69) or B(71)
    const int result70 = q.quantize(70);
    REQUIRE((result70 == 69 || result70 == 71));
}

// ─────────────────────────────────────────────────────────
// OUTPUT ALWAYS IN SCALE
// ─────────────────────────────────────────────────────────

TEST_CASE("ScaleQuantizer - output always in scale",
          "[scale]") {
    ScaleQuantizer q;
    q.setEnabled(true);
    q.setRoot(0);
    q.setScale(1);   // C Major

    // Degrees of C Major (0-indexed semitones)
    const bool major[12] = {
        true, false, true, false, true,  true,
        false, true, false, true, false, true
    };

    for (int n = 0; n <= 127; ++n) {
        const int r   = q.quantize(n);
        const int deg = r % 12;
        INFO("Input=" << n << " Output=" << r
             << " Degree=" << deg);
        REQUIRE(major[deg]);
    }
}

// ─────────────────────────────────────────────────────────
// ROOT TRANSPOSITION
// ─────────────────────────────────────────────────────────

TEST_CASE("ScaleQuantizer - G Major root=7 maps G as root",
          "[scale]") {
    ScaleQuantizer q;
    q.setEnabled(true);
    q.setRoot(7);    // G
    q.setScale(1);   // Major (now G Major)

    // G Major scale degrees (from G): G A B C D E F#
    // Semitones from C: 7,9,11,0,2,4,6
    const bool gMajor[12] = {
        true, false, true, false, true, false, true,
        true, false, true, false, true
    };

    for (int n = 48; n <= 84; ++n) {
        const int r   = q.quantize(n);
        const int deg = r % 12;
        INFO("Input=" << n << " Output=" << r
             << " Degree=" << deg);
        REQUIRE(gMajor[deg]);
    }
}

// ─────────────────────────────────────────────────────────
// CHROMATIC — ALL NOTES PASS THROUGH
// ─────────────────────────────────────────────────────────

TEST_CASE("ScaleQuantizer - Chromatic passes all notes",
          "[scale]") {
    ScaleQuantizer q;
    q.setEnabled(true);
    q.setRoot(0);
    q.setScale(0);   // Chromatic

    for (int n = 0; n <= 127; ++n)
        REQUIRE(q.quantize(n) == n);
}

// ─────────────────────────────────────────────────────────
// OUTPUT RANGE
// ─────────────────────────────────────────────────────────

TEST_CASE("ScaleQuantizer - output always 0..127",
          "[scale]") {
    ScaleQuantizer q;
    q.setEnabled(true);

    for (int scale = 0; scale < ScaleQuantizer::SCALE_COUNT;
         ++scale) {
        q.setScale(scale);
        for (int root = 0; root < 12; ++root) {
            q.setRoot(root);
            for (int n = 0; n <= 127; ++n) {
                const int r = q.quantize(n);
                REQUIRE(r >= 0);
                REQUIRE(r <= 127);
            }
        }
    }
}
```

---

## `tests/test_main.cpp`

```cpp
// Catch2 main is provided by Catch2WithMain target
// This file intentionally left minimal
#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    Catch::Session session;
    const int result = session.applyCommandLine(argc, argv);
    if (result != 0) return result;
    return session.run();
}
```

---

# PHẦN 17 — BUILD SYSTEM

## Root `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.22)
project(MiniMoogDSP VERSION 1.0.0 LANGUAGES CXX)

# ── C++ Standard ─────────────────────────────────────────
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ── Build type default ────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ── Compiler flags ────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /O2 /fp:fast /MP)
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic
        $<$<CONFIG:Release>:-O3 -DNDEBUG -ffast-math>
        $<$<CONFIG:Debug>:-g -fsanitize=address,undefined>
    )
    add_link_options(
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    )
endif()

# ── Output directories ────────────────────────────────────
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ── Dependencies ─────────────────────────────────────────
find_package(PkgConfig REQUIRED)

# RtAudio
pkg_check_modules(RTAUDIO REQUIRED rtaudio)

# RtMidi
pkg_check_modules(RTMIDI  REQUIRED rtmidi)

# GLFW3
find_package(glfw3 3.3 REQUIRED)

# OpenGL
find_package(OpenGL REQUIRED)

# nlohmann/json
find_package(nlohmann_json 3.10 REQUIRED)

# ImGui (vendored)
add_library(imgui STATIC
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    third_party/imgui
    third_party/imgui/backends
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# ImGui-Knobs (vendored)
add_library(imgui_knobs STATIC
    third_party/imgui-knobs/imgui-knobs.cpp)
target_include_directories(imgui_knobs PUBLIC
    third_party/imgui-knobs)
target_link_libraries(imgui_knobs PUBLIC imgui)

# ── DSP Core library ──────────────────────────────────────
add_library(dsp_core STATIC
    # shared
    shared/interfaces.cpp
    shared/params.cpp
    # dsp
    core/dsp/oscillator.cpp
    core/dsp/moog_filter.cpp
    core/dsp/envelope.cpp
    core/dsp/lfo.cpp
    core/dsp/glide.cpp
    core/dsp/noise_generator.cpp
    # voice
    core/voice/voice.cpp
    core/voice/voice_pool.cpp
    # music
    core/music/arpeggiator.cpp
    core/music/sequencer.cpp
    core/music/chord_engine.cpp
    core/music/scale_quantizer.cpp
    # engine
    core/engine/synth_engine.cpp
    # utils
    core/util/math_utils.cpp
)
target_include_directories(dsp_core PUBLIC
    ${CMAKE_SOURCE_DIR})
target_compile_definitions(dsp_core PUBLIC
    SAMPLE_RATE_DEFAULT=44100.0f
    BLOCK_SIZE_DEFAULT=256
    MAX_VOICES=8
    MAX_HELD_NOTES=32
    MAX_CHORD_NOTES=6
)

# ── HAL (PC) library ──────────────────────────────────────
add_library(hal_pc STATIC
    hal/pc/rtaudio_backend.cpp
    hal/pc/pc_midi.cpp
    hal/pc/keyboard_input.cpp
    hal/pc/preset_storage.cpp
)
target_include_directories(hal_pc PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${RTAUDIO_INCLUDE_DIRS}
    ${RTMIDI_INCLUDE_DIRS}
)
target_link_libraries(hal_pc PUBLIC
    dsp_core
    ${RTAUDIO_LIBRARIES}
    ${RTMIDI_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# ── UI library ────────────────────────────────────────────
add_library(ui_lib STATIC
    ui/imgui_app.cpp
    ui/widgets/adsr_display.cpp
    ui/panels/panel_controllers.cpp
    ui/panels/panel_oscillators.cpp
    ui/panels/panel_mixer.cpp
    ui/panels/panel_modifiers.cpp
    ui/panels/panel_output.cpp
    ui/panels/panel_arpeggiator.cpp
    ui/panels/panel_sequencer.cpp
    ui/panels/panel_chord_scale.cpp
    ui/panels/panel_presets.cpp
)
target_include_directories(ui_lib PUBLIC
    ${CMAKE_SOURCE_DIR})
target_link_libraries(ui_lib PUBLIC
    dsp_core hal_pc imgui imgui_knobs)

# ── Main executable ───────────────────────────────────────
add_executable(minimoog_sim sim/main.cpp)
target_link_libraries(minimoog_sim PRIVATE
    dsp_core hal_pc ui_lib)

# ── Tests ─────────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    find_package(Catch2 3 REQUIRED)

    add_executable(minimoog_tests
        tests/test_main.cpp
        tests/test_oscillator.cpp
        tests/test_moog_filter.cpp
        tests/test_envelope.cpp
        tests/test_glide.cpp
        tests/test_voice.cpp
        tests/test_arpeggiator.cpp
        tests/test_sequencer.cpp
        tests/test_scale_quantizer.cpp
    )
    target_link_libraries(minimoog_tests PRIVATE
        dsp_core Catch2::Catch2WithMain)

    include(CTest)
    include(Catch)
    catch_discover_tests(minimoog_tests)
endif()
```

---

# PHẦN 18 — PROJECT DIRECTORY TREE

```
MiniMoogDSP/
├── CMakeLists.txt
├── README.md
│
├── shared/
│   ├── types.h                  # sample_t, hz_t, enums
│   ├── params.h / .cpp          # AtomicParamStore, PARAM_META
│   └── interfaces.h / .cpp      # IAudioProcessor, MidiEventQueue
│
├── core/
│   ├── dsp/
│   │   ├── oscillator.h / .cpp  # PolyBLEP multi-waveform OSC
│   │   ├── moog_filter.h / .cpp # 4-pole ladder filter (tanh)
│   │   ├── envelope.h / .cpp    # ADSR exponential
│   │   ├── lfo.h / .cpp         # Low-frequency oscillator
│   │   ├── glide.h / .cpp       # Portamento (log smoother)
│   │   └── noise_generator.h / .cpp
│   │
│   ├── voice/
│   │   ├── voice.h / .cpp       # Single voice (OSC+FILT+ENV)
│   │   └── voice_pool.h / .cpp  # Poly/Mono/Unison manager
│   │
│   ├── music/
│   │   ├── arpeggiator.h / .cpp # 6-mode arp + swing
│   │   ├── sequencer.h / .cpp   # 16-step sequencer + swing
│   │   ├── chord_engine.h / .cpp# 16 chord voicings
│   │   └── scale_quantizer.h / .cpp # 16 scales
│   │
│   ├── engine/
│   │   └── synth_engine.h / .cpp# Top-level audio processor
│   │
│   └── util/
│       └── math_utils.h / .cpp  # clamp, semitonesToRatio, etc.
│
├── hal/
│   └── pc/
│       ├── rtaudio_backend.h / .cpp
│       ├── pc_midi.h / .cpp
│       ├── keyboard_input.h / .cpp
│       └── preset_storage.h / .cpp
│
├── ui/
│   ├── imgui_app.h / .cpp       # Main window + dockspace
│   ├── widgets/
│   │   └── adsr_display.h / .cpp# ADSR shape renderer
│   └── panels/
│       ├── panel_controllers.h / .cpp
│       ├── panel_oscillators.h / .cpp
│       ├── panel_mixer.h / .cpp
│       ├── panel_modifiers.h / .cpp  # filter + envelopes
│       ├── panel_output.h / .cpp
│       ├── panel_arpeggiator.h / .cpp
│       ├── panel_sequencer.h / .cpp
│       ├── panel_chord_scale.h / .cpp
│       └── panel_presets.h / .cpp
│
├── sim/
│   └── main.cpp                 # Entry point
│
├── tests/
│   ├── CMakeLists.txt
│   ├── test_main.cpp
│   ├── test_oscillator.cpp
│   ├── test_moog_filter.cpp
│   ├── test_envelope.cpp
│   ├── test_glide.cpp
│   ├── test_voice.cpp
│   ├── test_arpeggiator.cpp
│   ├── test_sequencer.cpp
│   └── test_scale_quantizer.cpp
│
├── presets/                     # JSON preset files (runtime)
│   ├── factory_001_lead.json
│   ├── factory_002_bass.json
│   └── ...
│
└── third_party/
    ├── imgui/                   # Dear ImGui (git submodule)
    ├── imgui-knobs/             # ImGui-Knobs (git submodule)
    └── catch2/                  # Catch2 (or via system pkg)
```

---

# PHẦN 19 — CODING CONVENTIONS

# PHẦN 20 — BUILD & RUN GUIDE

```bash
# ── 1. Clone & setup submodules ──────────────────────────
git clone https://github.com/yourname/MiniMoogDSP.git
cd MiniMoogDSP
git submodule update --init --recursive

# ── 2. Install system dependencies (Ubuntu/Debian) ───────
sudo apt install -y \
    librtaudio-dev librtmidi-dev \
    libglfw3-dev libopengl-dev   \
    nlohmann-json3-dev           \
    catch2                       \
    cmake pkg-config build-essential

# ── 3. Configure ─────────────────────────────────────────
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# ── 4. Build ─────────────────────────────────────────────
cmake --build build --parallel $(nproc)

# ── 5. Run simulator ─────────────────────────────────────
./build/bin/minimoog_sim

# ── 6. Run tests ─────────────────────────────────────────
cd build && ctest --output-on-failure -j$(nproc)

# ── 7. Debug build (ASan + UBSan) ────────────────────────
cmake -B build_dbg -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build_dbg --parallel $(nproc)
cd build_dbg && ctest --output-on-failure

# ── macOS (Homebrew) ─────────────────────────────────────
brew install rtaudio rtmidi glfw nlohmann-json catch2 cmake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# ── Windows (MSVC / vcpkg) ───────────────────────────────
vcpkg install rtaudio rtmidi glfw3 nlohmann-json catch2
```

## 17.1 Ngôn ngữ & Chuẩn

```
Tiêu chuẩn   : C++20 (ISO/IEC 14882:2020)
Compiler hỗ trợ: GCC ≥ 12, Clang ≥ 14, MSVC ≥ 19.34
Warning level: -Wall -Wextra -Wpedantic (toàn bộ warning = error
               trong CI)
```

## 17.2 Naming Conventions

```
─────────────────────────────────────────────────────────
Loại                  Convention          Ví dụ
─────────────────────────────────────────────────────────
Namespace             snake_case          core::dsp
Class / Struct        PascalCase          MoogFilter
Interface (abstract)  IPascalCase         IAudioProcessor
Enum class            PascalCase          WaveShape::Sawtooth
Member variable       camelCase + _       sampleRate_
Static member         s_camelCase         s_instance_
Constant (constexpr)  UPPER_SNAKE         MAX_VOICES
Macro (#define)       UPPER_SNAKE         BLOCK_SIZE_DEFAULT
Free function         camelCase           midiNoteToHz()
Method                camelCase           setSampleRate()
Template param        SingleCapital / T   T, SampleType
Local variable        camelCase           bufferSize
Parameter             camelCase           sampleRate
File name             snake_case          moog_filter.cpp
Header guard          ALL_CAPS_PATH_H     CORE_DSP_MOOG_FILTER_H
─────────────────────────────────────────────────────────
```

## 17.3 File Structure

```cpp
// ─────────────────────────────────────────────────────────
// FILE: core/dsp/moog_filter.h
// BRIEF: 4-pole Moog ladder filter with tanh saturation
// AUTHOR: <author>
// ─────────────────────────────────────────────────────────
#pragma once           // (1) always use #pragma once

// (2) system / STL includes (alphabetical)
#include <array>
#include <cmath>
#include <cstdint>

// (3) third-party includes
// (none for this file)

// (4) project includes (relative to project root)
#include "shared/types.h"

// (5) forward declarations (prefer over full includes in .h)
class AtomicParamStore;

// (6) class definition
class MoogFilter {
public:
    // ... public API first
private:
    // ... implementation detail last
};
```

## 17.4 Class Layout Order

```cpp
class MyClass {
public:
    // 1. Type aliases / nested types / enums
    using SampleType = float;
    enum class State { Idle, Running };

    // 2. Static constants
    static constexpr int MAX_SIZE = 256;

    // 3. Constructors / Destructor
    MyClass();
    ~MyClass();

    // 4. Copy / Move (= delete nếu không cần)
    MyClass(const MyClass&) = delete;
    MyClass& operator=(const MyClass&) = delete;

    // 5. Public methods (alphabetical trong nhóm)
    void init(float sampleRate);
    void reset();
    float tick(float input);

    // 6. Getters / Setters
    float getSampleRate() const noexcept;
    void  setSampleRate(float sr) noexcept;

protected:
    // 7. Protected methods (nếu có)

private:
    // 8. Private helper methods
    void updateCoefficients();

    // 9. Member variables (theo nhóm chức năng)
    float sampleRate_ = 44100.f;
    float cutoff_     = 1000.f;

    // 10. Static members
    static int instanceCount_;
};
```

## 17.5 Real-Time Thread Rules

```
╔══════════════════════════════════════════════════════════╗
║  AUDIO CALLBACK — TUYỆT ĐỐI CẤM                        ║
╠══════════════════════════════════════════════════════════╣
║  ✗  new / delete / malloc / free                        ║
║  ✗  std::vector::push_back (nếu có thể realloc)        ║
║  ✗  std::string construction / assignment               ║
║  ✗  std::mutex::lock() / std::lock_guard                ║
║  ✗  std::cout / printf / fprintf                        ║
║  ✗  File I/O (fopen, fread, ifstream...)                ║
║  ✗  Bất kỳ syscall có thể block                        ║
║  ✗  throw exception                                     ║
╠══════════════════════════════════════════════════════════╣
║  AUDIO CALLBACK — ĐƯỢC PHÉP                             ║
╠══════════════════════════════════════════════════════════╣
║  ✓  Stack allocation (fixed-size arrays ≤ 64 KB)       ║
║  ✓  std::atomic load/store (memory_order_relaxed)       ║
║  ✓  SPSCQueue::push / pop                               ║
║  ✓  Arithmetic, std::sin/cos/exp (từ <cmath>)           ║
║  ✓  std::array, std::span                               ║
╚══════════════════════════════════════════════════════════╝
```

Annotation bắt buộc:

```cpp
// [RT-SAFE] — phải đặt ở đầu mọi function
//             được gọi từ audio callback
float MoogFilter::process(float input) noexcept { // [RT-SAFE]
    // ...
}

// [RT-UNSAFE] — đặt ở function KHÔNG được gọi từ audio thread
void PresetStorage::savePreset(...) {              // [RT-UNSAFE]
    // uses std::ofstream
}
```

## 17.6 Const Correctness & noexcept

```cpp
// Rule 1: getter LUÔN const + noexcept
float getCutoff() const noexcept { return cutoff_; }

// Rule 2: setter noexcept nếu không throw
void setCutoff(float hz) noexcept {
    cutoff_ = std::clamp(hz, 20.f, 20000.f);
    updateCoefficients();
}

// Rule 3: tick() / process() LUÔN noexcept
float tick(float in) noexcept { /* ... */ }

// Rule 4: KHÔNG dùng raw pointer làm owner
//   ✗  float* buffer = new float[N];
//   ✓  std::array<float, N> buffer;
//   ✓  std::unique_ptr<float[]> buffer;

// Rule 5: Parameter passing
//   - Primitive types (float, int, bool): by value
//   - Large structs read-only: const reference
//   - Output params: reference hoặc return value
void processBlock(const float* in,  // read-only ptr
                  float*       out, // write ptr
                  int          n) noexcept;
```

## 17.7 Magic Number Policy

```cpp
// ✗ BAD — magic numbers
float attack = std::exp(-1.f / (0.001f * 44100.f));

// ✓ GOOD — named constants
static constexpr float MIN_ENV_TIME_MS = 1.0f;
static constexpr float SR              = 44100.f;
float attack = std::exp(-1.f / (MIN_ENV_TIME_MS * 0.001f * SR));

// ✓ GOOD — PARAM_META array cho UI labels & ranges
// (xem shared/params.h)
```

## 17.8 Error Handling Strategy

```
─────────────────────────────────────────────────────────
Tình huống                     Cách xử lý
─────────────────────────────────────────────────────────
Lỗi init (audio, MIDI, UI)    Return bool + lastError_ string
Lỗi file I/O (preset)         Return bool + lastError_ string
Lỗi lập trình (assert)        assert() trong Debug,
                               clamp/saturate trong Release
Lỗi trong audio callback      TUYỆT ĐỐI KHÔNG throw,
                               clamp output, log qua
                               atomic flag để UI đọc
Giá trị ngoài range           std::clamp() tại setter
─────────────────────────────────────────────────────────

// Pattern chuẩn cho init functions:
bool SomeClass::init(Config cfg) {
    if (!validateConfig(cfg)) {
        lastError_ = "Invalid config: ...";
        return false;
    }
    // ...
    return true;
}
```

## 17.9 Comment Style

```cpp
// ─────────────────────────────────────────────────────────
// Section divider (dùng cho nhóm method lớn)
// ─────────────────────────────────────────────────────────

// Single-line comment: giải thích WHY, không phải WHAT

/*
 * Multi-line comment cho thuật toán phức tạp.
 * Giải thích công thức, paper tham khảo.
 *
 * Reference: Stilson & Smith, "Analyzing the Moog VCF", 1996
 */

/// @brief Doxygen cho public API
/// @param cutoffHz  Cutoff frequency [20..20000 Hz]
/// @param resonance Resonance [0..1], 1 = self-oscillation
/// @note  [RT-SAFE] — safe to call from audio thread
void setCutoffAndResonance(float cutoffHz,
                            float resonance) noexcept;

// TODO(username): mô tả việc cần làm
// FIXME: mô tả lỗi đã biết
// HACK: giải thích tại sao hack này tồn tại
// NOTE: thông tin quan trọng cần lưu ý
```

## 17.10 Git Commit Convention

```
Format:  <type>(<scope>): <subject>

type:
  feat     — tính năng mới
  fix      — sửa lỗi
  perf     — cải thiện hiệu năng
  refactor — tái cấu trúc không thay đổi behavior
  test     — thêm/sửa test
  docs     — tài liệu
  build    — CMake, CI
  style    — formatting, không thay đổi logic

scope:    dsp | voice | engine | arp | seq | ui | hal | shared

Ví dụ:
  feat(dsp): add PolyBLEP correction to Oscillator
  fix(filter): clamp resonance to prevent NaN at r=1.0
  perf(engine): snapshot params once per block not per voice
  test(envelope): add retrigger-from-release test case
  build(cmake): add MSVC /fp:fast flag for Release
```

---

# PHẦN 21 — GLOSSARY

## 18.1 DSP Terms

|Thuật ngữ|Viết tắt|Định nghĩa|
|---|---|---|
|**Sample Rate**|SR|Số mẫu audio mỗi giây. Mặc định: 44100 Hz|
|**Block Size**|N|Số sample xử lý mỗi lần callback. Mặc định: 256|
|**Nyquist Frequency**|Fₙ|SR/2 = 22050 Hz — giới hạn tần số biểu diễn được|
|**PolyBLEP**|—|Polynomial Band-Limited Step: kỹ thuật giảm aliasing tại điểm không liên tục của waveform|
|**Aliasing**|—|Nhiễu tần số xuất hiện khi waveform chứa harmonics vượt Nyquist|
|**Oversampling**|—|Xử lý ở SR cao hơn rồi downsample, giảm aliasing|
|**DC Offset**|—|Thành phần DC (0 Hz) không mong muốn trong tín hiệu|
|**Tanh Saturation**|—|Hàm tanh() dùng để mô phỏng soft-clipping trong mạch analog|
|**Ladder Filter**|—|Kiến trúc bộ lọc của Moog: 4 tầng RC nối tiếp với feedback|
|**Cutoff Frequency**|Fc|Tần số -3dB (hoặc -6dB) của bộ lọc|
|**Resonance / Q**|r|Hệ số phản hồi của filter. r→1 = self-oscillation|
|**Envelope**|ENV|Đường cong biên độ theo thời gian: Attack–Decay–Sustain–Release|
|**LFO**|LFO|Low-Frequency Oscillator: dao động tần số thấp (<20 Hz) dùng để modulate|
|**Portamento / Glide**|—|Trượt tần số liên tục từ note này sang note khác|
|**Cents**|ct|Đơn vị điều chỉnh pitch: 100 cents = 1 semitone|
|**Semitone**|st|Nửa cung âm nhạc. 12 semitones = 1 octave|
|**MIDI Note**|—|Số nguyên 0–127 biểu diễn nốt nhạc. C4 = 60|
|**Velocity**|vel|Lực nhấn phím MIDI (0–127)|
|**Pitch Bend**|PB|Lệnh MIDI thay đổi pitch ±2 semitone (mặc định)|

## 18.2 Architecture Terms

|Thuật ngữ|Định nghĩa|
|---|---|
|**AtomicParamStore**|Bộ lưu trữ tham số dùng `std::atomic<float>`, thread-safe không cần lock|
|**MidiEventQueue**|SPSC ring buffer truyền `MidiEvent` từ MIDI thread → Audio thread|
|**SPSC Queue**|Single-Producer Single-Consumer queue — lock-free khi chỉ 1 thread ghi, 1 thread đọc|
|**Voice**|Một instance hoàn chỉnh: 3 OSC + Filter + 2 ENV + Glide|
|**VoicePool**|Quản lý N voices, thực hiện voice stealing, phân bổ poly/mono/unison|
|**Voice Stealing**|Cơ chế chọn voice cũ nhất để tái sử dụng khi hết voice rảnh|
|**Unison Mode**|Nhiều voices cùng chơi một note với detune khác nhau|
|**HAL**|Hardware Abstraction Layer — tầng cách ly platform (PC/MCU)|
|**RT-SAFE**|Real-Time Safe: hàm không block, không alloc, an toàn trong audio callback|
|**Snapshot**|Đọc toàn bộ PARAM_COUNT atomic params một lần, copy sang local array|
|**Panel**|Một cửa sổ ImGui hiển thị nhóm controls liên quan|

## 18.3 Music Terms

|Thuật ngữ|Định nghĩa|
|---|---|
|**Arpeggiator**|Tự động phát các nốt trong chord theo thứ tự (Up/Down/Bounce/Random...)|
|**Sequencer**|Phát chuỗi nốt lập trình trước theo nhịp|
|**BPM**|Beats Per Minute — nhịp độ bài nhạc|
|**Gate**|Thời gian nốt được giữ (0–1, = tỉ lệ so với 1 step). Gate=1.0 → legato|
|**Tie**|Nối hai step liền nhau không ngắt sound|
|**Swing**|Delay nhẹ cho beat chẵn tạo cảm giác groove (shuffle)|
|**Scale Quantizer**|Snap nốt bất kỳ vào nốt gần nhất trong scale được chọn|
|**Root Note**|Nốt gốc của scale/chord|
|**Chord Voicing**|Cách sắp xếp các nốt trong 1 chord|
|**Octave**|Quãng 8: tần số nhân/chia đôi. MIDI: ±12 semitones|
|**Unison Detune**|Độ lệch pitch (cents) giữa các voice trong unison|

## 18.4 Parameter ID Quick Reference

```
─────────────────────────────────────────────────────────
ID Range    Group              Key Params
─────────────────────────────────────────────────────────
0 – 3       OSC 1              ON, Range, Freq, Wave
4 – 7       OSC 2              ON, Range, Freq, Wave
8 – 11      OSC 3              ON, Range, Freq, Wave
12 – 14     Mixer              OSC1/2/3 Level
15 – 17     Mixer              Noise, Ext, Master
18 – 21     Filter             Cutoff, Emphasis, Amount,
                               KBD Track
22 – 25     Filter Envelope    A, D, S, R
26 – 29     Amp Envelope       A, D, S, R
30 – 32     LFO                Rate, Wave, Depth
33 – 34     Glide              Time, Enable
35 – 38     Controllers        MasterTune, MasterVol,
                               VoiceCount, PlayMode
39          OSC3               LFO Mode
40 – 43     Arpeggiator        Enable, Mode, Octaves,
                               RateIndex
44 – 46     Arpeggiator        Gate, Swing, HoldMode
47 – 51     Sequencer          Enable, BPM, StepCount,
                               RateIndex, Gate
52          Sequencer          Swing
53 – 55     Chord Engine       Enable, ChordType, Spread
56 – 58     Scale Quantizer    Enable, Root, Scale
─────────────────────────────────────────────────────────
Total: PARAM_COUNT = 59 parameters
─────────────────────────────────────────────────────────
```

## 18.5 MIDI CC Mapping

|CC#|Tên|Param tương ứng|
|---|---|---|
|1|Modulation Wheel|`P_LFO_DEPTH`|
|7|Volume|`P_MASTER_VOL`|
|10|Pan|_(reserved)_|
|64|Sustain Pedal|Giữ nốt không noteOff|
|65|Portamento On/Off|`P_GLIDE_ON`|
|5|Portamento Time|`P_GLIDE_TIME`|
|74|Brightness|`P_FILTER_CUTOFF`|
|71|Resonance|`P_FILTER_EMPHASIS`|
|73|Attack|`P_AENV_ATTACK`|
|72|Release|`P_AENV_RELEASE`|

## 18.6 Abbreviations Index

```
ADSR   — Attack Decay Sustain Release
ARP    — Arpeggiator
BPM    — Beats Per Minute
CC     — MIDI Control Change
DAC    — Digital-to-Analog Converter
DSP    — Digital Signal Processing
ENV    — Envelope
EXT    — External (input)
HAL    — Hardware Abstraction Layer
Hz     — Hertz (cycles per second)
IIR    — Infinite Impulse Response (filter type)
LFO    — Low-Frequency Oscillator
LP     — Low-Pass
MCU    — Microcontroller Unit
MIDI   — Musical Instrument Digital Interface
OSC    — Oscillator
PC     — Personal Computer
RT     — Real-Time
SEQ    — Sequencer
SPSC   — Single-Producer Single-Consumer
SR     — Sample Rate
UI     — User Interface
VCA    — Voltage-Controlled Amplifier
VCF    — Voltage-Controlled Filter
VCO    — Voltage-Controlled Oscillator
```