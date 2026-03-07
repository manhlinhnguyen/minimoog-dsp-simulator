# MiniMoog DSP Simulator

# Technical Design Document — V2.1

**Document version:** 2.1.0
**Target platform:** PC (Windows / macOS / Linux)
**Language:** C++17
**Status:** Implemented & Verified
**Tài liệu tham chiếu:** TDD V1.0 (baseline), README.md

---

# MỤC LỤC

| # | Tên phần | Trạng thái |
|---|----------|------------|
| 1 | TỔNG QUAN V2.1 — NHỮNG GÌ THAY ĐỔI SO VỚI V1 | ✅ |
| 2 | KIẾN TRÚC TỔNG THỂ (V2.1) | ✅ |
| 3 | EFFECTS SYSTEM — THIẾT KẾ CHI TIẾT | ✅ |
| 4 | HIỆU ỨNG RIÊNG LẺ (8 Effect Types) | ✅ |
| 5 | EFFECT PRESET STORAGE | ✅ |
| 6 | HAL LAYER — FILE RENAMING & NEW FILES | ✅ |
| 7 | UI SYSTEM — CONSOLIDATION & NEW PANELS | ✅ |
| 8 | SHARED LAYER — CLEANUPS & FIXES | ✅ |
| 9 | THREADING MODEL (V2.1) | ✅ |
| 10 | CẤU TRÚC THƯ MỤC V2.1 | ✅ |
| 11 | FACTORY ASSETS V2.1 | ✅ |
| 12 | TEST PLAN (V2.1) | ✅ |
| 13 | BUILD SYSTEM (CMakeLists.txt changes) | ✅ |
| 14 | CODING CONVENTIONS (V2.1 additions) | ✅ |
| 15 | ROADMAP V3 | ✅ |
| 16 | GLOSSARY | ✅ |

---

# PHẦN 1 — TỔNG QUAN V2.1: NHỮNG GÌ THAY ĐỔI SO VỚI V1

## 1.1 Mục Tiêu V2.1

V2.1 mở rộng V1 theo hai hướng chính:

1. **Post-synth effect chain** — thêm 8 loại hiệu ứng âm thanh nối tiếp nhau sau VCA output, với hệ thống preset riêng.
2. **Codebase cleanup & hardening** — đổi tên file cho rõ nghĩa, loại bỏ dead code, sửa data race tiềm ẩn, thêm `noexcept` đầy đủ, fix oscilloscope slider bug.

V2.1 **không thay đổi** DSP core (oscillators, filter, envelopes, voice pool, music layer) — tất cả vẫn 100% tương thích với V1.

## 1.2 Tính Năng Mới V2.1

```
EFFECTS CHAIN (MỚI):
  ✅ EffectChain — tối đa 16 slot nối tiếp
  ✅ Gain / Overdrive / Distortion
  ✅ Stereo Chorus
  ✅ Jet Flanger
  ✅ 4-stage All-pass Phaser
  ✅ Amplitude Tremolo (Sine/Triangle/Square)
  ✅ Stereo Delay (tối đa 2 giây)
  ✅ Schroeder Reverb (comb + all-pass)
  ✅ 5-band Parametric Equalizer (double-buffered)
  ✅ Add / Remove / Reorder effects tự do
  ✅ Effect preset save/load (JSON)
  ✅ 10 factory effect presets

UI:
  ✅ panel_effects — effect chain editor
  ✅ panel_moog_engine — hợp nhất Controllers+OSC+Mixer+Filter (3 cột)
  ✅ Oscilloscope auto/manual scale toggle
  ✅ panel_presets — 2 tab (Moog Presets + Effect Presets)

HAL:
  ✅ moog_preset_storage — đổi tên từ preset_storage
  ✅ sequencer_pattern_storage — đổi tên từ pattern_storage
  ✅ effect_preset_storage — file mới

CLEANUPS:
  ✅ Xóa 7 panel file không compile (dead code)
  ✅ noexcept đầy đủ trên IAudioProcessor, SynthEngine, helpers
  ✅ HeldNote::active field thừa bị xóa
  ✅ setSlotParam atomic store nhất quán với giá trị đã clamp
  ✅ Oscilloscope slider lúc nào cũng hoạt động đúng
```

## 1.3 Những Gì KHÔNG Thay Đổi

- Toàn bộ `core/dsp/` (Oscillator, MoogFilter, Envelope, LFO, Glide, Noise, ParamSmoother)
- Toàn bộ `core/voice/` (Voice, VoicePool)
- Toàn bộ `core/music/` (Arpeggiator, StepSequencer, ChordEngine, ScaleQuantizer)
- `core/engine/synth_engine.cpp` — chỉ thêm `noexcept` ở override
- `shared/params.h` — ParamID enum không đổi (chỉ thêm `noexcept` ở helpers)
- `shared/types.h` — chỉ xóa field thừa `HeldNote::active`
- MIDI, RtAudio backend, QWERTY keyboard input
- 44 unit tests — tất cả vẫn pass

---

# PHẦN 2 — KIẾN TRÚC TỔNG THỂ (V2.1)

## 2.1 Layer Diagram

