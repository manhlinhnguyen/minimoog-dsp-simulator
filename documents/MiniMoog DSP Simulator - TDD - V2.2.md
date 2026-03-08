# MiniMoog DSP Simulator

# Technical Design Document — V2.2

**Document version:** 2.2.0
**Date:** 2026-03-09
**Target platform:** PC (Windows / macOS / Linux)
**Language:** C++17
**Status:** Implemented & Verified
**Tài liệu tham chiếu:** TDD V2.1 (baseline), DSP_Engine_Analysis_v1.md, DSP_Engine_Analysis_v2.md, Engine_Improvements_Log.md

---

# MỤC LỤC

| # | Tên phần | Trạng thái |
|---|----------|------------|
| 1 | TỔNG QUAN V2.2 — NHỮNG GÌ THAY ĐỔI SO VỚI V2.1 | ✅ |
| 2 | KIẾN TRÚC TỔNG THỂ (V2.2) | ✅ |
| 3 | MULTI-ENGINE ARCHITECTURE | ✅ |
| 4 | ENGINE MANAGER | ✅ |
| 5 | SÁU DSP ENGINES — CHI TIẾT | ✅ |
| 6 | MIDI FILE PLAYER | ✅ |
| 7 | DSP ENGINE IMPROVEMENTS (P1 → P3) | ✅ |
| 8 | HAL LAYER — FILE MỚI | ✅ |
| 9 | UI SYSTEM — CẤU TRÚC MỚI | ✅ |
| 10 | FACTORY ASSETS V2.2 | ✅ |
| 11 | THREADING MODEL (V2.2) | ✅ |
| 12 | CẤU TRÚC THƯ MỤC V2.2 | ✅ |
| 13 | BUG FIXES | ✅ |
| 14 | PERFORMANCE ANALYSIS | ✅ |
| 15 | TEST PLAN (V2.2) | ✅ |
| 16 | ROADMAP V3 (CẬP NHẬT) | ✅ |

---

# PHẦN 1 — TỔNG QUAN V2.2: NHỮNG GÌ THAY ĐỔI SO VỚI V2.1

## 1.1 Mục Tiêu V2.2

V2.2 là bước nhảy lớn nhất kể từ V1. Thay vì một Minimoog đơn lẻ, simulator giờ là một **multi-engine workstation** với 6 DSP engines chia sẻ một music layer và một effect chain. Đồng thời toàn bộ chất lượng DSP của từng engine được nâng cấp đáng kể qua 3 giai đoạn cải thiện (P1 → P2 → P3).

**Ba hướng chính của V2.2:**

1. **Multi-engine architecture** — 6 engines hoạt động song song, chuyển đổi real-time không mất âm thanh.
2. **DSP quality uplift** — Tất cả 21 item trong roadmap P1/P2/P3 từ `DSP_Engine_Analysis_v1.md` đã được triển khai hoàn chỉnh.
3. **Platform expansion** — MIDI file player, thêm analysis panels (spectrum, spectrogram, lissajous, VU meter, correlation), global presets engine+effects.

## 1.2 Tính Năng Mới V2.2

```
MULTI-ENGINE:
  ✅ IEngine interface — abstraction chung cho tất cả engines
  ✅ EngineManager — orchestrator: music layer + engine selection + MIDI routing
  ✅ MoogEngine — Moog Model D (tách từ core/engine/ → core/engines/moog/)
  ✅ HammondB3Engine — 9-drawbar tonewheel additive synthesis, 61 voices
  ✅ RhodesEngine — 4-mode biquad modal resonator physical model, 12 voices
  ✅ DX7Engine — 6-op FM, 32 algorithms, 8 voices
  ✅ MellotronEngine — multi-cycle wavetable tape emulation, 35 voices
  ✅ DrumEngine — hybrid DSP (8 pads) + WAV sample (8 pads)

MIDI FILE PLAYER:
  ✅ MidiFilePlayer — load/parse Standard MIDI Format (SMF) Type 0/1
  ✅ Transport controls — Play, Pause, Stop, Seek
  ✅ Real-time tempo sync — BPM từ MIDI file → Arp/Seq
  ✅ 10 factory MIDI files (classical repertoire, public domain)

DSP IMPROVEMENTS (P1):
  ✅ Pitch bend hoạt động đúng trên Hammond, Rhodes, DX7, Mellotron
  ✅ DX7 voice stealing: round-robin → oldest-active
  ✅ Hammond drawbar ParamSmoother (9×5ms)
  ✅ Drums hi-hat choke group (open ↔ close mutual kill)
  ✅ Drums velocity → decay scaling

DSP IMPROVEMENTS (P2):
  ✅ Hammond Leslie dual-rotor (Doppler + AM + crossover)
  ✅ DX7 exponential ADSR envelopes
  ✅ DX7 fixed-frequency operators (Hz mode, 12 params)
  ✅ Mellotron tape wow/flutter (dual LFO, 4 params)
  ✅ Mellotron tape hiss floor (−50 dBFS, voice-scaled)
  ✅ Rhodes vibrato LFO (±35 cents, change-detect gate)
  ✅ Rhodes CC64 sustain pedal (deferred noteOff)
  ✅ Rhodes velocity → pickup nonlinearity (4× dynamic timbre range)
  ✅ Rhodes ParamSmoother Tone/Decay/Drive (3 smoothers)
  ✅ Rhodes per-mode excitation scaling (HAMMER_EXCITE[])
  ✅ Rhodes modal amplitude calibration (Tolonen 1998)
  ✅ Rhodes sympathetic resonance (interval-based energy injection)
  ✅ Moog post-VCA output limiter (tanh soft clip)
  ✅ DX7 keyboard rate scaling (6 params, per-operator)

DSP IMPROVEMENTS (P3):
  ✅ Moog mono note priority (Last/Lowest/Highest)
  ✅ Moog analog-style unison detune (±20% LCG jitter, deterministic)
  ✅ Rhodes extended polyphony 8→12 (POLY_NORM = 1/√12)
  ✅ Hammond tube overdrive pre-Leslie (tanh, 1×–5×, ParamSmoother)
  ✅ Drums 808-style hi-hat ring modulation (6-osc RM, 3 pairs)
  ✅ Drums kick sweep param exposure (depth + time)
  ✅ Mellotron multi-cycle wavetable (8 cycles×2048 = 16384, harmonic jitter ±8–12%)

HAL:
  ✅ engine_preset_storage — per-engine JSON preset load/save
  ✅ global_preset_storage — engine + effects combined preset
  ✅ midi_file_loader.h — SMF binary parser (HAL boundary)
  ✅ drum_sample_loader.h — WAV loader for DrumEngine sample pads

UI:
  ✅ ui/panels/ refactored vào 3 subfolder: engines/, controls/, analysis/
  ✅ panel_engine_selector — RadioButton chọn engine + per-engine sub-panel
  ✅ panel_moog (trong engines/) — Moog controls thuần túy
  ✅ panel_hammond, panel_rhodes, panel_dx7, panel_mellotron, panel_drums
  ✅ panel_midi_player — transport + file browser (trong controls/)
  ✅ panel_presets — Engine tab + Effect tab + Global tab (3 tab, Load phải dưới list)
  ✅ panel_spectrum, panel_spectrogram, panel_lissajous, panel_vumeter, panel_correlation (analysis/)

ASSETS:
  ✅ assets/hammond_presets/ — 10 factory presets
  ✅ assets/rhodes_presets/ — 10 factory presets
  ✅ assets/dx7_presets/ — 10 factory presets
  ✅ assets/mellotron_presets/ — 10 factory presets
  ✅ assets/global_presets/ — 35 cross-engine global presets
  ✅ assets/midi/ — 10 classical MIDI files (public domain)
  ✅ assets/moog_presets/ — 10 presets mới (thay thế 20 cũ, cập nhật schema)
  ✅ assets/effect_presets/ — mở rộng từ 10 → 20 presets
  ✅ assets/sequencer_patterns/ — mở rộng từ 10 → 20 patterns
```