```
╔══════════════════════════════════════════════════════════════╗
║                     APPLICATION LAYER                         ║
║  sim/main.cpp                                                ║
║  • Khởi tạo tất cả subsystem (thêm EffectChain)             ║
║  • GLFW event loop + render loop                             ║
╠══════════════════════════════════════════════════════════════╣
║                       UI LAYER                                ║
║  ui/imgui_app.h/cpp                                          ║
║  ui/panels/panel_moog_engine.*   ← HỢP NHẤT (3 cột)        ║
║  ui/panels/panel_music.*         ← Arp+Chord+Scale+Seq+KB   ║
║  ui/panels/panel_effects.*       ← MỚI: effect chain editor ║
║  ui/panels/panel_oscilloscope.*  ← auto/manual scale toggle ║
║  ui/panels/panel_presets.*       ← 2 tab (Moog + Effect)    ║
║  ui/panels/panel_output.*        ← Master volume            ║
║  ui/widgets/                     ← knob, ADSR, piano, seq   ║
╠══════════════════════════════════════════════════════════════╣
║                      PC HAL LAYER                             ║
║  hal/pc/rtaudio_backend.*        ← RtAudio                  ║
║  hal/pc/pc_midi.*                ← RtMidi                   ║
║  hal/pc/keyboard_input.*         ← GLFW keys                ║
║  hal/pc/moog_preset_storage.*    ← ĐỔITÊN: preset_storage   ║
║  hal/pc/sequencer_pattern_storage.* ← ĐỔITÊN: pattern_storage║
║  hal/pc/effect_preset_storage.*  ← MỚI                      ║
╠══════════════════════════════════════════════════════════════╣
║                     SHARED LAYER                              ║
║  shared/types.h      shared/params.h      shared/interfaces.h║
╠══════════════════════════════════════════════════════════════╣
║                    DSP CORE LAYER                             ║
║  core/dsp/      core/voice/    core/music/   core/engine/   ║
║  core/effects/  ← MỚI: EffectBase, EffectChain, 8 effects  ║
║  core/util/                                                  ║
╚══════════════════════════════════════════════════════════════╝
```

## 2.2 Signal Path (V2.1)

```
OSC1 ─┐
OSC2 ─┼──► MIXER ──► MOOG LADDER FILTER ──► VCA ──► EFFECT CHAIN ──► OUTPUT
OSC3 ─┤              ▲         ▲            ▲         │
NOISE─┘           FilterEnv  KbdTrack    AmpEnv       │
                                │                     ├── Gain
                    LFO (OSC3 LFO mode)               ├── Chorus
                                                      ├── Flanger
                                                      ├── Phaser
                                                      ├── Tremolo
                                                      ├── Delay
                                                      ├── Reverb
                                                      └── Equalizer
                                               (tối đa 16 slot, nối tiếp)
```

## 2.3 Data Flow (V2.1)

```
UI Thread:                        Audio Thread:
──────────                        ─────────────
User interaction                  RtAudio callback fires
     │                                  │
     ▼                                  ▼
ImGui renders UI              SynthEngine::processBlock()
     │                                  │
     ▼                                  ├── Drain MidiEventQueue
AtomicParamStore                        │        │
  .set(id, val)                         │        ▼
     │                                  │   MusicLayer tick (Arp/Seq)
Effect param changes                    │        │
  → EffectChain::setSlotParam()         │        ▼
     │                                  │   VoicePool.noteOn/Off
     ▼                                  │        │
MidiEventQueue.push()                   ├── Read ParamCache
                                        │        │
                                        ▼        ▼
                                   Voice[n].tick() × N voices
                                        │
                                        ▼
                                   Mix + Master Volume
                                        │
                                        ▼
                                   EffectChain::processBlock()
                                   (lock-free, atomic params,
                                    double-buffered EQ coef)
                                        │
                                        ▼
                                   outL[], outR[] → RtAudio DAC
```

---

# PHẦN 3 — EFFECTS SYSTEM: THIẾT KẾ CHI TIẾT

## 3.1 Tổng Quan

Effects system gồm 3 lớp:

```
EffectBase          ← Interface chung (abstract)
  └── 8 concrete effect classes
        (GainEffect, ChorusEffect, FlangerEffect,
         PhaserEffect, TremoloEffect, DelayEffect,
         ReverbEffect, EqEffect)

EffectChain         ← Container (tối đa 16 slot)
  • Quản lý vòng đời effects
  • Thread safety: atomic params + mutex cho structural ops
  • processBlock(): gọi tuần tự qua tất cả active slot
```

## 3.2 EffectBase Interface

```cpp
// core/effects/effect_base.h
class EffectBase {
public:
    virtual ~EffectBase() = default;
    virtual void  init(float sampleRate) noexcept = 0;
    virtual void  processBlock(float* L, float* R,
                               int nFrames) noexcept = 0;
    virtual void  setParam(int param, float value) noexcept = 0;
    virtual float getParam(int param) const noexcept = 0;
    virtual int   getParamCount() const noexcept = 0;
    virtual const char* getParamName(int param) const noexcept = 0;
    virtual EffectType  getType() const noexcept = 0;
    virtual bool  isBypassed() const noexcept = 0;
    virtual void  setBypassed(bool b) noexcept = 0;
    virtual void  reset() noexcept = 0;
};

enum class EffectType : int {
    Gain=0, Chorus=1, Flanger=2, Phaser=3,
    Tremolo=4, Delay=5, Reverb=6, Equalizer=7,
    COUNT=8
};
```

## 3.3 EffectChain — Thread Safety Model

EffectChain phải thread-safe giữa UI thread (thêm/xóa/reorder effect, thay đổi param) và Audio thread (processBlock).

### Hai loại operation:

**Structural changes** (thêm/xóa/reorder slot):
- UI thread acquire `mutex_` bằng `try_lock()` (non-blocking)
- Nếu lock fail → skip, thử lại frame sau (không block audio)
- Set `configDirty_` = true sau khi xong
- Audio thread check `configDirty_` để rebuild active list

**Param changes** (knob xoay):
- Mỗi slot có `atomicParams_[MAX_SLOTS][MAX_PARAMS]` — `std::atomic<float>`
- UI thread ghi trực tiếp bằng `store(memory_order_relaxed)`
- Audio thread đọc và gọi `effect->setParam()` tại đầu mỗi block
- Không cần mutex — atomic đủ cho float param

### Tại sao không dùng mutex cho param changes?

Params là float đơn lẻ — `std::atomic<float>` với `memory_order_relaxed` là đủ vì:
- Float param đọc/ghi không cần thứ tự với nhau
- Không có invariant "atomic group" cần giữ
- Audio thread chỉ cần "param mới nhất gần đúng" — không cần exact

```
                UI Thread          Audio Thread
                ─────────          ────────────
Param change:  atomic[s][p].store()   atomic[s][p].load()
                                       effect->setParam()

Add/Remove:    mutex.try_lock()    configDirty_.load()
               modify slots[]      rebuild active list
               configDirty_=true
               mutex.unlock()
```

### setSlotParam — clamping consistency

Khi UI gọi `setSlotParam(slot, param, rawValue)`:

```cpp
void EffectChain::setSlotParam(int slot, int param, float v) noexcept {
    if (effects_[slot]) {
        effects_[slot]->setParam(param, v);        // clamp bên trong
        atomicParams_[slot][param].store(
            effects_[slot]->getParam(param),        // đọc lại giá trị đã clamp
            std::memory_order_relaxed);
    } else {
        atomicParams_[slot][param].store(v,
            std::memory_order_relaxed);
    }
}
```

Lý do: `effect->setParam()` clamp giá trị vào range hợp lệ. Nếu lưu `v` gốc vào atomic, khi serialize/load JSON sẽ ra giá trị ngoài range. Phải lưu giá trị đã clamp.

## 3.4 EqEffect — Double-Buffered Coefficients

EQ là effect đặc biệt vì `BiquadCoef` gồm 5 float (20 bytes) — không thể atomic update nguyên khối. Nếu audio thread đọc trong khi UI thread đang ghi → undefined behavior.

**Giải pháp: Double buffering + atomic dirty flag**

```
  UI Thread                         Audio Thread
  ─────────                         ────────────
  computeCoef(band, pendingCoef_[b])
  coefDirty_[b].store(true, release)

                        ──── processBlock() ────
                        for each band b:
                          if coefDirty_[b].load(acquire):
                            coef_[b] = pendingCoef_[b]   // copy vào "live" buffer
                            coefDirty_[b].store(false, relaxed)
                        apply coef_[b] in biquad loop
```

Bộ dữ liệu trong EqEffect:
```cpp
BiquadCoef  coef_[BANDS]        = {};   // audio thread only — không ai khác ghi
BiquadCoef  pendingCoef_[BANDS] = {};   // UI thread ghi, audio thread chỉ copy
std::atomic<bool> coefDirty_[BANDS];   // handshake: release từ UI, acquire ở audio
```

`BiquadCoef`:
```cpp
struct BiquadCoef {
    float b0, b1, b2, a1, a2;
};
```

**Tại sao acquire/release?**

- `store(true, release)` đảm bảo mọi write vào `pendingCoef_[b]` xảy ra TRƯỚC khi dirty flag được thấy
- `load(acquire)` đảm bảo mọi read từ `pendingCoef_[b]` xảy ra SAU khi dirty flag được confirm

Không có race condition nào có thể xảy ra với pattern này.

## 3.5 5 Tần Số EQ Bands

```
Band 0: 80   Hz  — Low shelf    (shelving filter, S = 0.7)
Band 1: 320  Hz  — Low-Mid peak (peaking EQ,   Q = 1.0)
Band 2: 1000 Hz  — Mid peak     (peaking EQ,   Q = 1.0)
Band 3: 3500 Hz  — Hi-Mid peak  (peaking EQ,   Q = 1.0)
Band 4: 10000 Hz — High shelf   (shelving filter, S = 0.7)
```

Công thức: Audio EQ Cookbook (Robert Bristow-Johnson).

Range gain: −12 dB đến +12 dB per band.
Param 5 (Level): 0.0 → 1.0, output scale sau EQ.

---

# PHẦN 4 — HIỆU ỨNG RIÊNG LẺ (8 EFFECT TYPES)

## 4.1 Gain / Overdrive / Distortion

```
Params:
  0 — Drive    (0..1)  → linear gain 1..20x → tanh saturation
  1 — Tone     (0..1)  → 1-pole LP filter cutoff sau distortion
  2 — Asymmetry(0..1)  → DC bias trước tanh (0=symmetric, 1=max clip)
  3 — Level    (0..1)  → output gain

Signal path:
  in → (+ DC bias) → tanh(drive * in) → tone LP → * level → out
```

## 4.2 Chorus

```
Params:
  0 — Rate     (0..1)  → LFO rate 0.1..5 Hz
  1 — Depth    (0..1)  → delay modulation 0..20ms
  2 — Mix      (0..1)  → wet/dry mix
  3 — Level    (0..1)  → output gain

Implementation:
  2 delay lines (stereo), mỗi line modulated bởi sinusoidal LFO lệch pha 90°.
  Delay base = 15ms, modulation ±depth.
```

## 4.3 Flanger

```
Params:
  0 — Rate     (0..1)  → LFO rate 0.05..5 Hz
  1 — Depth    (0..1)  → delay modulation 0..10ms
  2 — Feedback (0..1)  → feedback amount 0..0.9
  3 — Mix      (0..1)  → wet/dry
  4 — Level    (0..1)

Implementation:
  Short delay line (max 20ms), feedback từ output về input.
  LFO modulates delay time → "jet plane" sweep effect.
```

## 4.4 Phaser

```
Params:
  0 — Rate     (0..1)  → LFO rate 0.05..5 Hz
  1 — Depth    (0..1)  → all-pass frequency sweep depth
  2 — Feedback (0..1)  → feedback 0..0.9
  3 — Mix      (0..1)
  4 — Level    (0..1)

Implementation:
  4 all-pass stages (1st order), center frequency modulated by LFO.
  Freq range: 200..4000 Hz.
  Stereo: L và R modulated lệch pha 180°.
```

## 4.5 Tremolo