## 1.3 Những Gì KHÔNG Thay Đổi Từ V2.1

- `core/dsp/` — Oscillator, MoogFilter, Envelope, LFO, Glide, Noise, ParamSmoother
- `core/effects/` — EffectChain + 8 effect types (chỉ minor update: BpmSync context)
- `core/util/` — SPSCQueue, math_utils
- MIDI CC/CC thread model — RtMidi → MidiEventQueue → EngineManager drain
- Build system cơ bản — FetchContent dependencies, static link, no DLLs
- 44 unit tests — tất cả vẫn pass

---

# PHẦN 2 — KIẾN TRÚC TỔNG THỂ (V2.2)

## 2.1 Layer Diagram

```
╔══════════════════════════════════════════════════════════════════════╗
║                        APPLICATION LAYER                              ║
║  sim/main.cpp                                                         ║
║  • Khởi tạo: EngineManager, EffectChain, RtAudioBackend, RtMidi      ║
║  • GLFW event loop → ImGuiApp::render()                               ║
╠══════════════════════════════════════════════════════════════════════╣
║                          UI LAYER                                      ║
║  ui/imgui_app.{h,cpp}          — top-level render + menu bar          ║
║  ui/panels/engines/                                                    ║
║    panel_engine_selector.*     ← RadioButton + per-engine sub-panel   ║
║    panel_moog.*                ← Moog controls                        ║
║    panel_hammond.*             ← 9 drawbars, Leslie, overdrive        ║
║    panel_rhodes.*              ← Physical model params                ║
║    panel_dx7.*                 ← 6-op table, algorithms               ║
║    panel_mellotron.*           ← Tape selector, wow/flutter           ║
║    panel_drums.*               ← 16-pad grid                          ║
║  ui/panels/controls/                                                   ║
║    panel_effects.*             ← Effect chain editor                  ║
║    panel_music.*               ← Arp + Chord + Scale + Seq + Keyboard ║
║    panel_midi_player.*         ← MIDI file transport                  ║
║    panel_output.*              ← Master Volume                        ║
║    panel_presets.*             ← Engine + Effect + Global tabs        ║
║  ui/panels/analysis/                                                   ║
║    panel_oscilloscope.*        ← Triggered waveform                   ║
║    panel_spectrum.*            ← Real-time FFT spectrum               ║
║    panel_spectrogram.*         ← Scrolling time-frequency             ║
║    panel_lissajous.*           ← L vs R Lissajous                    ║
║    panel_vumeter.*             ← Peak + RMS VU meters                 ║
║    panel_correlation.*         ← L/R cross-correlation               ║
║  ui/widgets/                   ← knob, ADSR, piano, seq grid          ║
╠══════════════════════════════════════════════════════════════════════╣
║                        PC HAL LAYER                                    ║
║  hal/pc/rtaudio_backend.*      ← RtAudio callback                    ║
║  hal/pc/pc_midi.*              ← RtMidi input/output                  ║
║  hal/pc/keyboard_input.*       ← GLFW → MIDI noteOn/Off              ║
║  hal/pc/engine_preset_storage.*← per-engine JSON I/O                 ║
║  hal/pc/global_preset_storage.*← engine+effects combined JSON I/O    ║
║  hal/pc/moog_preset_storage.*  ← legacy Moog preset (compat)         ║
║  hal/pc/effect_preset_storage.*← effect chain JSON I/O               ║
║  hal/pc/midi_file_loader.h     ← SMF binary parser (HAL boundary)    ║
║  hal/pc/drum_sample_loader.h   ← WAV loader for DrumEngine           ║
╠══════════════════════════════════════════════════════════════════════╣
║                       SHARED LAYER                                     ║
║  shared/types.h     shared/params.h     shared/interfaces.h           ║
║  (AtomicParamStore, MidiEventQueue, IEngine interface base types)      ║
╠══════════════════════════════════════════════════════════════════════╣
║                      DSP CORE LAYER                                    ║
║  core/dsp/        — Oscillator, MoogFilter, Envelope, LFO, Glide     ║
║  core/music/      — Arp, Seq, Chord, Scale, MidiFilePlayer            ║
║  core/engines/    — IEngine + EngineManager + 6 engine implementations║
║    ├── moog/      — MoogEngine, Voice, VoicePool                      ║
║    ├── hammond/   — HammondB3Engine, HammondVoice                     ║
║    ├── rhodes/    — RhodesEngine, RhodesVoice                         ║
║    ├── dx7/       — DX7Engine, DX7Voice, DX7Algorithms                ║
║    ├── mellotron/ — MellotronEngine, MellotronVoice, MellotronTables  ║
║    └── drums/     — DrumEngine, DrumPadDsp, DrumPadSample             ║
║  core/effects/    — EffectChain + 8 effect types                      ║
║  core/util/       — SPSCQueue, math_utils                             ║
╚══════════════════════════════════════════════════════════════════════╝
```

## 2.2 Signal Path (V2.2)

```
MIDI FILE (in memory)
  ↓  MidiFilePlayer::tick() — 1 lần/block, stack-alloc BlockEvents (max 64)
  │  [KHÔNG qua MidiEventQueue — audio thread là cả producer và consumer]
  ▼
MIDI EVENTS ──► Scale Quantizer ──► Arpeggiator ──► Chord Engine
                                                         │
                                                    per-note noteOn
                                                         ▼
                                               ACTIVE ENGINE (1 trong 6)
  ┌────────────────────────────────────────────────────────────────────┐
  │  Moog:      OSC1/2/3 → Mixer → 4-pole Ladder Filter → VCA → tanh  │
  │  Hammond:   9-drawbar tonewheel additive → Overdrive → Leslie      │
  │  Rhodes:    Hammer excite → 4 modal biquads → Pickup poly → Sat    │
  │  DX7:       6-op FM (32 algorithms) → exp-ADSR → carrier sum       │
  │  Mellotron: 8-cycle wavetable (wow/flutter LFO) → tape hiss        │
  │  Drums:     8 DSP pads (RM/sweep) + 8 WAV sample pads             │
  └────────────────────────────────────────────────────────────────────┘
                                         │
                                   Master Volume (P_MASTER_VOL)
                                         │
                                   EFFECT CHAIN (serial, ≤16 slots)
                                   Gain → Chorus → Flanger → Phaser →
                                   Tremolo → Delay → Reverb → EQ
                                         │
                            ┌────────────┴────────────┐
                            ▼                         ▼
                       outL[], outR[]          Oscilloscope ring buf
                            │                         │
                      RtAudio DAC              Analysis panels
```

## 2.3 Data Flow — Multi-Thread (V2.2)

```
UI Thread                 Audio Thread              MIDI Thread (RtMidi)
─────────                 ────────────              ────────────────────
User interaction          RtAudio callback          Hardware MIDI event
     │                         │                          │
     ▼                         ▼                          ▼
engine->setParam()        EngineManager::          MidiEventQueue.push()
(atomic store)            processBlock()
                               │
effect->setSlotParam()         ├─ snapshotGlobal()    ← atomic loads
(atomic store → direct         │
 effect->setParam())           ├─ midiPlayer_.tick()  ← parse + dispatch
                               │    [no SPSC, stack-alloc]
mgr.switchEngine()             │
(allSoundOff old →             ├─ drain MidiEventQueue ← SPSC pop
 atomic update activeIdx_)     │
                               ├─ Music layer tick (Arp/Seq)
                               │
preset_storage.load()          ├─ eng->beginBlock()    ← param snapshot
→ engine->allNotesOff()        │
→ engine->setParam() (×N)      ├─ for i in nFrames:
                               │    eng->tickSample(L, R)
                               │
                               ├─ effectChain.processBlock()
                               │
                               └─ outL/outR → DAC
```