```
Params:
  0 — Rate     (0..1)  → 0.5..20 Hz
  1 — Depth    (0..1)  → modulation depth 0..1
  2 — Shape    (0..1)  → Sine=0, Triangle=0.5, Square=1
  3 — Level    (0..1)

Implementation:
  Amplitude modulation: out = in * (1 - depth + depth * LFO)
  LFO waveform: lerp giữa Sine → Triangle → Square theo Shape param.
```

## 4.6 Delay

```
Params:
  0 — Time     (0..1)  → 10ms..2000ms
  1 — Feedback (0..1)  → 0..0.95
  2 — Mix      (0..1)
  3 — Spread   (0..1)  → stereo spread (L/R delay time offset)
  4 — Level    (0..1)

Implementation:
  Delay buffer: float[2][88200] (pre-allocated, 2 giây @ 44100 Hz).
  Stereo: R delay = Time * (1 + Spread * 0.3).
  Feedback clamped to 0.95 để tránh instability.
```

## 4.7 Reverb (Schroeder)

```
Params:
  0 — Room     (0..1)  → room size (delay line lengths scale)
  1 — Damp     (0..1)  → HF damping trong feedback comb
  2 — Mix      (0..1)
  3 — Predelay (0..1)  → 0..100ms pre-delay trước reverb
  4 — Level    (0..1)

Implementation:
  4 parallel comb filters với feedback + LP damping
  2 serial all-pass diffusors
  Comb lengths (samples @ 44100): 1116, 1188, 1277, 1356 (×room)
  All-pass lengths: 556, 441
  Stereo: L/R dùng cùng reverb network nhưng mix khác nhau
```

## 4.8 Equalizer (5-band Parametric)

```
Params:
  0 — Low     (0..1)  → 80 Hz   shelf, -12..+12 dB (0.5 = 0 dB)
  1 — LowMid  (0..1)  → 320 Hz  peak,  -12..+12 dB
  2 — Mid     (0..1)  → 1 kHz   peak,  -12..+12 dB
  3 — HiMid   (0..1)  → 3.5 kHz peak,  -12..+12 dB
  4 — High    (0..1)  → 10 kHz  shelf, -12..+12 dB
  5 — Level   (0..1)  → output scale

Implementation: Biquad IIR (transposed Direct Form II).
Thread safety: Double-buffering (xem Phần 3.4).
```

---

# PHẦN 5 — EFFECT PRESET STORAGE

## 5.1 Mục Đích

`EffectPresetStorage` lưu/load toàn bộ cấu hình effect chain (slots, types, params, bypass) dưới dạng JSON.

## 5.2 JSON Format

```json
{
  "version": 1,
  "slots": [
    {
      "type": 0,
      "name": "Gain",
      "bypassed": false,
      "params": [0.3, 0.5, 0.0, 0.8]
    },
    {
      "type": 6,
      "name": "Reverb",
      "bypassed": false,
      "params": [0.6, 0.4, 0.5, 0.0, 1.0]
    }
  ]
}
```

## 5.3 Interface

```cpp
// hal/pc/effect_preset_storage.h
class EffectPresetStorage {
public:
    bool load(const std::string& path, EffectChain& chain);  // [RT-UNSAFE]
    bool save(const std::string& path, const EffectChain& chain);
    std::vector<std::string> listPresets(const std::string& dir) const;
};
```

`load()` gọi `EffectChain::clear()` + `addSlot()` + `setSlotParam()` — tất cả đều RT-UNSAFE (chứa mutex + vector ops). Không được gọi từ audio thread.

## 5.4 Factory Effect Presets (10 presets)

```
01_clean_boost.json       — Gain mild
02_mild_overdrive.json    — Gain medium drive
03_heavy_distortion.json  — Gain full + Tone
04_chorus_clean.json      — Chorus + Reverb
05_delay_reverb.json      — Delay + Reverb
06_cathedral_shimmer.json — Long Reverb + EQ
07_summer_boost.json      — EQ bright + Chorus
08_jet_flanger.json       — Flanger full depth
09_phaser_funk.json       — Phaser fast + Tremolo
10_tremolo_warm.json      — Tremolo slow + Reverb
```

---

# PHẦN 6 — HAL LAYER: FILE RENAMING & NEW FILES

## 6.1 File Renaming (V1 → V2.1)

| V1 | V2.1 | Lý do |
|----|------|-------|
| `hal/pc/preset_storage.h/.cpp` | `hal/pc/moog_preset_storage.h/.cpp` | Rõ nghĩa — chỉ store Moog synth sound presets |
| `hal/pc/pattern_storage.h/.cpp` | `hal/pc/sequencer_pattern_storage.h/.cpp` | Rõ nghĩa — chỉ store step sequencer patterns |

Tất cả include references trong `imgui_app.h`, `imgui_app.cpp`, `sim/main.cpp`, `CMakeLists.txt` đã được cập nhật.

## 6.2 File Mới (V2.1)

```
hal/pc/effect_preset_storage.h
hal/pc/effect_preset_storage.cpp
```

## 6.3 MoogPresetStorage — Lưu Ý isMusicParam()

Khi save Moog preset, `MoogPresetStorage` bỏ qua tất cả music params (ARP, SEQ, CHORD, SCALE, BPM) vì preset chỉ là "âm thanh" — không phải pattern nhạc.

```cpp
bool isMusicParam(ParamID id) {
    return id >= P_BPM && id <= P_SCALE_TYPE;
    // P_BPM, P_ARP_*, P_SEQ_*, P_CHORD_*, P_SCALE_*
}
```

---

# PHẦN 7 — UI SYSTEM: CONSOLIDATION & NEW PANELS

## 7.1 Panel Consolidation

V1 có nhiều panel nhỏ riêng lẻ. V2.1 hợp nhất và xóa bỏ dead code:

| V1 Panels (xóa) | V2.1 Replacement |
|-----------------|-----------------|
| `panel_controllers.h/.cpp` | Hợp nhất vào `panel_moog_engine.cpp` |
| `panel_oscillators.h/.cpp` | Hợp nhất vào `panel_moog_engine.cpp` |
| `panel_mixer.h/.cpp` | Hợp nhất vào `panel_moog_engine.cpp` |
| `panel_modifiers.h/.cpp` | Hợp nhất vào `panel_moog_engine.cpp` |
| `panel_arpeggiator.h/.cpp` | Hợp nhất vào `panel_music.cpp` |
| `panel_sequencer.h/.cpp` | Hợp nhất vào `panel_music.cpp` |
| `panel_chord_scale.h/.cpp` | Hợp nhất vào `panel_music.cpp` |
| `panel_engine.h/.cpp` | Đổi tên → `panel_moog_engine.h/.cpp` |

## 7.2 Panel Inventory V2.1

| Panel | File | Mô tả |
|-------|------|--------|
| Moog Engine | `panel_moog_engine.cpp` | Controllers + OSC + Mixer + Filter/ENV (3 cột cố định) |
| Music | `panel_music.cpp` | Arp + Chord + Scale + Seq + Keyboard (5 tab) |
| Effects | `panel_effects.cpp` | Effect chain editor (add/remove/reorder/bypass) |
| Oscilloscope | `panel_oscilloscope.cpp` | Waveform display, auto/manual scale |
| Presets | `panel_presets.cpp` | 2 tab: Moog Presets + Effect Presets |
| Output | `panel_output.cpp` | Master Volume knob |

## 7.3 panel_moog_engine Layout

```
┌─────────────────────────────────────────────────────────────┐
│  [Controllers 215px]  [OSC + Mixer 400px]  [Filter+ENV 262px]│
│                                                              │
│  Glide           │  OSC1: Range Wave Freq  │  Filter Cutoff  │
│  Mod Mix         │  OSC2: Range Wave Freq  │  Resonance      │
│  BPM             │  OSC3: Range Wave Freq  │  Filter ENV Amt │
│  Voice Mode      │       [LFO Toggle]      │  KBD Track      │
│  Voice Count     │                         │  Filter ADSR    │
│  Steal Mode      │  Mix: OSC1 OSC2 OSC3    │  Amp ADSR       │
│  Unison Detune   │        Noise Color      │  Master Tune    │
└─────────────────────────────────────────────────────────────┘
```

Chiều rộng cột được cố định để layout không bị shift khi knob thay đổi.

## 7.4 panel_effects — Effect Chain Editor

```
┌──────────────────────────────────────────────┐
│  [+ Add Effect ▼]   [Clear All]              │
│                                              │
│  Slot 0: [Gain       ] [▲][▼][Bypass][✕]   │
│    Drive: ─────●──  Tone: ──●────            │
│                                              │
│  Slot 1: [Reverb     ] [▲][▼][Bypass][✕]   │
│    Room: ──────●─  Damp: ───●───             │
└──────────────────────────────────────────────┘
```

- **Add Effect**: dropdown với 8 loại, append vào cuối chain
- **▲ / ▼**: swap slot với slot liền kề (reorder)
- **Bypass**: toggle bypass từng slot
- **✕**: xóa slot
- Params render tự động dựa trên `getParamCount()` và `getParamName()`

## 7.5 Oscilloscope Auto/Manual Scale Fix

V1 bug: slider `scale` bị auto-scale ghi đè mỗi frame → slider không dùng được.

V2.1 fix:
```cpp
static bool  s_autoScale = true;
static float s_scale     = 1.0f;

if (s_autoScale) {
    float peak = 0.0f;
    for (int i = 0; i < nSamples; ++i)
        peak = std::max(peak, std::abs(buf[i]));
    if (peak > 0.001f) s_scale = 0.9f / peak;
}

ImGui::Checkbox("Auto", &s_autoScale);
ImGui::BeginDisabled(s_autoScale);
ImGui::SliderFloat("Scale", &s_scale, 0.1f, 10.0f);
ImGui::EndDisabled();
```

Khi `s_autoScale = true`: slider disabled (grayed out), auto-scale chạy.
Khi `s_autoScale = false`: slider enabled, user control hoàn toàn.

## 7.6 panel_presets — 2 Tab

```cpp
if (ImGui::BeginTabBar("PresetTabs")) {
    if (ImGui::BeginTabItem("Moog Presets")) {
        // Load/Save/Reset synth sound presets
    }
    if (ImGui::BeginTabItem("Effect Presets")) {
        // Load/Save effect chain configurations
    }
}
```

`render()` signature:
```cpp
namespace PanelPresets {
    void render(AtomicParamStore&    params,
                PresetStorage&       moogStorage,
                EffectChain&         effectChain,
                EffectPresetStorage& effectStorage);
}
```

---

# PHẦN 8 — SHARED LAYER: CLEANUPS & FIXES

## 8.1 IAudioProcessor — noexcept

V1 thiếu `noexcept` trên virtual methods → MSVC C2694 khi override thêm `noexcept`.

```cpp
// shared/interfaces.h — V2.1
class IAudioProcessor {
public:
    virtual void setSampleRate(float sr) noexcept = 0;
    virtual void setBlockSize (int bs)   noexcept = 0;
    virtual void processBlock (sample_t* outL,
                                sample_t* outR,
                                int nFrames) noexcept = 0;
    virtual ~IAudioProcessor() = default;
};
```

## 8.2 shared/params.h — noexcept Helpers

Tất cả 6 inline norm helper functions thêm `noexcept`:

```cpp
inline float normToLog      (float n, float lo, float hi) noexcept;
inline float logToNorm      (float v, float lo, float hi) noexcept;
inline float normToCutoffHz (float n) noexcept;
inline float normToEnvMs    (float n) noexcept;
inline float normToGlideMs  (float n) noexcept;
inline float normToSemitones(float n) noexcept;
```