---

# PHẦN 3 — MULTI-ENGINE ARCHITECTURE

## 3.1 IEngine Interface

`core/engines/iengine.h` — contract chung cho tất cả 6 engines.

```cpp
class IEngine {
public:
    virtual ~IEngine() = default;

    // Lifecycle
    virtual void init(float sampleRate) noexcept = 0;
    virtual void beginBlock(int nFrames) noexcept {}  // param snapshot
    virtual void tickSample(float& outL, float& outR) noexcept = 0;

    // Note events
    virtual void noteOn(int note, int vel) noexcept = 0;
    virtual void noteOff(int note) noexcept = 0;
    virtual void allNotesOff() noexcept = 0;
    virtual void allSoundOff() noexcept = 0;

    // MIDI
    virtual void pitchBend(int bend) noexcept = 0;     // -8192..+8191
    virtual void controlChange(int cc, int val) noexcept = 0;

    // Params
    virtual void  setParam(int id, float v) noexcept = 0;
    virtual float getParam(int id) const noexcept = 0;
    virtual float getParamDefault(int id) const noexcept = 0;
    virtual int   getParamCount() const noexcept = 0;
    virtual const char* getParamName(int id) const noexcept = 0;
    virtual float getParamMin(int id) const noexcept = 0;
    virtual float getParamMax(int id) const noexcept = 0;

    // Identity
    virtual const char* getName() const noexcept = 0;
    virtual int getActiveVoices() const noexcept = 0;
};
```

**Mọi engine đều dùng pattern `beginBlock()` param snapshot:**
- `params_[id]` — `std::atomic<float>`, UI thread ghi
- `paramCache_[id]` — plain `float`, copy từ atomic ở đầu block
- `tickSample()` chỉ đọc `paramCache_` — không atomic trong hot path

## 3.2 Engine Param Stores

| Engine | Param enum | Count | File |
|--------|-----------|-------|------|
| Moog | `MoogParam` (MP_*) | 42 | `moog_params.h` |
| Hammond | `HammondParam` (HP_*) | 22 | `hammond_engine.h` |
| Rhodes | `RhodesParam` (RP_*) | 11 | `rhodes_engine.h` |
| DX7 | `DX7Param` (DX7P_*) | 63 | `dx7_engine.h` |
| Mellotron | `MellotronParam` (MP_*) | 11 | `mellotron_engine.h` |
| Drums | `DrumParam` (DRUM_*) | 67 | `drum_engine.h` |

## 3.3 Engine Switching — Wait-Free

```cpp
// UI thread:
void EngineManager::switchEngine(int newIdx) noexcept {
    int old = activeIdx_.load();
    if (old == newIdx) return;
    engines_[old]->allSoundOff();       // Dừng mọi âm thanh engine cũ
    activeIdx_.store(newIdx);           // Atomic write — audio thread sees immediately
}

// Audio thread (beginBlock):
IEngine* eng = engines_[activeIdx_.load(std::memory_order_acquire)].get();
```

Chuyển engine: không có latency, không có pop vì `allSoundOff()` đã silence engine cũ trước khi switch.

---

# PHẦN 4 — ENGINE MANAGER

`core/engines/engine_manager.{h,cpp}`

## 4.1 Trách Nhiệm

EngineManager là **orchestrator duy nhất** của audio thread:

```
EngineManager::processBlock(outL[], outR[], nFrames)
  │
  ├─ snapshotGlobal()          — load master BPM, scale, arp, seq params
  │
  ├─ midiPlayer_.tick()        — generate MIDI events from MIDI file
  │    └─ routeNoteOn/Off/CC() — route qua scale→arp→chord→engine
  │
  ├─ drain MidiEventQueue      — hardware MIDI / keyboard input
  │    └─ same routing path
  │
  ├─ syncMusicLayer()          — BPM/swing/mode từ AtomicParamStore
  │
  ├─ eng->beginBlock(nFrames)  — param snapshot của active engine
  │
  └─ for i in nFrames:
       ├─ arp_.tick()          — advance arp phase → noteOn/Off nếu cần
       ├─ seq_.tick()          — advance seq step → noteOn/Off nếu cần
       └─ eng->tickSample(outL[i], outR[i])
                               — tổng hợp 1 sample từ active engine
  │
  effectChain_->processBlock(outL, outR, nFrames, fxCtx)
  fxCtx.bpm = currentBpm_     — DelayEffect BpmSync dùng BPM thực tế
```

## 4.2 Music Layer — Shared Across All Engines

Music layer (Arp/Seq/Chord/Scale) được EngineManager quản lý và apply TRƯỚC khi gửi event xuống engine. Tất cả 6 engines được hưởng lợi từ music layer tự động mà không cần implement riêng.

```
External MIDI noteOn(C4)
  ↓ Scale Quantizer (nếu enabled): C4 → C4 (hoặc snap to scale degree)
  ↓ Chord Engine (nếu enabled): C4 → {C4, E4, G4}
  ↓ Arpeggiator (nếu enabled): queue notes, phát từng note theo timing
  ↓ Active Engine: noteOn(C4, vel) / noteOn(E4, vel) / noteOn(G4, vel)
```

## 4.3 Oscilloscope Capture

Sau `tickSample()` mỗi sample, EngineManager ghi vào oscilloscope ring buffer (2048 samples, stereo):
```cpp
oscBuf_[oscWritePos_] = {outL[i], outR[i}};  // post-engine, pre-effect
oscWritePos_ = (oscWritePos_ + 1) & (OSC_BUF_SIZE - 1);
```
Analysis panels đọc buffer này từ UI thread (no lock — single producer, single consumer bởi convention).

---

# PHẦN 5 — SÁU DSP ENGINES — CHI TIẾT

## 5.1 MoogEngine — Subtractive

**Files:** `core/engines/moog/`

| Item | Spec |
|------|------|
| Voice model | Analog VCO → Mixer → 4-pole Ladder Filter → VCA |
| Polyphony | 1 (Mono) / 1–8 (Poly) / 1–8 (Unison) |
| Oscillators | 3× phase-accumulator (tri/saw/square/pulse/rev-saw) |
| Filter | Moog ladder 4-pole tanh saturation, ADSR + KbdTrack + ModMatrix |
| Voice stealing | 3 strategies: Oldest-Active / Lowest / Quietest |
| Note priority | 3 modes (Mono): Last / Lowest / Highest |
| Unison detune | Linear spread ±20% LCG jitter (deterministic per note) |
| Post-VCA limiter | tanh(x×1.25)/1.25 — soft clip khi unison stack |
| Params | 42 |

**Signal chain:**
```
OSC1(±7st, 6 waves) ─┐
OSC2(±7st, 6 waves)  ├→ MIXER → 4-pole Ladder Filter → VCA → tanh limiter
OSC3(audio or LFO)   ┘   ▲              ▲                ▲
NOISE(white/pink)         │      KbdTrack + FilterEnv    AmpEnv
                    ModMix blend (OSC3↔Noise)
                          ↓
                   OSC pitch + Filter cutoff mod
```

## 5.2 HammondB3Engine — Additive Tonewheel

**Files:** `core/engines/hammond/`

| Item | Spec |
|------|------|
| Voice model | 9-drawbar additive sine per key |
| Polyphony | 61 voices (full keyboard) |
| Drawbar smoothing | 9 × ParamSmoother 5ms — no zipper |
| Percussion | First-note, 2nd/3rd harmonic, fast/slow decay |
| Vibrato/Chorus | V1/V2/V3/C1/C2/C3 — LFO phase modulation |
| Leslie rotary | Dual-rotor (horn 0.8/6.2 Hz, drum 0.67/5.6 Hz) |
|               | Doppler delay + AM shading + crossover ~700 Hz |
|               | Mechanical inertia spin-up/spin-down |
| Tube overdrive | tanh pre-Leslie, drive 1×–5×, ParamSmoother |
| Pitch bend | ±2 semitones, all 9 harmonics |
| Params | 22 |

**Signal chain:**
```
61 voices × (9 harmonic sines × drawbar level × pitchBendFactor)
  + key-click noise burst (2ms per noteOn)
  + percussion (1st-note, decay envelope)
  ↓ [Tube Overdrive: tanh(x·drive)/drive, smoothed]
  ↓ [Leslie: mono → crossover → hornDelay + drumDelay → stereo AM]
  → outL, outR
```

## 5.3 RhodesEngine — Modal Physical Model

**Files:** `core/engines/rhodes/`

| Item | Spec |
|------|------|
| Voice model | Biquad modal resonator (Tolonen 1998) |
| Polyphony | 12 voices (extended từ 8) |
| Modal resonators | 4 modes: 1.000×, 2.045×, 5.979×, 9.214× (inharmonic) |
| Torsional mode | 6 cents sharp relative to bell partial → AM wobble |
| Pickup | Displacement-based polynomial: y = x + α·x² + β·x³ |
| Pickup range | α: 0.04 (pp/dark) → 0.16 (ff/bright) — velocity + tone |
| Hammer excitation | Per-mode scaling (1.00 / 0.85 / 0.50 / 0.25) |
| Amplitudes | Calibrated vs Tolonen 1998 (1.000 / 0.280 / 0.190 / 0.070) |
| Release | 2-stage: 30ms felt contact → user-defined releaseMs |
| Sustain pedal | CC64 deferred noteOff (noteHeld_[128] array) |
| Vibrato LFO | ±35 cents max, change-detect gate, pop-free via updateFreq() |
| Sympathetic resonance | Interval-based energy injection (unison: 0.015, 5th: 0.010, 4th: 0.008) |
| ParamSmoothers | 3: Tone / Decay / Drive (5ms each) |
| Cabinet EQ | +2dB@200Hz, −8dB@8kHz (biquad shelf) |
| DC blocker | Per-channel (L/R independent state) |
| Params | 11 |

**Pitch bend — pop-free với modal resonators:**
```cpp
// KHÔNG reset state y0/y1 — bảo toàn amplitude
void ModalMode::updateFreq(float freqHz, float sr) noexcept {
    coef = 2.0f * std::cos(TWO_PI * freqHz / sr);
    // y0, y1 không thay đổi → energy conserved
}
// Vibrato + pitchBend cộng hưởng qua updatePitchFactor()
```

## 5.4 DX7Engine — 6-Op FM

**Files:** `core/engines/dx7/`

| Item | Spec |
|------|------|
| Voice model | 6-operator phase modulation FM |
| Algorithms | 32 (all original DX7 algorithms) |
| Polyphony | 8 voices, oldest-active stealing |
| Envelopes | Exponential ADSR per operator (approach-to-target) |
| Fixed-freq mode | Per-operator: ratio mode OR fixed Hz (20–8000 Hz) |
| Fixed-freq + pitch bend | Fixed operators bypass pitch bend (hardware behavior) |
| Keyboard rate scaling | Per-operator kbdFactor = (1+kbdRate×3)^((note-60)/12) |
| Velocity sensitivity | Per-operator |
| Feedback | Op0 self-feedback |
| Params | 63 (+12 fixed-mode, +6 KRS) |

**Exponential envelope — DX7 character:**
```cpp
case Stage::Attack:
    envVal = 1.0f - (1.0f - envVal) * attackRate;  // fast rise, slow top
    break;
case Stage::Decay:
    envVal = sustainLvl + (envVal - sustainLvl) * decayRate;  // plunge + long tail
    break;
case Stage::Release:
    envVal *= releaseRate;  // natural fade
    break;
```

## 5.5 MellotronEngine — Multi-Cycle Wavetable

**Files:** `core/engines/mellotron/`

| Item | Spec |
|------|------|
| Voice model | Wavetable playback với tape emulation |
| Polyphony | 35 voices (full M400 keyboard C2–C5) |
| Wavetable | 4 tapes × 16384 samples (8 cycles × 2048) |
| Per-cycle jitter | Harmonic amplitude ±8–12%, phase drift 0–0.05 rad |
| Wow LFO | Engine-level, 0.1–3.0 Hz, 0–2.0% speed mod |
| Flutter LFO | Engine-level, 6–18 Hz, 0–0.6% speed mod |
| Tape hiss | LCG noise −55dBFS + 0.00015/voice (max −43dBFS at 35 voices) |
| Tape runout | Amplitude fade after runoutTimeSec (2–12s param) |
| Pitch spread | LCG per-note — each key has unique detuning |
| Pitch bend | ±2 semitones (phaseInc × pitchBendCache_) |
| Params | 11 (+4 wow/flutter) |

**Shared LFO rationale:** M400 có 1 capstan drive → tất cả tape strips bị wow/flutter cùng nhau → engine-level LFO là đúng vật lý.

## 5.6 DrumEngine — Hybrid DSP + Sample

**Files:** `core/engines/drums/`

| Item | Spec |
|------|------|
| Architecture | 8 DSP synthesis pads + 8 WAV sample pads |
| Polyphony | 16 pads one-shot (no noteOff) |
| Hi-hat choke | Pad 2 (HiHatO) ↔ Pad 3 (HiHatC) mutual kill |
| Hi-hat synthesis | 808-style: 6 osc RM pairs + 30% white noise |
| Hi-hat RM ratios | 1.000, 1.4471, 1.6818, 1.9545, 2.2727, 2.6364 |
| Kick sweep | Parameterized: depth 0–1, time 10–200ms |
| Velocity dynamics | Amplitude + decay scaling (velScale = 0.8 + 0.4×vel) |
| MIDI mapping | GM-compatible: Kick=36, Snare=38, HiHatC=42, HiHatO=46 |
| Sample pads | Notes 44–51, WAV loaded via DrumSampleLoader at startup |
| Params | 67 (+2 kick sweep) |

**DSP pad types:**

| Pad | Type | Model |
|-----|------|-------|
| Kick | Sine + exp pitch sweep (parameterized depth + time) | `DspDrumType::Kick` |
| Snare | Sine + bandpassed noise | `DspDrumType::Snare` |
| HiHat C | 6-osc RM pairs + BPF + short decay | `DspDrumType::HiHatC` |
| HiHat O | Same RM model + long decay | `DspDrumType::HiHatO` |
| Clap | 4 noise bursts with small timing offsets | `DspDrumType::Clap` |
| Tom Lo | Sine pitch sweep (low register) | `DspDrumType::TomLow` |
| Tom Mid | Sine pitch sweep (mid register) | `DspDrumType::TomMid` |
| Rimshot | Transient click + decaying ring osc | `DspDrumType::Rimshot` |

---

# PHẦN 6 — MIDI FILE PLAYER

`core/music/midi_file_player.{h,cpp}`

## 6.1 Tổng Quan