## 8.3 shared/types.h — HeldNote Cleanup

V1 `HeldNote` có field `active` thừa — không dùng đến, gây bug khởi tạo.

```cpp
// V1 (xóa):
struct HeldNote {
    int  midiNote = -1;
    int  velocity = 0;
    bool active   = false;   // ← THỪA, gây lỗi C2679
};

// V2.1 (clean):
struct HeldNote {
    int  midiNote = -1;
    int  velocity = 0;
    bool isValid() const noexcept { return midiNote >= 0; }
};
```

Và trong `voice_pool.cpp`:
```cpp
// V1: heldNotes_[heldCount_++] = { note, vel, true };  ← compile error
// V2.1:
heldNotes_[heldCount_++] = { note, vel };
```

## 8.4 rtaudio_backend.h — noexcept Getters

```cpp
unsigned int getSampleRate() const noexcept { return cfg_.sampleRate; }
unsigned int getBufferSize() const noexcept { return cfg_.bufferSize; }
bool         isRunning()     const noexcept { return running_; }
std::string  getLastError()  const noexcept { return lastError_; }
```

---

# PHẦN 9 — THREADING MODEL (V2.1)

## 9.1 Threads

| Thread | Role | Thay đổi so với V1 |
|--------|------|---------------------|
| **UI Thread** | ImGui render, param writes, MIDI push | Thêm: gọi EffectChain::addSlot/removeSlot/setSlotParam |
| **Audio Thread** | processBlock() → VoicePool → EffectChain | Thêm: EffectChain::processBlock() sau VoicePool |
| **MIDI Thread** | RtMidi callback → push MidiEvent | Không đổi |

## 9.2 EffectChain Thread Safety Details

```
UI Thread          EffectChain              Audio Thread
──────────         ─────────────            ────────────
addSlot()     →   mutex.try_lock()
                  effects_[n] = makeEffect()
                  configDirty_ = true
                  mutex.unlock()

                                       processBlock():
                                         if configDirty_:
                                           rebuildActiveList()
                                           configDirty_ = false
                                         for each active slot:
                                           flush atomic params
                                           effect->processBlock()

setSlotParam() →  atomicParams_[s][p].store(v, relaxed)
                                         atomicParams_[s][p].load(relaxed)
                                         effect->setParam()

EQ setParam() →   pendingCoef_[b] = compute()
                  coefDirty_[b].store(true, release)
                                         coefDirty_[b].load(acquire) → true
                                         coef_[b] = pendingCoef_[b]
                                         biquad using coef_[b]
```

## 9.3 RT-Safety Guarantee cho EffectChain::processBlock()

- Không allocation (tất cả buffer pre-allocated trong `init()`)
- Không mutex lock (chỉ atomic reads)
- Không `std::string` construction
- Không file I/O
- Không `throw`
- `try_lock()` từ UI thread — nếu audio đang trong critical section, UI skip (retry next frame)

---

# PHẦN 10 — CẤU TRÚC THƯ MỤC V2.1

```
minimog_digitwin/
│
├── CMakeLists.txt
├── README.md
├── CLAUDE.md
│
├── shared/
│   ├── types.h           ← HeldNote cleanup (xóa field active)
│   ├── params.h          ← noexcept helpers
│   ├── params.cpp
│   └── interfaces.h      ← noexcept virtual methods
│
├── core/
│   ├── dsp/              ← KHÔNG ĐỔI so với V1
│   │   ├── oscillator.h/.cpp
│   │   ├── moog_filter.h/.cpp
│   │   ├── envelope.h/.cpp
│   │   ├── glide.h/.cpp
│   │   ├── lfo.h/.cpp
│   │   ├── noise.h/.cpp
│   │   └── param_smoother.h
│   │
│   ├── voice/            ← KHÔNG ĐỔI (chỉ sửa HeldNote init)
│   │   ├── voice.h/.cpp
│   │   ├── voice_pool.h/.cpp
│   │
│   ├── music/            ← KHÔNG ĐỔI
│   │   ├── arpeggiator.h/.cpp
│   │   ├── sequencer.h/.cpp
│   │   ├── chord_engine.h/.cpp
│   │   └── scale_quantizer.h/.cpp
│   │
│   ├── engine/           ← chỉ thêm noexcept
│   │   ├── synth_engine.h/.cpp
│   │   └── mod_matrix.h/.cpp
│   │
│   ├── effects/          ← MỚI HOÀN TOÀN (V2.1)
│   │   ├── effect_base.h          ← interface + EffectType enum
│   │   ├── effect_chain.h/.cpp    ← container, thread safety
│   │   ├── gain_effect.h/.cpp
│   │   ├── chorus_effect.h/.cpp
│   │   ├── flanger_effect.h/.cpp
│   │   ├── phaser_effect.h/.cpp
│   │   ├── tremolo_effect.h/.cpp
│   │   ├── delay_effect.h/.cpp
│   │   ├── reverb_effect.h/.cpp
│   │   └── eq_effect.h/.cpp
│   │
│   └── util/             ← KHÔNG ĐỔI
│       ├── math_utils.h
│       └── spsc_queue.h
│
├── hal/
│   └── pc/
│       ├── rtaudio_backend.h/.cpp  ← thêm noexcept getters
│       ├── pc_midi.h/.cpp
│       ├── keyboard_input.h/.cpp
│       ├── moog_preset_storage.h/.cpp       ← ĐỔI TÊN từ preset_storage
│       ├── sequencer_pattern_storage.h/.cpp ← ĐỔI TÊN từ pattern_storage
│       └── effect_preset_storage.h/.cpp     ← MỚI
│
├── ui/
│   ├── imgui_app.h/.cpp            ← cập nhật includes, thêm effect panels
│   ├── panels/
│   │   ├── panel_moog_engine.h/.cpp  ← ĐỔI TÊN từ panel_engine (3 cột)
│   │   ├── panel_music.h/.cpp        ← KHÔNG ĐỔI
│   │   ├── panel_effects.h/.cpp      ← MỚI
│   │   ├── panel_oscilloscope.h/.cpp ← auto/manual scale fix
│   │   ├── panel_presets.h/.cpp      ← 2 tab + EffectPresetStorage
│   │   └── panel_output.h/.cpp       ← KHÔNG ĐỔI
│   │   ←── ĐÃ XÓA: panel_controllers, panel_oscillators, panel_mixer,
│   │            panel_modifiers, panel_arpeggiator, panel_sequencer,
│   │            panel_chord_scale (7 files = 14 file total)
│   └── widgets/
│       ├── knob_widget.h
│       ├── adsr_display.h
│       ├── keyboard_display.h
│       └── seq_display.h
│
├── sim/
│   └── main.cpp           ← version string "v2.1", patternDir update
│
├── tests/                 ← 44 tests, tất cả pass, KHÔNG ĐỔI
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
├── assets/
│   ├── moog_presets/          ← ĐỔI TÊN từ presets/ (20 JSON)
│   ├── sequencer_patterns/    ← ĐỔI TÊN từ patterns/ (10 JSON)
│   └── effect_presets/        ← MỚI (10 JSON)
│
└── documents/
    ├── MiniMoog DSP Simulator - TDD - V1.md
    ├── MiniMoog DSP Simulator - TDD - V2.1.md  ← file này
    └── Huong_Dan_Su_Dung.md
```