MidiFilePlayer parse và playback Standard MIDI Format (SMF) Type 0 và Type 1. Thiết kế đặc biệt để hoạt động trong audio thread mà không cần MidiEventQueue.

## 6.2 Tại Sao Không Dùng MidiEventQueue

```
// Lý do: SPSC queue yêu cầu 1 producer, 1 consumer
// MidiEventQueue: producer = MIDI thread, consumer = audio thread

// Nếu MidiFilePlayer push vào MidiEventQueue (trong audio thread):
// Audio thread vừa là producer vừa là consumer của cùng queue
// → Vi phạm SPSC contract → undefined behavior

// Giải pháp: tick() trả về BlockEvents (stack-alloc, max 64 events)
// Audio thread dispatch trực tiếp trong cùng block
struct BlockEvents {
    MidiEvent events[64];
    int count = 0;
};
```

## 6.3 Tick Interface

```cpp
// Gọi 1 lần mỗi audio block (256 samples ≈ 5.8ms tại 44.1kHz)
void MidiFilePlayer::tick(int nFrames, float sampleRate,
                           BlockEvents& outEvents) noexcept;

// tick():
// 1. Advance playback position: ticksElapsed += (nFrames/sampleRate) × ticksPerSecond
// 2. Scan events vector: collect tất cả events với tick <= ticksElapsed
// 3. Fill outEvents (tối đa 64 events per block)
// 4. Route: NoteOn/NoteOff → routeNoteOn/Off() → Scale→Arp→Chord→Engine
//           CC → engine->controlChange()
//           PitchBend → engine->pitchBend()
//           Tempo change → update currentBpm_
```

## 6.4 Transport Controls

```cpp
void play() noexcept;     // atomic state_ = Playing
void pause() noexcept;    // atomic state_ = Paused (position preserved)
void stop() noexcept;     // atomic state_ = Stopped, position = 0
void seek(double beats) noexcept;  // atomic seekReq_ + seekPos_
bool isPlaying() const noexcept;
double getPositionBeats() const noexcept;
double getTotalBeats() const noexcept;
```

## 6.5 SMF Parser (HAL Boundary)

`hal/pc/midi_file_loader.h` — đọc file MIDI từ disk, parse binary SMF format, trả về `MidiFileData` (vector of timed events). MidiFilePlayer nhận `MidiFileData` — không biết file path, không có I/O. HAL boundary sạch.

---

# PHẦN 7 — DSP ENGINE IMPROVEMENTS (P1 → P3)

## 7.1 Tổng Kết Chất Lượng

| Engine | V2.1 Quality | V2.2 Quality | Key improvement |
|--------|-------------|-------------|-----------------|
| Moog | ★★★★☆ | ★★★★½ | Output limiter, note priority, analog unison |
| Hammond | ★★★☆☆ | ★★★★☆ | Leslie dual-rotor, tube overdrive, drawbar smooth |
| Rhodes | ★★☆☆☆ | ★★★★★ | Complete physical model rewrite (Tolonen 1998) |
| DX7 | ★★☆☆☆ | ★★★★☆ | Exp envelopes, fixed-freq ops, KRS, oldest-active |
| Mellotron | ★★☆☆☆ | ★★★★☆ | 8-cycle wavetable, wow/flutter, tape hiss |
| Drums | ★★★☆☆ | ★★★★☆ | 808 RM hi-hat, kick sweep, velocity dynamics |

## 7.2 Code Patterns Canonical — Chuẩn Hóa Toàn Codebase

Sau P1–P3, 4 patterns sau là chuẩn cho mọi engine mới hoặc future improvements:

**Pattern 1 — RT-Safe Pitch Bend:**
```cpp
// Header: float pitchBendCache_ = 1.0f;
// pitchBend() [UI/MIDI thread]:
pitchBendCache_ = std::pow(2.0f, std::clamp(bend/8192.0f*2.0f, -2.f, 2.f) / 12.0f);
// tickSample() [audio thread]: v.tick(pitchBendCache_, outL, outR);
```

**Pattern 2 — ParamSmoother:**
```cpp
// init(): smoother_.init(sr, 5.0f); smoother_.snapTo(initial);
// beginBlock(): smoother_.setTarget(paramCache_[id]);
// tickSample(): float v = smoother_.tick();
```

**Pattern 3 — Voice Stealing Oldest-Active:**
```cpp
// Pass 1: tìm idle voice → best
// Pass 2 (nếu không có): tìm min(voiceAge_[i]) → steal → voiceAge_[best] = ++counter
```

**Pattern 4 — ModalMode::updateFreq() — Pop-Free:**
```cpp
coef = 2.0f * std::cos(TWO_PI * freqHz / sr);  // chỉ update coef, giữ y0/y1
```

## 7.3 Thống Kê Params Mới Thêm

| Engine | V2.1 params | V2.2 params | Delta |
|--------|------------|------------|-------|
| Moog | 41 | 42 | +1 (note priority) |
| Hammond | 17 | 22 | +5 (Leslie×4, overdrive) |
| Rhodes | 9 | 11 | +2 (vibrato rate/depth) |
| DX7 | 45 | 63 | +18 (fixed-mode×12, KRS×6) |
| Mellotron | 7 | 11 | +4 (wow/flutter depth/rate) |
| Drums | 65 | 67 | +2 (kick sweep depth/time) |

---

# PHẦN 8 — HAL LAYER — FILE MỚI

## 8.1 engine_preset_storage.{h,cpp}

Load/save per-engine JSON preset. Hỗ trợ tất cả 6 engines thông qua `IEngine` interface.

```cpp
class EnginePresetStorage {
    std::string engineDir_;  // assets/hammond_presets/, assets/rhodes_presets/, etc.
    std::string lastError_;
public:
    void setEngine(const char* engineName);     // chọn thư mục assets
    std::vector<std::string> list() const;      // danh sách presets
    bool loadByIndex(int idx, IEngine& eng);    // load → setParam() tất cả params
    bool save(const std::string& fn,
              const char* name, const IEngine& eng); // getParam() tất cả → JSON
    const std::string& getLastError() const;
};
```

## 8.2 global_preset_storage.{h,cpp}

Lưu kết hợp engine params + effect chain config vào một JSON file.

```cpp
struct GlobalPreset {
    std::string name;
    std::string category;
    std::string engineName;
    std::map<int, float> engineParams;    // id → value
    EffectChainConfig effectChain;
};

class GlobalPresetStorage {
    GlobalPreset capture(const char* name, const char* cat,
                          EngineManager& mgr, EffectChain& chain);
    bool save(const std::string& fn, const GlobalPreset& gp);
    bool load(const std::string& fn, GlobalPreset& gp);
    void apply(const GlobalPreset& gp, EngineManager& mgr, EffectChain& chain);
};
```

## 8.3 midi_file_loader.h

HAL interface để tách I/O khỏi DSP Core:
```cpp
struct MidiFileData { std::vector<TimedMidiEvent> events; double totalBeats; double bpm; };
MidiFileData loadMidiFile(const std::string& path);  // [RT-UNSAFE] — đọc file
```

## 8.4 drum_sample_loader.h

WAV loader, gọi một lần tại startup:
```cpp
std::vector<float> loadWavMono(const std::string& path, float targetSampleRate);
// Engine nhận ownership của vector → DrumPadSample playback
```

---

# PHẦN 9 — UI SYSTEM — CẤU TRÚC MỚI

## 9.1 Tái Cấu Trúc Subfolder

V2.1 có tất cả panels trong `ui/panels/`. V2.2 chia thành 3 subfolder:

```
ui/panels/
  engines/      ← per-engine UI panels
  controls/     ← global controls: effects, music, MIDI, presets, output
  analysis/     ← visualization: oscilloscope, spectrum, etc.
```

## 9.2 Panel Engine Selector

`ui/panels/engines/panel_engine_selector.{h,cpp}` — điểm vào duy nhất của Engine window.

```cpp
void render(EngineManager& mgr, EnginePresetStorage& storage,
            EffectChain& chain, /*...*/) {
    // RadioButton: MiniMoog | Hammond | Rhodes | DX7 | Mellotron | Drums
    // Chọn → mgr.switchEngine(idx)
    // Render sub-panel tương ứng:
    switch (activeIdx) {
        case 0: PanelMoog::render(mgr); break;
        case 1: PanelHammond::render(mgr); break;
        // ...
    }
}
```

## 9.3 Analysis Panels

6 panels phân tích tín hiệu trong `ui/panels/analysis/`:

| Panel | Tên | Mô tả |
|-------|-----|-------|
| `panel_oscilloscope` | Oscilloscope | Triggered waveform, auto/manual scale |
| `panel_spectrum` | Spectrum | Real-time FFT magnitude (log scale) |
| `panel_spectrogram` | Spectrogram | Scrolling time-frequency heatmap |
| `panel_lissajous` | Lissajous | L vs R scatter plot (stereo phase) |
| `panel_vumeter` | VU Meter | Peak + RMS L/R với peak hold |
| `panel_correlation` | Correlation | L/R cross-correlation curve |

Tất cả đọc từ oscilloscope ring buffer của EngineManager — không cần thêm data path.

## 9.4 Panel Presets — Layout Update

`ui/panels/controls/panel_presets.cpp` — 3 tabs:

- **Engine tab:** Reset / Refresh / [name] Save phía trên. **Load bên dưới danh sách, căn phải.**
- **Effect tab:** Reset / Refresh / [name] Save phía trên. **Load bên dưới danh sách, căn phải.**
- **Global tab:** Refresh / [name] [cat] Save phía trên. **Load bên dưới danh sách, căn phải.**

Right-align pattern:
```cpp
const float btnW = 80.f;
ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - btnW);
if (ImGui::Button("Load##eng", ImVec2(btnW, 0.f)) && active) { ... }
```

## 9.5 Panel MIDI Player

`ui/panels/controls/panel_midi_player.{h,cpp}`:
```
[File Browser] [Load] [▶ Play] [⏸ Pause] [⏹ Stop]
Progress bar: ████████░░░░░░ 2:34 / 4:15
BPM: 120.0   Engine: MiniMoog Model D
```

---

# PHẦN 10 — FACTORY ASSETS V2.2

## 10.1 Tổng Hợp Assets

| Directory | Count | Format | Notes |
|-----------|-------|--------|-------|
| `assets/moog_presets/` | 10 | JSON | Presets mới, schema cập nhật (42 params) |
| `assets/hammond_presets/` | 10 | JSON | Mới hoàn toàn |
| `assets/rhodes_presets/` | 10 | JSON | Mới hoàn toàn (11 params V2) |
| `assets/dx7_presets/` | 10 | JSON | Mới hoàn toàn (63 params) |
| `assets/mellotron_presets/` | 10 | JSON | Mới hoàn toàn (11 params) |
| `assets/drum_kits/default/` | — | WAV | Sample pads 8–15 |
| `assets/global_presets/` | 35 | JSON | Cross-engine (engine + effects) |
| `assets/effect_presets/` | 20 | JSON | Mở rộng từ 10 |
| `assets/sequencer_patterns/` | 20 | JSON | Mở rộng từ 10 |
| `assets/midi/` | 10 | SMF | Classical repertoire, public domain |

**Tổng: 145 factory assets** (V2.1: 40 assets)

## 10.2 Global Presets — Danh Sách

35 global presets bao phủ tất cả 6 engines:
- **Moog (6):** arena lead, deep sub, acid house, ambient pad, retro funk, dark drone, 80s poly
- **Hammond (6):** gospel live, jazz club, prog rock, soul groove, church, 80s poly
- **Rhodes (5):** neo soul, jazz lounge, funk wah, chill wave, vintage amp
- **DX7 (6):** 80s EP, crystal bells, synth brass, ambient bell, bass stab, FM organ
- **Mellotron (5):** strawberry fields, space choir, vintage orch, dark ambient, brass section
- **Drums (7):** 808 boom, rock kit, jazz brush, electronic, lo-fi, dub echo, industrial

## 10.3 MIDI Files — Danh Sách

| File | Composer |
|------|----------|
| Moonlight Sonata Op.27 No.2 Mvt.1 | Beethoven |
| Prelude and Fugue in C Major BWV 846 | Bach (WTK I) |
| Well-Tempered Clavier I — BWV 866 | Bach |
| Nocturne Op.9 No.1 | Chopin |
| Vous Dirai-je Maman (Twinkle) | Mozart arr. |
| Toccata and Fugue BWV 547 | Bach |
| Piano Sonata No.13 Mvt.1 | Mozart |
| Swan Lake Op.20 Theme | Tchaikovsky arr. |
| The Art of Fugue BWV 1080 — Contrapunctus I | Bach |
| Goldberg Variations BWV 988 | Bach |

---

# PHẦN 11 — THREADING MODEL (V2.2)

## 11.1 Tổng Quan 3 Thread

| Thread | Role | Sync mechanism |
|--------|------|---------------|
| **UI (main)** | ImGui render, param writes, MIDI file load/seek | `std::atomic<float>` params; `std::atomic<int>` activeIdx_ |
| **Audio (RtAudio)** | processBlock(), tickSample() × N, effectChain | Atomic reads; SPSC queue drain; try_lock effect config |
| **MIDI (RtMidi)** | Hardware MIDI input | SPSC queue push |

## 11.2 Nguyên Tắc RT-Safety (không đổi từ V2.1)

**FORBIDDEN trong audio thread:** `new`/`delete`/`malloc`, `push_back` (realloc), `std::string` construction, `mutex::lock()`, `cout`/`printf`, file I/O, `throw`.

**ALLOWED:** Stack alloc ≤64KB, `std::atomic` load/store relaxed, `SPSCQueue::push/pop`, `std::array`, arithmetic/math, `sin`/`cos`/`pow` (single call OK per block).

## 11.3 Preset Load During MIDI Playback — Bug Fix

**Vấn đề (V2.1):** Load Moog preset (nhiều bass presets có sustain = 0) trong khi MIDI đang play → âm thanh im lặng. Voice ở giai đoạn Sustain lập tức nhận `level_ = 0.0f`, không recover.

**Giải pháp (V2.2):** Gọi `active->allNotesOff()` TRƯỚC khi load preset:
```cpp
if (ImGui::Button("Load##eng") && active) {
    active->allNotesOff();                          // Chuyển voices sang Release
    storage.loadByIndex(s_engSelected, *active);    // Load params mới
}
```
Pattern tương tự cho Reset button và double-click trên danh sách.

---

# PHẦN 12 — CẤU TRÚC THƯ MỤC V2.2