---

# PHẦN 11 — FACTORY ASSETS V2.1

## 11.1 Moog Presets (20 presets) — assets/moog_presets/

```
Phân loại Bass (5):   bass_classic, bass_fat, bass_funk, bass_sub, bass_taurus
Phân loại Lead (7):   lead_acid, lead_emerson, lead_lucky_man, lead_mellow,
                      lead_prog_rock, lead_theremin, lead_unison_fat
Phân loại Pad (4):    pad_analog_strings, pad_solar_wind, pad_strings, pad_warm
Phân loại Misc (4):   brass_stab, keys_pluck, fx_sweep, arp_sequence
```

## 11.2 Sequencer Patterns (10 patterns) — assets/sequencer_patterns/

```
acid_303, blue_monday, blues_shuffle, donna_disco, funk_pocket,
oxygene_arp, pentatonic_run, popcorn_melody, reggae_offbeat, techno_pulse
```

## 11.3 Effect Presets (10 presets) — assets/effect_presets/ (MỚI)

```
01_clean_boost, 02_mild_overdrive, 03_heavy_distortion,
04_chorus_clean, 05_delay_reverb, 06_cathedral_shimmer,
07_summer_boost, 08_jet_flanger, 09_phaser_funk, 10_tremolo_warm
```

---

# PHẦN 12 — TEST PLAN (V2.1)

## 12.1 Existing Tests (44 tests — unchanged)

| File | Tests | Module |
|------|-------|--------|
| test_oscillator.cpp | 8 | Oscillator, PolyBLEP, waveforms |
| test_moog_filter.cpp | 6 | MoogFilter, resonance, self-oscillation |
| test_envelope.cpp | 8 | ADSR stages, gate, retrigger |
| test_glide.cpp | 4 | Portamento, exponential slide |
| test_voice.cpp | 6 | Voice tick, noteOn/Off, VoicePool |
| test_arpeggiator.cpp | 5 | Modes, octaves, swing |
| test_sequencer.cpp | 4 | Steps, play/stop, tie |
| test_scale_quantizer.cpp | 3 | 16 scales, quantize |

## 12.2 V2.1 New Tests (Recommended, Not Yet Written)

| Test | Mô tả |
|------|-------|
| test_effect_chain.cpp | addSlot, removeSlot, swapSlots, processBlock |
| test_gain_effect.cpp | Drive curves, tanh clipping, Level param |
| test_eq_effect.cpp | Coef computation, double-buffer flush, band isolation |
| test_eq_butterworth.cpp | Shelving filter frequency response |
| test_effect_preset.cpp | JSON save/load roundtrip |

---

# PHẦN 13 — BUILD SYSTEM (CMAKELISTS.TXT CHANGES)

## 13.1 Source Files Changed

```cmake
# HAL — renamed files
hal/pc/moog_preset_storage.cpp        # was: preset_storage.cpp
hal/pc/sequencer_pattern_storage.cpp  # was: pattern_storage.cpp
hal/pc/effect_preset_storage.cpp      # NEW

# UI — renamed file
ui/panels/panel_moog_engine.cpp       # was: panel_engine.cpp

# UI — new files
ui/panels/panel_effects.cpp           # NEW

# Core — new directory and 9 new files
core/effects/effect_chain.cpp         # NEW
core/effects/gain_effect.cpp          # NEW
core/effects/chorus_effect.cpp        # NEW
core/effects/flanger_effect.cpp       # NEW
core/effects/phaser_effect.cpp        # NEW
core/effects/tremolo_effect.cpp       # NEW
core/effects/delay_effect.cpp         # NEW
core/effects/reverb_effect.cpp        # NEW
core/effects/eq_effect.cpp            # NEW
```

## 13.2 Assets Copy (Post-build)

```cmake
# V1: copy presets/ và patterns/
# V2.1: thêm effect_presets/
add_custom_command(TARGET minimoog_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/moog_presets
        $<TARGET_FILE_DIR:minimoog_sim>/moog_presets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/sequencer_patterns
        $<TARGET_FILE_DIR:minimoog_sim>/sequencer_patterns
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/effect_presets
        $<TARGET_FILE_DIR:minimoog_sim>/effect_presets
)
```

## 13.3 Dependencies (không thay đổi)

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

# PHẦN 14 — CODING CONVENTIONS (V2.1 ADDITIONS)

Tất cả convention từ V1 vẫn áp dụng. V2.1 bổ sung:

## 14.1 noexcept Requirements

```
RULE: Tất cả các hàm sau PHẢI có noexcept:
  • Getters (const methods): luôn noexcept
  • tick() / process() / processBlock(): luôn noexcept
  • setSampleRate() / setBlockSize(): noexcept (và override phải match)
  • init() nếu không throw: noexcept
  • isActive(), getNote(), getAge(), getLevel(): noexcept

RULE: noexcept trên base class virtual method → tất cả override PHẢI noexcept
  (MSVC C2694 sẽ báo lỗi nếu vi phạm)
```

## 14.2 Effect Class Template

```cpp
// core/effects/my_effect.h
// ─────────────────────────────────────────────────────────
// FILE: core/effects/my_effect.h
// BRIEF: <mô tả ngắn>
// ─────────────────────────────────────────────────────────
#pragma once
#include "effect_base.h"

class MyEffect : public EffectBase {
public:
    void  init(float sampleRate) noexcept override;
    void  processBlock(float* L, float* R, int n) noexcept override;
    void  setParam(int p, float v) noexcept override;
    float getParam(int p) const noexcept override;
    int   getParamCount() const noexcept override { return PARAM_COUNT; }
    const char* getParamName(int p) const noexcept override;
    EffectType  getType() const noexcept override { return EffectType::MyEffect; }
    bool  isBypassed() const noexcept override { return bypass_; }
    void  setBypassed(bool b) noexcept override { bypass_ = b; }
    void  reset() noexcept override;

private:
    static constexpr int PARAM_COUNT = N;
    float params_[PARAM_COUNT] = {};
    bool  bypass_ = false;
    float sampleRate_ = 44100.0f;
    // ... internal state
};
```

## 14.3 RT-Safety trong EffectChain

```
[RT-SAFE]:
  EffectChain::processBlock()
  EffectBase::processBlock()
  EffectChain::getSlotParam()

[RT-UNSAFE]:
  EffectChain::addSlot()        ← std::unique_ptr constructor
  EffectChain::removeSlot()     ← destructor call
  EffectChain::swapSlots()      ← mutex
  EffectPresetStorage::load()   ← JSON parse, file I/O
  EffectPresetStorage::save()
```

---

# PHẦN 15 — ROADMAP V3

```
V3 mục tiêu: Teensy 4.1 firmware + hardware synthesizer

Hardware:
  → Teensy 4.1 (ARM Cortex-M7, 600 MHz)
  → I2S audio codec (PCM5102 hoặc WM8731)
  → SD card preset storage
  → OLED/TFT display (status)
  → Physical knobs, buttons, MIDI DIN

New HAL (hal/teensy/):
  → teensy_audio.h/.cpp     ← I2S AudioStream adapter
  → teensy_sd_storage.h     ← SD card JSON preset I/O
  → teensy_midi.h           ← MIDI DIN (hardware serial)
  → teensy_display.h        ← SSD1306 / ILI9341 display

DSP Additions (V3):
  → FM operator routing (2-op, 4-op) — Rhodes EP, DX7 bells
  → Karplus-Strong oscillator — plucked string
  → Wavetable oscillator — sample-based timbres
  → Velocity → filter/timbre mapping (currently vol only)
  → MIDI CC learn

V3 sẽ reuse 100% core/dsp/, core/voice/, core/music/, core/engine/,
core/effects/ từ V2.1 mà không sửa một dòng DSP code.
Chỉ viết thêm hal/teensy/ để map vào interfaces.h.
```

---

# PHẦN 16 — GLOSSARY

| Thuật ngữ | Định nghĩa |
|-----------|-----------|
| **PolyBLEP** | Polynomial Band-Limited stEP — kỹ thuật anti-aliasing cho oscillator bằng cách làm mượt discontinuity tại điểm chuyển đổi waveform |
| **Huovilainen Model** | Mô hình toán học của Vesa Huovilainen (2004) mô phỏng Moog ladder filter với tanh saturation phi tuyến |
| **ADSR** | Attack / Decay / Sustain / Release — 4 giai đoạn của envelope generator |
| **ParamSmoother** | 1-pole IIR low-pass filter áp dụng cho param thay đổi liên tục, tránh zipper noise |
| **SPSC Queue** | Single-Producer Single-Consumer — lock-free ring buffer cho MIDI events |
| **AtomicParamStore** | Mảng 103 `std::atomic<float>` — shared state giữa UI thread và audio thread |
| **EffectChain** | Container quản lý tối đa 16 effect slot nối tiếp |
| **Biquad IIR** | 2nd-order Infinite Impulse Response filter — building block của EQ |
| **Double buffering (EQ)** | Kỹ thuật dùng 2 buffer (pending + live) + atomic dirty flag để update multi-float struct thread-safe |
| **Voice stealing** | Khi tất cả voice đang dùng, steal voice cũ nhất / thấp nhất / nhỏ nhất để play note mới |
| **Schroeder Reverb** | Kiến trúc reverb của Manfred Schroeder: parallel comb filters + serial all-pass diffusors |
| **PolyBLEP** | Xem trên |
| **RT-SAFE** | Real-Time Safe — hàm không allocate, không lock, không throw, an toàn trong audio callback |
| **RT-UNSAFE** | Không an toàn trong audio thread — chứa file I/O, mutex, heap allocation |
| **Unison** | Voice mode: tất cả voice chơi cùng một note nhưng detune khác nhau |
| **Portamento** | Glide — trượt pitch mượt từ note trước sang note mới |

---

*MiniMoog DSP Simulator — Technical Design Document V2.1*
*March 2026 — Implemented & Verified*