```
minimoog_digitwin/
├── sim/
│   └── main.cpp                      Application entry point
├── ui/
│   ├── imgui_app.{h,cpp}             Top-level render + menu bar
│   ├── panels/
│   │   ├── engines/
│   │   │   ├── panel_engine_selector.{h,cpp}
│   │   │   ├── panel_moog.{h,cpp}
│   │   │   ├── panel_hammond.{h,cpp}
│   │   │   ├── panel_rhodes.{h,cpp}
│   │   │   ├── panel_dx7.{h,cpp}
│   │   │   ├── panel_mellotron.{h,cpp}
│   │   │   └── panel_drums.{h,cpp}
│   │   ├── controls/
│   │   │   ├── panel_effects.{h,cpp}
│   │   │   ├── panel_music.{h,cpp}
│   │   │   ├── panel_midi_player.{h,cpp}
│   │   │   ├── panel_output.{h,cpp}
│   │   │   └── panel_presets.{h,cpp}
│   │   └── analysis/
│   │       ├── panel_oscilloscope.{h,cpp}
│   │       ├── panel_spectrum.{h,cpp}
│   │       ├── panel_spectrogram.{h,cpp}
│   │       ├── panel_lissajous.{h,cpp}
│   │       ├── panel_vumeter.{h,cpp}
│   │       └── panel_correlation.{h,cpp}
│   └── widgets/                       knob, ADSR, piano, seq grid
├── hal/pc/
│   ├── rtaudio_backend.{h,cpp}
│   ├── pc_midi.{h,cpp}
│   ├── keyboard_input.{h,cpp}
│   ├── engine_preset_storage.{h,cpp}  ← MỚI V2.2
│   ├── global_preset_storage.{h,cpp}  ← MỚI V2.2
│   ├── midi_file_loader.h              ← MỚI V2.2
│   ├── drum_sample_loader.h            ← MỚI V2.2
│   ├── moog_preset_storage.{h,cpp}    (compat legacy)
│   └── effect_preset_storage.{h,cpp}
├── shared/
│   ├── types.h
│   ├── params.h
│   └── interfaces.h
├── core/
│   ├── dsp/                           Oscillator, MoogFilter, Envelope, LFO, …
│   ├── music/
│   │   ├── arpeggiator.{h,cpp}
│   │   ├── step_sequencer.{h,cpp}
│   │   ├── chord_engine.{h,cpp}
│   │   ├── scale_quantizer.{h,cpp}
│   │   └── midi_file_player.{h,cpp}   ← MỚI V2.2
│   ├── engines/
│   │   ├── iengine.h                  ← MỚI V2.2
│   │   ├── engine_manager.{h,cpp}     ← MỚI V2.2
│   │   ├── moog/
│   │   │   ├── moog_engine.{h,cpp}
│   │   │   ├── moog_params.h
│   │   │   ├── voice.{h,cpp}
│   │   │   └── voice_pool.{h,cpp}
│   │   ├── hammond/
│   │   │   ├── hammond_engine.{h,cpp}
│   │   │   └── hammond_voice.h
│   │   ├── rhodes/
│   │   │   ├── rhodes_engine.{h,cpp}
│   │   │   └── rhodes_voice.h
│   │   ├── dx7/
│   │   │   ├── dx7_engine.{h,cpp}
│   │   │   ├── dx7_voice.h
│   │   │   └── dx7_algorithms.h
│   │   ├── mellotron/
│   │   │   ├── mellotron_engine.{h,cpp}
│   │   │   ├── mellotron_voice.h
│   │   │   └── mellotron_tables.h
│   │   └── drums/
│   │       ├── drum_engine.{h,cpp}
│   │       ├── drum_pad_dsp.h
│   │       └── drum_pad_sample.h
│   ├── effects/                        EffectChain + 8 effect types
│   └── util/                           SPSCQueue, math_utils
├── assets/
│   ├── moog_presets/                  10 JSON
│   ├── hammond_presets/               10 JSON  ← MỚI
│   ├── rhodes_presets/                10 JSON  ← MỚI
│   ├── dx7_presets/                   10 JSON  ← MỚI
│   ├── mellotron_presets/             10 JSON  ← MỚI
│   ├── drum_kits/default/             WAV      ← MỚI
│   ├── global_presets/                35 JSON  ← MỚI
│   ├── effect_presets/                20 JSON  (10 cũ + 10 mới)
│   ├── sequencer_patterns/            20 JSON  (10 cũ + 10 mới)
│   └── midi/                          10 SMF   ← MỚI
├── documents/
│   ├── MiniMoog DSP Simulator - TDD - V1.md
│   ├── MiniMoog DSP Simulator - TDD - V2.1.md
│   ├── MiniMoog DSP Simulator - TDD - V2.2.md  ← TÀI LIỆU NÀY
│   ├── DSP_Engine_Analysis_v1.md
│   ├── DSP_Engine_Analysis_v2.md
│   ├── Engine_Improvements_Log.md
│   ├── Huong_Dan_Su_Dung.md
│   ├── Preset_Reference_Guide.md
│   └── Visualization_Guide.md
├── tests/                             44 Catch2 unit tests
├── tools/                             gen_midi_assets (CMake custom target)
├── CMakeLists.txt
├── CLAUDE.md
└── README.md
```

---

# PHẦN 13 — BUG FIXES

## 13.1 Pitch Bend No-Op (P1)

**Bug:** 5 trong 6 engines có `pitchBend() {}` empty override — mọi MIDI pitch bend event bị bỏ qua.

**Fix:** Implement pitchBend() cho Hammond, Rhodes, DX7, Mellotron với chuẩn pattern RT-safe (§7.2 Pattern 1). Rhodes dùng `updateFreq()` thay vì nhân phaseInc (modal resonators không có phaseInc).

## 13.2 DX7 Pop Khi Voice Steal (P1)

**Bug:** Round-robin stealing chọn voice theo index tuần tự — có thể steal voice đang ở peak attack → audible pop.

**Fix:** Oldest-active stealing (§5.4) — steal voice đã sound lâu nhất, thống kê ở sustain/release → ít pop nhất.

## 13.3 Hammond Drawbar Zipper Noise (P1)

**Bug:** Drawbar value đọc trực tiếp từ `paramCache_` mỗi sample — kéo drawbar tạo step thay đổi → zipper noise.

**Fix:** 9 × ParamSmoother 5ms, setTarget() ở beginBlock(), tick() ở tickSample() (§5.2).

## 13.4 Rhodes Stereo Wobble (V2 Rewrite)

**Bug:** DC blocker trong V1 dùng chung state `{x1, y1}` cho cả L và R — DC blocker kênh L ảnh hưởng kênh R → stereo wobble artifact.

**Fix:** Tách thành 4 state variables: `dcX1L, dcY1L, dcX1R, dcY1R` (§5.3).

## 13.5 Moog Unison Clip (P2)

**Bug:** 8 voices unison stack → amplitude tổng có thể lên tới 8× → hard clip tại ±1.

**Fix:** Post-VCA tanh limiter `tanh(x×1.25)/1.25` — transparent ở single voice, graceful compression khi stack (§5.1, §7.2).

## 13.6 Preset Load Silence During MIDI Playback (Bug Fix Session)

**Bug:** Load Moog preset (sustain = 0) trong khi MIDI đang play → voices đang ở Sustain stage lập tức `level_ = 0` → silence. Voices không Idle, block voice slot mới.

**Root cause:** `ControlEnvelope::tick()` Sustain stage: `level_ = p_.sustain` — đọc ngay giá trị mới mỗi sample.

**Fix:** `active->allNotesOff()` trước mọi preset load → voices chuyển sang Release → voices new MIDI notes trigger từ Attack stage với preset mới. Applied tại 3 locations: Load button, Reset button, double-click (§11.3).

---

# PHẦN 14 — PERFORMANCE ANALYSIS

## 14.1 CPU Cost Per Block (256 samples @ 44.1 kHz)

| Engine | Worst case voices | Cost/voice/sample | Total/block |
|--------|------------------|------------------|-------------|
| Moog | 8 voices unison | ~38 flops | ~78K flops |
| Hammond | 61 voices + Leslie | ~35 flops + 10 Leslie | ~563K flops |
| Rhodes | 12 voices | ~32 flops | ~98K flops |
| DX7 | 8 voices | ~40 flops | ~82K flops |
| Mellotron | 35 voices + 2 LFO | ~8 flops | ~71K flops |
| Drums | 16 pads | ~14 flops | ~57K flops |

**Hammond là engine nặng nhất** tại full polyphony. Trên PC hiện đại (≥2 GHz), tất cả đều trong budget thoải mái.

**Teensy V3 concern:** Hammond 61 voices + Leslie cần optimization cho ARM Cortex-M7:
- Giới hạn polyphony 32 voices
- Leslie delay read dùng lookup table thay sin/cos
- Leslie process tại block rate (64 samples) thay per-sample

## 14.2 Memory Footprint (DSP Core)

| Component | Size |
|-----------|------|
| Moog voices (8) | 3.2 KB |
| Hammond voices (61) + Leslie delay | 6.1 KB + 8 KB |
| Rhodes voices (12) | 2.4 KB |
| DX7 voices (8) | 2.6 KB |
| Mellotron voices (35) | 1.8 KB |
| Mellotron wavetables (4 × 16384 × 4B) | **256 KB** |
| Drum DSP pads (8) | 0.6 KB |
| Drum sample data | Variable (WAV heap) |
| **Total DSP state (excl. samples)** | **~283 KB** |

**Mellotron wavetable là mục lớn nhất** (256 KB). Teensy V3: dùng `MELLO_CYCLES = 4` → 128 KB qua `#ifdef TEENSY`.

---

# PHẦN 15 — TEST PLAN (V2.2)

## 15.1 Unit Tests (không thay đổi)

44 Catch2 tests trong `tests/`, bao phủ toàn bộ DSP primitives:

```
test_oscillator.cpp    — 6 waveforms, phase, PolyBLEP
test_moog_filter.cpp   — 4-pole response, resonance, tanh saturation
test_envelope.cpp      — ADSR stages, timing, retrigger
test_glide.cpp         — portamento timing, pitch accuracy
test_voice.cpp         — Moog voice tick, param cache
test_arpeggiator.cpp   — 6 modes, octave range, timing
test_sequencer.cpp     — step edit, gate, tie, swing
test_scale_quantizer.cpp — 16 scales, 12 roots
```

## 15.2 Integration Checklist (manual)

### Multi-Engine
- [ ] Chuyển engine trong khi MIDI đang play → không pop, không silence
- [ ] Mỗi engine phát sound đúng với phím keyboard
- [ ] Pitch bend wheel ảnh hưởng đúng trên tất cả 5 engines (Hammond/Rhodes/DX7/Mellotron/Moog)
- [ ] Chuyển engine → music layer (Arp/Chord/Scale) vẫn hoạt động trên engine mới

### MIDI File Player
- [ ] Load MIDI file → play → sound qua active engine
- [ ] Pause → Resume giữ đúng position
- [ ] Stop → position về 0
- [ ] Chuyển engine trong khi play → MIDI tiếp tục, tiếng mới
- [ ] Load Moog preset trong khi MIDI play → không silence

### Preset System
- [ ] Load Engine preset: tất cả params cập nhật, sound thay đổi
- [ ] Save Engine preset → file JSON tạo đúng
- [ ] Load Global preset → engine thay đổi + effects thay đổi
- [ ] Load button bên dưới danh sách, căn phải (3 tabs)

### Effects
- [ ] Tất cả 8 effect types hoạt động trên mọi engine
- [ ] Add/Remove slot trong khi play → không pop
- [ ] EQ param thay đổi real-time → không glitch (double-buffer)

### Analysis Panels
- [ ] Oscilloscope hiển thị waveform đúng
- [ ] Spectrum, Spectrogram, Lissajous, VU meter cập nhật real-time

---

# PHẦN 16 — ROADMAP V3 (CẬP NHẬT)

## 16.1 V3 Target: Teensy 4.1 Hardware Port

```
hal/teensy/         ← New HAL (swap thay hal/pc/)
  teensy_audio.*    ← I2S audio (Teensy Audio Library)
  sd_preset.*       ← SD card read/write JSON
  encoder_input.*   ← Rotary encoder hardware controls
  oled_display.*    ← Small OLED status display
```

## 16.2 P4 DSP Improvements (Được Xác Định Trong DSP_Engine_Analysis_v2.md)

**Ưu tiên cao:**
- **DX7 System LFO** — PM + AM modulation to operators (6 waveforms)
- **DX7 Operator key-level scaling** — amplitude curve by note (piano accuracy)
- **Moog hard sync** OSC1 → OSC2

**Ưu tiên trung bình:**
- Velocity → Moog filter cutoff modulation (`MP_VEL_FILTER_AMT`)
- DX7 operator ratio range → 24× (từ 8×)
- Hammond scanner vibrato delay-line approximation (Pekonen 2011)
- Rhodes vibrato onset delay per voice
- Mellotron per-tape attack shape
- Drums snare wire buzz (decaying 2kHz bandpass noise)

**Deferred:**
- MIDI CC learn (map CC → param)
- Drum kit browser UI
- True 4R4L DX7 envelope

## 16.3 Tính Năng V3 Mới

- **Karplus-Strong oscillator** — plucked string, acoustic guitar
- **Velocity → filter/timbre** — chuẩn hóa cho Moog, DX7
- **Delay BpmSync** — `ctx.bpm` đã wire sẵn, chỉ cần UI toggle
- **Wavetable oscillator** — sample-based timbres cho Moog

---

# GLOSSARY

| Term | Định nghĩa |
|------|-----------|
| **IEngine** | Interface C++ abstract cho tất cả 6 synthesis engines |
| **EngineManager** | Orchestrator: music layer + engine routing + effect chain coordination |
| **BlockEvents** | Stack-allocated struct (max 64 MidiEvent) trả về từ MidiFilePlayer::tick() |
| **MidiFilePlayer** | Parse và playback SMF MIDI files trong audio thread, không dùng SPSC queue |
| **EnginePresetStorage** | HAL class: load/save per-engine JSON preset từ disk |
| **GlobalPresetStorage** | HAL class: load/save combined engine+effects JSON preset |
| **Leslie rotary** | Dual-rotor speaker (Hammond Model 122): horn + drum, Doppler + AM |
| **Modal resonator** | Biquad IIR decaying oscillator mô phỏng 1 mode của tine rung |
| **Pickup nonlinearity** | Polynomial waveshaper y = x + αx² + βx³ mô phỏng magnetic pickup |
| **P1/P2/P3** | Ba giai đoạn cải thiện DSP engine (xem Engine_Improvements_Log.md) |
| **Oldest-active** | Voice stealing: steal voice đã sound lâu nhất → ít pop nhất |
| **updateFreq()** | Cập nhật frequency của modal resonator mà không reset state (no pop) |
| **ParamSmoother** | First-order IIR ramp 5ms loại bỏ zipper noise khi thay đổi knob |
| **SPSC queue** | Single-Producer Single-Consumer lock-free ring buffer (MidiEventQueue) |
| **SM** | `std::memory_order_relaxed` — dùng cho atomic param reads trong hot path |
| **Wow/Flutter** | Pitch modulation từ tape transport: wow = 0.1–3 Hz, flutter = 6–18 Hz |
| **KRS** | Keyboard Rate Scaling — DX7 envelope time scale theo note pitch |

---

*MiniMoog DSP Simulator V2.2 — TDD Document — March 2026*
*Tham chiếu: Engine_Improvements_Log.md, DSP_Engine_Analysis_v1.md, DSP_Engine_Analysis_v2.md*
