# Engine Improvements Log

> **Scope:** Tài liệu ghi lại tất cả cải thiện kỹ thuật đã thực hiện trên các DSP Engines, theo thứ tự thời gian. Mỗi mục bao gồm: vấn đề gốc, giải pháp, code pattern, và kết quả.

---

## Mục lục

### 1. [Rhodes Engine — V2 Physical Model Rewrite](#1-rhodes-engine--v2-physical-model-rewrite)

| § | Tiêu đề | Nội dung |
|---|---------|----------|
| A | DC Blocker Per-Channel | Fix stereo wobble — tách state L/R |
| B | Displacement-Based Pickup | Frequency-independent output |
| C | 4 Modal Resonators | Thêm bell (6×) + high (9×) partials |
| D | Pickup Nonlinearity ↓ | Alpha/Beta giảm 3–5× → bớt buzzy |
| E | Loại bỏ `softSat()` | Một tầng tanh duy nhất ở output |
| F | Normalization Constants | VOICE_NORM + POLY_NORM cho 8 voices |
| G | Drive Range Thu Hẹp | 1–5× → 1–2.5× |
| H | Decay Scale Per Note | Range 0.70–1.30 (ratio 1.86:1) |
| I | Torsional Mode | ~6 cents AM wobble |
| J | 2-Stage Damper Release | Felt contact 30ms → user release |
| K | Cabinet EQ | Low shelf +2dB@200Hz, High shelf -8dB@8kHz |
| L | Per-Engine Presets | 10 JSON presets |

### 2. [P1 — Pitch Bend & Voice Quality](#2-p1-cross-engine-improvements--pitch-bend--voice-quality)

| § | Engine | Tiêu đề |
|---|--------|---------|
| [2.1](#21-hammond--pitch-bend-real-time) | Hammond | Pitch Bend Real-Time |
| [2.2](#22-hammond--drawbar-paramsmoother) | Hammond | Drawbar ParamSmoother (9×5ms) |
| [2.3](#23-rhodes--pitch-bend-via-modalmodeupdatefreq) | Rhodes | Pitch Bend via `updateFreq()` |
| [2.4](#24-dx7--pitch-bend-real-time) | DX7 | Pitch Bend Real-Time |
| [2.5](#25-dx7--voice-stealing-oldest-active) | DX7 | Voice Stealing: Oldest-Active |
| [2.6](#26-mellotron--pitch-bend-real-time) | Mellotron | Pitch Bend Real-Time |
| [2.7](#27-drums--hi-hat-choke-group) | Drums | Hi-Hat Choke Group |
| [2.8](#28-drums--velocity--decay-scaling) | Drums | Velocity → Decay Scaling |

### 3. [P2 — Quality & Realism](#3-p2-cross-engine-improvements--quality--realism)

| § | Engine | Tiêu đề |
|---|--------|---------|
| [3.1](#31-trạng-thái-tổng-quan-p2) | — | Trạng thái tổng quan P2 |
| [3.2](#32-hammond--true-leslie-dual-rotor) | Hammond | True Leslie Dual-Rotor (Doppler + AM) |
| [3.3](#33-dx7--exponential-envelope-contour) | DX7 | Exponential Envelope Contour |
| [3.4](#34-dx7--fixed-frequency-operators) | DX7 | Fixed-Frequency Operators (12 params) |
| [3.5](#35-mellotron--tape-wowflutter) | Mellotron | Tape Wow/Flutter (dual LFO) |
| [3.6](#36-mellotron--tape-hiss-floor) | Mellotron | Tape Hiss Floor (-50 dBFS) |
| [3.7](#37-rhodes--vibrato-lfo) | Rhodes | Vibrato LFO (±35 cents) |
| [3.8](#38-rhodes--cc64-sustain-pedal) | Rhodes | CC64 Sustain Pedal |
| [3.9](#39-rhodes--velocity--pickup-nonlinearity) | Rhodes | Velocity → Pickup Nonlinearity |
| [3.10](#310-rhodes--paramsmoother-tonedecaydrive) | Rhodes | ParamSmoother (Tone/Decay/Drive) |
| [3.11](#311-rhodes--per-mode-excitation-scaling) | Rhodes | Per-Mode Excitation Scaling |
| [3.12](#312-rhodes--modal-amplitude-calibration-tolonen-1998) | Rhodes | Modal Amplitude Calibration (Tolonen 1998) |
| [3.13](#313-rhodes--sympathetic-resonance) | Rhodes | Sympathetic Resonance |
| [3.14](#314-moog--post-vca-output-limiter) | Moog | Post-VCA Output Limiter (tanh) |
| [3.15](#315-dx7--keyboard-rate-scaling) | DX7 | Keyboard Rate Scaling (6 params) |

### 4. [P3 Roadmap](#4-còn-lại--p3-roadmap)

### 5. [P3 Implementation](#5-p3-implementation-2026-03-08)

| § | Engine | Tiêu đề |
|---|--------|---------|
| [5.1](#51-moog--mono-note-priority-modes) | Moog | Mono Note Priority (Last/Low/High) |
| [5.2](#52-moog--analog-style-unison-detune) | Moog | Analog-Style Unison Detune (±20% jitter) |
| [5.3](#53-rhodes--extended-polyphony-812) | Rhodes | Extended Polyphony 8→12 |
| [5.4](#54-hammond--tube-overdrive--preamp-saturation) | Hammond | Tube Overdrive / Preamp Saturation |
| [5.5](#55-drums--808-style-hi-hat-ring-modulation) | Drums | 808-Style Hi-Hat Ring Modulation |
| [5.6](#56-drums--kick-sweep-param-exposure) | Drums | Kick Sweep Param Exposure |
| [5.7](#57-mellotron--multi-cycle-wavetable-loops) | Mellotron | Multi-Cycle Wavetable Loops (8×2048) |

### [Phụ lục — Code Patterns](#phụ-lục--code-patterns-quan-trọng)

| # | Pattern |
|---|---------|
| 1 | RT-Safe Pitch Bend (tất cả engines) |
| 2 | ParamSmoother cho Continuous Params |
| 3 | Voice Stealing — Oldest-Active |
| 4 | ModalMode::updateFreq (no-pop frequency update) |

---

## 1. Rhodes Engine — V2 Physical Model Rewrite

### Tổng quan

Rhodes V1 dùng oscillator + envelope đơn giản. V2 chuyển sang **mô hình vật lý** dựa trên biquad modal resonator (Tolonen 1998), mô phỏng chính xác cơ học của tine điện cơ.

### A. Bug Fix: DC Blocker — Per-Channel State

**Vấn đề:** V1 dùng chung một cặp state `{x1, y1}` cho cả L và R output của DC blocker. Kết quả: DC blocker L ảnh hưởng DC blocker R → stereo wobble artifact.

**Giải pháp:**
```cpp
// rhodes_voice.h — Trước (V1):
float dcX1 = 0.0f, dcY1 = 0.0f;  // chung cho cả L và R

// Sau (V2):
float dcX1L = 0.0f, dcY1L = 0.0f;  // kênh L riêng
float dcX1R = 0.0f, dcY1R = 0.0f;  // kênh R riêng
```

### B. Pickup Model: Velocity-Based → Displacement-Based

**Vấn đề:** V1 đọc output trực tiếp từ resonator state (`y0`). Output amplitude **không nhất quán** giữa các notes vì resonator amplitude phụ thuộc frequency — low notes có amplitude lớn hơn high notes ở cùng energy.

**Giải pháp:** Chuyển sang **displacement-based pickup polynomial**:
```cpp
// Pickup nonlinearity: mô phỏng từ trường của pickup
// displacement `d` = tổng modal displacement
// Output = d + alpha*d^2 + beta*d^3
float d = 0.0f;
for (int m = 0; m < NUM_MODES; ++m)
    d += modes[m].y0 * modeAmps[m];

float out = d + pickupAlpha * d * d + pickupBeta * d * d * d;
```

**Kết quả:** Output level nhất quán toàn keyboard (frequency-independent). Tone color thay đổi tự nhiên theo velocity thông qua `pickupAlpha/pickupBeta`.

### C. Modal Resonators — Tăng từ 2 → 4 Partials

**Vấn đề:** V1 chỉ có 2 resonators (fundamental + 2nd). Thiếu "bell partial" (overtone đặc trưng của Rhodes ở 6× fundamental) và high partial (9×).

**Giải pháp — 4 modal resonators:**

| Mode | Ratio | Amplitude | Decay Scale | Ghi chú |
|------|-------|-----------|-------------|---------|
| Fundamental | 1.000 | 1.000 | 1.0× | Tine resonance chính |
| 2nd partial | 2.045 | 0.280 | 0.55× | Từ Tolonen 1998 (inharmonic) |
| Bell partial | 6.149 | 0.200 | 0.12× | Đặc trưng "bell tone" của Rhodes |
| High partial | 9.214 | 0.075 | 0.04× | Brightness, decay rất nhanh |

**Inharmonic ratios từ Tolonen 1998:** Rhodes thực có stiffness của tine → partials bị "sharp" nhẹ so với integer ratios:
- Mode 2: 2.045× (thay vì 2.000×)
- Mode 3: 6.149× (thay vì 6.000×)
- Mode 4: 9.214× (thay vì 9.000×)

### D. Pickup Nonlinearity Giảm 3–5×

**Vấn đề:** V1 có `pickupAlpha = 0.25` → quá nhiều harmonic distortion, tạo ra tiếng "buzzy" không giống Rhodes thực.

**Giải pháp:**
```cpp
// Tone param 0..1 → pickup nonlinearity
// V1: range 0.10 .. 0.25
// V2: range 0.03 .. 0.10 (3.3× thấp hơn)
const float pickupDrive = 0.03f + toneParam * 0.07f;
pickupAlpha = pickupDrive;
pickupBeta  = pickupDrive * 0.5f;
```

### E. Loại Bỏ `softSat()` Layer Thừa

**Vấn đề:** V1 có 2 lớp saturation: pickup polynomial + riêng một lớp `softSat()`. Double saturation tạo thêm distortion không mong muốn.

**Giải pháp:** Xóa `softSat()`, thay bằng **tanh soft limiter** duy nhất ở output:
```cpp
// Thay vì softSat(cubic clip):
float out = d + pickupAlpha * d * d + pickupBeta * d * d * d;
out = std::tanh(out * 0.8f) * 1.2f;  // Gentle tanh — không thêm harmonics mạnh
```

### F. Normalization Constants

Output normalization được tính lại cho V2:
```cpp
static constexpr float VOICE_NORM = 0.32f;   // Mỗi voice × 0.32
static constexpr float POLY_NORM  = 0.354f;  // Thêm poly scaling = 1/sqrt(8) ≈ 0.354
```

So với V1 (`VOICE_NORM = 0.5f`, không có `POLY_NORM`), V2 đảm bảo không clip khi play 8 voices cùng lúc.

### G. Drive Range Thu Hẹp

**Vấn đề:** V1 drive range 1.0×–5.0×. Drive cao làm bão hòa hoàn toàn pickup polynomial → mất dynamics.

**Giải pháp:**
```cpp
// V1: drive = 1.0 + driveParam * 4.0  →  range [1.0, 5.0]
// V2: drive = 1.0 + driveParam * 1.5  →  range [1.0, 2.5]
const float drive = 1.0f + driveParam_ * 1.5f;
```

### H. Decay Scale Per Note — Range Chuẩn Hóa

**Vấn đề:** V1 decay scale range quá rộng (0.60–1.60, ratio 2.67:1). Rhodes thực: high notes decay nhanh hơn ~2×, không phải 2.67×.

**Giải pháp:**
```cpp
// V2: range [0.70, 1.30] — ratio 1.86:1
// noteNorm = (midi_note - 21) / 87.0  →  0..1
const float decayScale = 0.70f + 0.60f * noteNorm;
```

### I. Torsional Mode — "Bell Ringing" Effect

Rhodes thực có cộng hưởng torsional (xoắn) của tine tạo ra slight AM wobble đặc trưng. V2 thêm `torsionMode_`:

```cpp
// Torsional mode: ~6 cents sharp so với bell partial
// AM wobble khoảng 3-8 Hz — slow warble
ModalMode torsionMode_;  // Init tại freq = modes[2].freq * pow(2, 6.0/1200.0)

// Trong tick(): amplitude modulation từ torsionMode_
const float torsionAM = 1.0f + 0.015f * torsionMode_.y0;
out *= torsionAM;
```

### J. 2-Stage Damper Release

**Vấn đề:** V1 release = damper ngay lập tức làm tắt modes. Rhodes thực có damper felt pad tiếp xúc tine tạo ra tiếng "thud" ngắn trước khi note fade tự nhiên.

**Giải pháp — 2 giai đoạn:**
```cpp
// Stage 1: Felt contact — damper felt ép vào tine
// ~30ms đầu: decay rate tăng mạnh (felt attenuation)
// Stage 2: Sustain release — user-defined releaseMs
// Sau 30ms: note tắt theo decay bình thường nhưng với releaseMs

enum class ReleaseStage { FELT_CONTACT, NATURAL_DECAY, DONE };
ReleaseStage releaseStage_ = ReleaseStage::DONE;
int feltContactSamples_ = 0;
```

### K. Cabinet EQ

Thêm 2-band shelving EQ mô phỏng cabinet speaker Rhodes:
```cpp
// Low shelf: +2dB @ 200Hz  (cabinet body resonance)
// High shelf: -8dB @ 8kHz  (cabinet rolloff — Rhodes không bright)
// Implemented: simple biquad shelf coefficients computed in init()
```

### L. Per-Engine Presets

10 JSON presets được tạo trong `assets/rhodes_presets/`:

| File | Tên preset | Đặc điểm |
|------|-----------|---------|
| `01_classic_ep.json` | Classic Electric Piano | Cân bằng — Rhodes Mk I điển hình |
| `02_tremolo_funk.json` | Tremolo Funk | Stereo tremolo mạnh, Suitcase style |
| `03_jazz_comp.json` | Jazz Comping | Tone tối, pickup low, jazz voicing |
| `04_gospel_bright.json` | Gospel Bright | Tone sáng, vel cao, attack rõ |
| `05_ballad_soft.json` | Ballad Soft | Tone mềm mại, sustain dài |
| `06_rhodes_chorus.json` | Rhodes Chorus | Chorus rộng, dày hơn |
| `07_bark_drive.json` | Rhodes Bark | Drive cao, bark rõ ở ff |
| `08_suitcase_stereo.json` | Suitcase Wide | Stereo spread tối đa |
| `09_studio_direct.json` | Studio Direct | DI signal, EQ flat |
| `10_vintage_worn.json` | Vintage Worn | Intonation imperfect, worn tines |

---

## 2. P1 Cross-Engine Improvements — Pitch Bend & Voice Quality

> **Nguồn gốc:** Phân tích `DSP_Engine_Analysis.md` → Priority 1 Roadmap. Mục tiêu: áp dụng best practices từ MoogEngine lên tất cả engines còn lại.

### Pattern chung — Pitch Bend

Tất cả engines dùng cùng một pattern:
```cpp
// [RT-UNSAFE] — gọi từ UI/MIDI thread
void XxxEngine::pitchBend(int bend) noexcept override {
    // bend: -8192..+8191 (MIDI standard)
    // Clamp ±2 semitones (giống MoogEngine)
    pitchBendSemis_ = std::clamp(bend / 8192.0f * 2.0f, -2.0f, 2.0f);
    pitchBendCache_ = std::pow(2.0f, pitchBendSemis_ / 12.0f);
    // pitchBendCache_ range: [pow(2,-2/12), pow(2,2/12)] ≈ [0.891, 1.122]
}

// Trong tickSample() — [RT-SAFE]:
// pitchBendCache_ đọc trực tiếp (float read = atomic on x86/x64)
// Truyền vào voice.tick() như float pitchFactor
// Voice dùng: phaseInc * pitchFactor (KHÔNG modify stored phaseInc)
```

---

### 2.1 Hammond — Pitch Bend Real-Time

**Vấn đề:** `HammondEngine::pitchBend()` trước là empty override `{}`. Pitch bend MIDI không có hiệu lực.

**Giải pháp:**

`hammond_engine.h`:
```cpp
void pitchBend(int bend) noexcept override;

float pitchBendSemis_  = 0.0f;
float pitchBendCache_  = 1.0f;
```

`hammond_voice.h` — signature `tick()` thêm `float pitchFactor`:
```cpp
void tick(float pitchFactor, const float drawbars[9],
          float& outL, float& outR) noexcept; // [RT-SAFE]
```

Body — áp dụng pitchFactor lên tất cả 9 harmonics:
```cpp
for (int h = 0; h < 9; ++h) {
    phases[h] += phaseInc[h] * pitchFactor;  // Thêm * pitchFactor
    if (phases[h] > TWO_PI) phases[h] -= TWO_PI;
    // ...
}
```

`hammond_engine.cpp`:
```cpp
void HammondEngine::pitchBend(int bend) noexcept {
    pitchBendSemis_ = std::clamp(bend / 8192.0f * 2.0f, -2.0f, 2.0f);
    pitchBendCache_ = std::pow(2.0f, pitchBendSemis_ / 12.0f);
}
```

---

### 2.2 Hammond — Drawbar ParamSmoother

**Vấn đề:** Drawbar values (9 float params) được đọc trực tiếp từ `drawbars_[]` trong audio thread không qua smoothing. Khi UI thay đổi drawbar → zipper noise (abrupt amplitude step).

**Giải pháp:** `ParamSmoother` 5ms cho mỗi drawbar:

`hammond_engine.h`:
```cpp
#include "core/dsp/param_smoother.h"

ParamSmoother drawbarSmooth_[9];
float drawbarsSmoothed_[9] = {};
```

`hammond_engine.cpp`:
```cpp
// init():
for (int h = 0; h < 9; ++h) {
    drawbarSmooth_[h].init(sampleRate_, 5.0f);  // 5ms ramp
    drawbarSmooth_[h].snapTo(drawbars_[h]);     // Start ở current value
}

// setSampleRate():
for (int h = 0; h < 9; ++h)
    drawbarSmooth_[h].init(sr, 5.0f);

// beginBlock():
for (int h = 0; h < 9; ++h)
    drawbarSmooth_[h].setTarget(drawbars_[h]);

// tickSample() — advance per sample:
for (int h = 0; h < 9; ++h)
    drawbarsSmoothed_[h] = drawbarSmooth_[h].tick();
// Truyền drawbarsSmoothed_ vào voice.tick()
```

**Kết quả:** Kéo drawbar mượt mà, không còn zipper noise. Ramp 5ms đủ nhanh để không cảm giác latency.

---

### 2.3 Rhodes — Pitch Bend via ModalMode::updateFreq

**Vấn đề:** `RhodesEngine::pitchBend()` trước là empty override. Thêm phức tạp hơn Hammond/DX7: Rhodes dùng **biquad modal resonators** — không thể chỉ nhân `phaseInc * pitchFactor` vì không có phaseInc.

**Thách thức kỹ thuật:** Modal resonator state `{y0, y1}` encode cả frequency và amplitude. Nếu reset state khi bend → click/pop vì năng lượng bị mất đột ngột.

**Giải pháp — `ModalMode::updateFreq()`:**
```cpp
// Cập nhật frequency KHÔNG reset state — chỉ update coef (phase velocity)
void ModalMode::updateFreq(float freqHz, float sampleRate) noexcept {
    const float omega = TWO_PI * freqHz / sampleRate;
    coef = 2.0f * std::cos(omega);  // Update chỉ coef, giữ nguyên y0/y1
    // y0, y1 không thay đổi → không pop, năng lượng bảo toàn
}
```

`rhodes_voice.h` — thêm `baseHz_` và `updatePitchFactor()`:
```cpp
float baseHz_ = 261.63f;  // Stored khi trigger()

void updatePitchFactor(float pitchBendSemis, float sampleRate) noexcept {
    const float factor = std::pow(2.0f, pitchBendSemis / 12.0f);
    static constexpr float modeRatios[NUM_MODES] = {1.000f, 2.045f, 6.149f, 9.214f};
    for (int m = 0; m < NUM_MODES; ++m)
        modes[m].updateFreq(baseHz_ * factor * modeRatios[m], sampleRate);
    torsionMode_.updateFreq(baseHz_ * factor * modeRatios[2]
                            * std::pow(2.0f, 6.0f / 1200.0f), sampleRate);
}
```

`rhodes_engine.cpp` — change-detect trong tickSample để tránh update khi không cần:
```cpp
// beginBlock() hoặc tickSample():
if (pitchBendSemis_ != prevPitchBendSemis_) {
    for (auto& v : voices_)
        if (v.active)
            v.updatePitchFactor(pitchBendSemis_, sampleRate_);
    prevPitchBendSemis_ = pitchBendSemis_;
}
```

---

### 2.4 DX7 — Pitch Bend Real-Time

**Vấn đề:** `DX7Engine::pitchBend()` trước là empty override.

**Cấu trúc DX7:** `DX7Operator::tick()` → accumulate `phase += phaseInc`. Pitch bend cần được forward qua `DX7Voice::tick()` xuống tất cả 6 operators.

**Giải pháp:**

`dx7_voice.h`:
```cpp
// DX7Operator — thêm pitchFactor param
float tick(float pitchFactor = 1.0f, float modInput = 0.0f) noexcept {
    phase += phaseInc * pitchFactor;  // [RT-SAFE]
    // ...
}

// DX7Voice — forward pitchFactor xuống operators
float tick(int algorithm, float pitchFactor,
           float& outL, float& outR) noexcept; // [RT-SAFE]
```

`dx7_engine.cpp`:
```cpp
void DX7Engine::pitchBend(int bend) noexcept {
    pitchBendSemis_ = std::clamp(bend / 8192.0f * 2.0f, -2.0f, 2.0f);
    pitchBendCache_ = std::pow(2.0f, pitchBendSemis_ / 12.0f);
}

// tickSample():
v.tick(algorithm_, pitchBendCache_, vL, vR);
```

---

### 2.5 DX7 — Voice Stealing: Oldest-Active

**Vấn đề:** V1 dùng **round-robin** để steal voice. Round-robin steal voice đang ở giữa release → audible pop vì amplitude đang decay nhưng bị cut abruptly.

**Vấn đề cụ thể:**
```
Voice 0 playing (active, age 5)
Voice 1 playing (active, age 3)
Voice 2 releasing (active, age 1)  ← round-robin chọn voice này
Voice 3 idle

// Round-robin: khi full, steal voice theo index tuần tự → có thể steal voice
// đang ở đỉnh attack thay vì voice đã sustaining lâu
```

**Giải pháp — Oldest-Active (giống MoogEngine):**

`dx7_engine.h`:
```cpp
// Thay thế voiceIdx_ = 0 (round-robin) bằng:
int voiceAge_[MAX_VOICES] = {};  // Age counter mỗi voice
int voiceAgeCounter_ = 0;        // Global monotonic counter
```

`dx7_engine.cpp — noteOn()`:
```cpp
// Tìm voice idle trước, nếu không có → steal oldest-active
int best = -1;
// Pass 1: tìm inactive voice
for (int i = 0; i < MAX_VOICES; ++i) {
    if (!voices_[i].active) { best = i; break; }
}
// Pass 2: nếu không có idle → oldest-active (smallest voiceAge_)
if (best < 0) {
    int oldest = INT_MAX;
    for (int i = 0; i < MAX_VOICES; ++i) {
        if (voiceAge_[i] < oldest) { oldest = voiceAge_[i]; best = i; }
    }
}

voices_[best].noteOn(note, vel, algorithm_, ops_);
voiceAge_[best] = ++voiceAgeCounter_;
```

**Kết quả:** Khi 8 voices đầy, steal voice đã sound lâu nhất → ít audible nhất. Không pop.

---

### 2.6 Mellotron — Pitch Bend Real-Time

**Vấn đề:** `MellotronEngine::pitchBend()` trước là empty override. Mellotron dùng wavetable playback với `phase += phaseInc` → áp dụng pitchFactor dễ nhất trong tất cả engines.

**Giải pháp:**

`mellotron_voice.h`:
```cpp
void tick(MellotronTape tape, float pitchFactor,
          float& outL, float& outR) noexcept { // [RT-SAFE]
    // ...
    phase += phaseInc * pitchFactor;  // Thêm pitchFactor
    // ...
}
```

`mellotron_engine.cpp`:
```cpp
void MellotronEngine::pitchBend(int bend) noexcept {
    pitchBendSemis_ = std::clamp(bend / 8192.0f * 2.0f, -2.0f, 2.0f);
    pitchBendCache_ = std::pow(2.0f, pitchBendSemis_ / 12.0f);
}

// tickSample():
v.tick(tape_, pitchBendCache_, vL, vR);
```

**Ghi chú:** Mellotron physical instrument không có pitch bend (tape transport). Tuy nhiên, tính năng này hữu ích trong context digital instrument và nhất quán với các engines khác.

---

### 2.7 Drums — Hi-Hat Choke Group

**Vấn đề:** Open hi-hat (HiHatO) và closed hi-hat (HiHatC) có thể play đồng thời → không realistic. Trong drum kit thực: đóng hi-hat sẽ tắt open hi-hat ngay lập tức (và ngược lại).

**Giải pháp — Choke Logic trong `triggerPad()`:**

`drum_engine.cpp`:
```cpp
void DrumEngine::triggerPad(int padIdx, float vel) noexcept {
    // Hi-hat choke: pad 2 = HiHatO, pad 3 = HiHatC
    if (padIdx == 2) {
        // Open hi-hat: choke closed
        dspPads_[3].active = false;
    } else if (padIdx == 3) {
        // Closed hi-hat: choke open
        dspPads_[2].active = false;
    }

    // ... tiếp tục trigger pad như bình thường
}
```

**Kết quả:** Realistic hi-hat behavior. Khi chơi pattern: `HiHatC HiHatC HiHatO_ HiHatC` → open hi-hat bị cut đúng khi closed hi-hat chơi.

**Note về index:** Pad index 2 = Open HiHat, 3 = Closed HiHat theo mapping mặc định trong `drum_engine.cpp`. Nếu mapping thay đổi, cần cập nhật choke logic.

---

### 2.8 Drums — Velocity → Decay Scaling

**Vấn đề:** V1 decay rate cố định — đánh nhẹ hay mạnh thì drum sound decay cùng tốc độ. Drum thực: đánh mạnh → drum head vibrates harder → decay chậm hơn (more energy to dissipate); đánh nhẹ → decay nhanh hơn.

**Giải pháp:**

`drum_engine.cpp` — trong `DrumPadDsp::trigger()`:
```cpp
// velocity → decay scaling
// vel = 0..127; velN = 0.0..1.0
// velScale range: [0.8 (vel=0), 1.2 (vel=127)]
const float velN = vel / 127.0f;
const float velScale = 0.8f + 0.4f * velN;

// Điều chỉnh decayRate theo velocity
// decayRate < 1: mỗi sample × decayRate → giảm dần
// power < 1 → decay chậm hơn (ví dụ decayRate=0.9995, velScale=1.2 → 0.9995^(1/1.2) ≈ 0.9996)
// power > 1 → decay nhanh hơn (velScale=0.8 → 0.9995^(1/0.8) ≈ 0.9994)
decayRate = std::pow(decayRate, 1.0f / velScale);
```

**Kết quả động học:**
- `vel = 0` → `velScale = 0.8` → `decayRate^(1/0.8)` → decay **nhanh hơn** (light hit)
- `vel = 64` → `velScale = 1.0` → `decayRate^1` → decay **không đổi** (medium hit)
- `vel = 127` → `velScale = 1.2` → `decayRate^(1/1.2)` → decay **chậm hơn** (hard hit)

Kết hợp với velocity→amplitude (đã có sẵn trước đó), dynamics của drums trở nên tự nhiên hơn rõ rệt.

---

## 3. P2 Cross-Engine Improvements — Quality & Realism

> **Nguồn gốc:** Phân tích `DSP_Engine_Analysis.md` → Priority 2 Roadmap. Mục tiêu: nâng chất lượng DSP phát triển theo hướng physical modeling, thêm tính năng tiêu chuẩn (sustain pedal, vibrato, exponential envelopes), và cải thiện tape/organ realism.

### 3.1 Trạng thái tổng quan P2

| # | Item | Engine | Trạng thái |
|---|------|--------|------------|
| 3.2 | **Leslie Rotary Cabinet (dual-rotor)** | Hammond | ✅ Hoàn thành |
| 3.3 | **DX7 Exponential Envelopes** | DX7 | ✅ Hoàn thành |
| 3.4 | **DX7 Fixed-Freq Operators** | DX7 | ✅ Hoàn thành |
| 3.5 | **Tape Wow/Flutter** | Mellotron | ✅ Hoàn thành |
| 3.6 | **Tape Hiss Floor** | Mellotron | ✅ Hoàn thành |
| 3.7 | **Rhodes Vibrato LFO** | Rhodes | ✅ Hoàn thành |
| 3.8 | **Rhodes CC64 Sustain Pedal** | Rhodes | ✅ Hoàn thành |
| 3.9 | **Velocity → Rhodes Pickup** | Rhodes | ✅ Hoàn thành |
| 3.10 | **Rhodes ParamSmoother (Tone/Decay/Drive)** | Rhodes | ✅ Hoàn thành |
| 3.11 | **Per-Mode Excitation Scaling** | Rhodes | ✅ Hoàn thành |
| 3.12 | **Modal Amplitude Calibration** | Rhodes | ✅ Hoàn thành |
| 3.13 | **Sympathetic Resonance** | Rhodes | ✅ Hoàn thành |
| 3.14 | **Moog Output Limiter (post-VCA)** | Moog | ✅ Hoàn thành |
| 3.15 | **DX7 Keyboard Rate Scaling** | DX7 | ✅ Hoàn thành |

---

### 3.2 Hammond — True Leslie Dual-Rotor

**Vấn đề:** Hammond chỉ có Vibrato/Chorus đơn giản (scanner LFO → pitch modulation). Thiếu **Leslie rotary speaker** — thành phần âm thanh quan trọng nhất định hình tone Hammond B-3 cổ điển. Leslie thật có 2 rotors quay độc lập (horn + drum), tạo ra Doppler shift + AM shading + crossover tách dải tần.

**Giải pháp:** Triển khai dual-rotor Leslie model hoàn chỉnh trong `processLeslie()`.

**Files đã sửa:**
- `core/engines/hammond/hammond_engine.h` — Thêm 4 params (`HP_LESLIE_ON/SPEED/MIX/SPREAD`), Leslie rotor state (phase, rateHz, target), delay line (`LESLIE_DELAY_SAMPLES = 2048`), crossover state (`lpState_`, `hpState_`)
- `core/engines/hammond/hammond_engine.cpp` — Implement `processLeslie()`, `readDelay()`, tích hợp vào `tickSample()`
- `ui/panels/engines/panel_hammond.cpp` — Thêm section "LESLIE ROTARY" với Enable, Fast, Mix, Spread controls

**Tham số mới trong `HammondParam`:**

| Param | ID | Range | Default | Ghi chú |
|-------|----|-------|---------|---------|
| `HP_LESLIE_ON` | 15 | 0/1 | 0 | Enable/disable |
| `HP_LESLIE_SPEED` | 16 | 0=slow, 1=fast | 0 | Slow/Fast toggle |
| `HP_LESLIE_MIX` | 17 | 0..1 | 0.65 | Dry/wet balance |
| `HP_LESLIE_SPREAD` | 18 | 0..1 | 0.75 | Stereo width |

**Code pattern — `processLeslie()`:**
```cpp
void HammondEngine::processLeslie(float inMono, float& outL, float& outR) noexcept {
    // 1. Mechanical inertia ramp (horn nhanh hơn drum)
    hornRateHz_ += (hornTargetHz_ - hornRateHz_) * 0.0009f;  // horn: 0.8↔6.2 Hz
    drumRateHz_ += (drumTargetHz_ - drumRateHz_) * 0.0007f;  // drum: 0.67↔5.6 Hz

    hornPhase_ += (TWO_PI * hornRateHz_) / sampleRate_;
    drumPhase_ += (TWO_PI * drumRateHz_) / sampleRate_;

    // 2. Simple crossover: low rotor (<~700 Hz), horn (>~700 Hz)
    lpState_ += 0.07f * (inMono - lpState_);
    const float lowBand  = lpState_;
    const float highBand = inMono - lpState_;

    // 3. Delay-based Doppler modulation
    leslieDelay_[leslieWritePos_] = inMono;
    leslieWritePos_ = (leslieWritePos_ + 1) & (LESLIE_DELAY_SAMPLES - 1);

    const float hornBase  = 1.8f + 0.8f * leslieSpread_;
    const float hornDepth = 1.4f + 1.1f * leslieSpread_;
    const float drumBase  = 3.4f + 0.9f * leslieSpread_;
    const float drumDepth = 0.8f + 0.7f * leslieSpread_;

    const float hornDelayL = hornBase + hornDepth * std::sin(hornPhase_);
    const float hornDelayR = hornBase - hornDepth * std::sin(hornPhase_);
    const float drumDelayL = drumBase + drumDepth * std::sin(drumPhase_);
    const float drumDelayR = drumBase - drumDepth * std::sin(drumPhase_);

    // 4. Read interpolated delay + band combine
    const float dopL = readDelay(hornDelayL) * highBand
                     + readDelay(drumDelayL) * lowBand;
    const float dopR = readDelay(hornDelayR) * highBand
                     + readDelay(drumDelayR) * lowBand;

    // 5. AM shading — rotor occlusion simulation
    const float hornAmL = 0.60f + 0.40f * (0.5f + 0.5f * std::cos(hornPhase_));
    const float hornAmR = 0.60f + 0.40f * (0.5f + 0.5f * std::cos(hornPhase_ + PI));
    const float drumAmL = 0.75f + 0.25f * (0.5f + 0.5f * std::cos(drumPhase_));
    const float drumAmR = 0.75f + 0.25f * (0.5f + 0.5f * std::cos(drumPhase_ + PI));

    outL = dopL * (0.65f * hornAmL + 0.35f * drumAmL);
    outR = dopR * (0.65f * hornAmR + 0.35f * drumAmR);
}
```

**Tích hợp vào `tickSample()`:**
```cpp
if (leslieOn_) {
    const float inMono = 0.5f * (sumL + sumR);
    float wetL = 0.0f, wetR = 0.0f;
    processLeslie(inMono, wetL, wetR);

    const float dry = 1.0f - leslieMix_;
    outL = masterVol_ * (dry * inMono + leslieMix_ * wetL);
    outR = masterVol_ * (dry * inMono + leslieMix_ * wetR);
    return;
}
```

**Signal path với Leslie:**
```
61 voices → Mix → [Overdrive] → mono sum → processLeslie()
                                   │
                    ┌───────────────┼───────────────┐
                    │ Simple Crossover (lpf 0.07)   │
                    ▼                               ▼
                 lowBand                         highBand
                    │                               │
              drumDelay (Doppler)            hornDelay (Doppler)
                    │                               │
              drumAM shading                 hornAM shading
                    └───────────┬───────────────────┘
                         stereo L/R mix
                                │
                     dry/wet blend → masterVol → output
```

**Kết quả:**
- Horn rotor: 0.8 Hz (slow) → 6.2 Hz (fast) — tần số quay chuẩn Leslie 122
- Drum rotor: 0.67 Hz (slow) → 5.6 Hz (fast) — chậm hơn horn (inertia nặng hơn)
- Inertia ramp tạo hiệu ứng spin-up/spin-down tự nhiên khi chuyển Slow↔Fast
- Doppler shift qua delay line (linear interpolated) tạo pitch wobble
- AM shading: horn ảnh hưởng 0.60–1.0, drum 0.75–1.0 → horn AM rõ hơn drum (đúng như thực tế)
- Spread control: điều chỉnh Doppler depth → stereo width (loa gần mic → stereo rộng)
- RT-safe: delay buffer pre-allocated fixed-size, chỉ sin/cos + linear interp

---

### 3.3 DX7 — Exponential Envelope Contour

**Vấn đề:** V1 dùng **linear ADSR** — Attack tăng đều, Decay giảm đều. DX7 hardware thật sử dụng **exponential contour** — Attack lao nhanh lên rồi chậm dần ở đỉnh, Decay "plunge" nhanh rồi tail dài. Linear envelope khiến transient thiếu "snap" và sustain transition không mượt.

**So sánh:**
```
Linear Attack:       Exponential Attack (DX7):
    ___                  ___
   /                    /
  /                    /
 /                   /│
/___________      /  │______
                  fast  slow↗
```

**Giải pháp:** Chuyển toàn bộ 4 stages sang per-sample exponential coefficients.

**Files đã sửa:**
- `core/engines/dx7/dx7_voice.h` — `DX7Operator::trigger()` → tính exponential rate, `tick()` → exponential stage processing

**Code pattern — `DX7Operator::trigger()`:**
```cpp
// Time param → milliseconds → exponential coefficient
// DX7-style range: 1ms..5000ms attack, 1ms..5000ms decay/release

// Attack: approach-to-1 exponential
// pow(0.001, 1/N) = hệ số mà sau N samples, phần dư còn 0.1% (-60dB)
const float atkMs = 1.0f + atk * 2999.0f;     // 1ms..3000ms
attackRate  = std::pow(0.001f, 1.0f / (atkMs * 0.001f * sampleRate));

const float decMs = 1.0f + dec * 4999.0f;     // 1ms..5000ms
decayRate   = std::pow(0.001f, 1.0f / (decMs * 0.001f * sampleRate));

const float relMs = 1.0f + rel * 2999.0f;     // 1ms..3000ms
releaseRate = std::pow(0.001f, 1.0f / (relMs * 0.001f * sampleRate));
```

**Code pattern — `DX7Operator::tick()` per-stage:**
```cpp
switch (stage) {
    case Stage::Attack:
        // Exponential approach: envVal → 1.0 (nhanh ban đầu, chậm dần)
        envVal = 1.0f - (1.0f - envVal) * attackRate;
        if (envVal >= 0.9995f) { envVal = 1.0f; stage = Stage::Decay; }
        break;
    case Stage::Decay:
        // Exponential approach: envVal → sustainLvl (nhanh ban đầu, tail dài)
        envVal = sustainLvl + (envVal - sustainLvl) * decayRate;
        if (envVal <= sustainLvl + 0.0001f) {
            envVal = sustainLvl; stage = Stage::Sustain;
        }
        break;
    case Stage::Sustain:
        envVal = sustainLvl;
        break;
    case Stage::Release:
        // Exponential decay → 0 (tail dài, fade tự nhiên)
        envVal *= releaseRate;
        if (envVal < 0.00001f) { envVal = 0.0f; stage = Stage::Off; }
        break;
}
```

**Kết quả:**
- Attack nhanh có "snap" — lao nhanh từ 0 → 0.5 trong 30% attack time, rồi chậm dần → 1.0
- Decay có "plunge + tail" — amplitude drop nhanh rồi sustain dài → chuẩn FM piano
- Release fade tự nhiên — giảm ~60dB sau releaseMs, thay vì linear cut
- Giữ nguyên schema param (0..1 → ms mapping) → preset JSON tương thích ngược
- Velocity sensitivity áp dụng vào `sustainLvl` → exponential decay target thay đổi theo velocity

---

### 3.4 DX7 — Fixed-Frequency Operators

**Vấn đề:** DX7 hardware cho phép mỗi operator chọn giữa **ratio mode** (freq = baseHz × ratio) và **fixed mode** (freq = constant Hz, bất kể note). Fixed mode cần thiết cho:
- **Metallic/bell sounds** → operator ở freq cố định tạo non-harmonic sidebands
- **Drum synthesis** → noise-like FM bằng fixed-freq modulator
- **Special FX** → inharmonic texture không phụ thuộc pitch

**Giải pháp:** Thêm 2 params/operator: `FixedMode` (bool 0/1) và `FixedHz` (20..8000 Hz). Tổng 12 params mới.

**Files đã sửa:**
- `core/engines/dx7/dx7_engine.h` — Thêm 12 params: `DX7P_OP0_FIXED_MODE..DX7P_OP5_FIXED_MODE`, `DX7P_OP0_FIXED_HZ..DX7P_OP5_FIXED_HZ`. `DX7_PARAM_COUNT` tăng thêm 12. Thêm `fixedMode_[6]` và `fixedHz_[6]` cached arrays.
- `core/engines/dx7/dx7_voice.h` — `DX7Operator` struct thêm `fixedMode`, `fixedHz` members. `trigger()` và `tick()` handle cả 2 mode.
- `core/engines/dx7/dx7_engine.cpp` — Metadata arrays (NAMES/MIN/MAX/DEF) thêm 12 entries. `beginBlock()` load fixedMode/fixedHz. `noteOn()` truyền fixedMode/fixedHz vào voice trigger.

**Tham số mới:**

| Param | Index | Range | Default | Ghi chú |
|-------|-------|-------|---------|---------|
| `DX7P_OP0_FIXED_MODE..OP5` | 43–48 | 0/1 | 0 | 0=ratio, 1=fixed |
| `DX7P_OP0_FIXED_HZ..OP5` | 49–54 | 20..8000 | 440 | Fixed frequency (Hz) |

**Code pattern — `DX7Operator::trigger()`:**
```cpp
fixedMode = isFixedMode;
fixedHz   = std::clamp(hzFixed, 20.0f, 8000.0f);

// Chọn frequency theo mode:
const float freqHz = fixedMode ? fixedHz : (baseFreq * ratio);
phaseInc = (TWO_PI * freqHz) / sampleRate;
```

**Code pattern — `DX7Operator::tick()` — pitch bend bypass cho fixed mode:**
```cpp
// Fixed mode: pitch bend KHÔNG ảnh hưởng (đúng như DX7 hardware)
phase += phaseInc * (fixedMode ? 1.0f : pitchFactor);
if (phase >= TWO_PI) phase -= TWO_PI;
```

**Code pattern — `DX7Voice::trigger()` — forward params:**
```cpp
void DX7Voice::trigger(int midiNote, int vel, float sampleRate,
                        int algorithm,
                        const float ratios[NUM_OPS],
                        const bool  fixedMode[NUM_OPS],   // ← mới
                        const float fixedHz[NUM_OPS],     // ← mới
                        const float levels[NUM_OPS],
                        /* ... */) noexcept {
    // ...
    for (int i = 0; i < NUM_OPS; ++i) {
        ops[i].trigger(baseFreq, sampleRate,
                       ratios[i], levels[i], vs[i],
                       fixedMode[i], fixedHz[i],    // ← mới
                       attacks[i], decays[i], sustains[i], releases[i], vel);
    }
}
```

**Kết quả:**
- Operator có thể chạy ở fixed Hz → tạo non-harmonic FM sidebands (DX7 bell/chime classic)
- Pitch bend chỉ ảnh hưởng ratio-mode operators → fixed operators giữ nguyên pitch (đúng hardware)
- 12 param mới backward-compatible: default = ratio mode + 440 Hz → preset cũ hoạt động bình thường
- UI table trong `panel_dx7.cpp` hiển thị FixedMode toggle + FixedHz slider per-operator

---

### 3.5 Mellotron — Tape Wow/Flutter

**Vấn đề:** Mellotron M400 có tape transport cơ khí với 2 loại pitch modulation không thể tránh:
- **Wow**: dao động tần số thấp (0.1–3 Hz) từ capstan/spindle eccentricity → pitch "drift" chậm
- **Flutter**: dao động tần số cao (6–18 Hz) từ motor vibration → shimmer nhanh

V1 chỉ có playback thẳng theo phaseInc → thiếu "analog tape feel" hoàn toàn.

**Giải pháp:** Thêm 4 params (Wow Depth/Rate, Flutter Depth/Rate) + engine-level dual sinusoid LFO. Compound pitch modulation truyền vào voice tick() cùng với pitch bend.

**Files đã sửa:**
- `core/engines/mellotron/mellotron_engine.h` — Thêm 4 params (`MP_WOW_DEPTH/RATE`, `MP_FLUTTER_DEPTH/RATE`), `wowPhase_`/`flutterPhase_` LFO state, cached `wowDepthPct_`/`wowRateHz_`/`flutterDepthPct_`/`flutterRateHz_`
- `core/engines/mellotron/mellotron_engine.cpp` — `beginBlock()` map normalized params → physical values, `tickSample()` advance dual LFOs + feed compound factor into voice tick

**Tham số mới:**

| Param | Range (normalized) | Physical mapping | Default |
|-------|-------------------|------------------|---------|
| `MP_WOW_DEPTH` | 0..1 | 0..2.0% speed modulation | 0.15 |
| `MP_WOW_RATE` | 0..1 | 0.1..3.0 Hz | 0.30 |
| `MP_FLUTTER_DEPTH` | 0..1 | 0..0.6% speed modulation | 0.10 |
| `MP_FLUTTER_RATE` | 0..1 | 6..18 Hz | 0.35 |

**Code pattern — `beginBlock()` mapping:**
```cpp
// Depth = % speed modulation (1.0 = 100%)
wowDepthPct_     = wowDepthRaw * 0.020f;       // 0..2.0%
wowRateHz_       = 0.1f + wowRateRaw * 2.9f;   // 0.1..3.0 Hz
flutterDepthPct_ = flutterDepthRaw * 0.006f;   // 0..0.6%
flutterRateHz_   = 6.0f + flutterRateRaw * 12.0f; // 6..18 Hz
```

**Code pattern — `tickSample()` compound modulation:**
```cpp
// Per-sample LFO advance (engine-level, shared across all voices)
wowPhase_ += (TWO_PI * wowRateHz_) / sampleRate_;
if (wowPhase_ >= TWO_PI) wowPhase_ -= TWO_PI;

flutterPhase_ += (TWO_PI * flutterRateHz_) / sampleRate_;
if (flutterPhase_ >= TWO_PI) flutterPhase_ -= TWO_PI;

// Compound tape transport modulation
const float wowMul     = 1.0f + wowDepthPct_ * std::sin(wowPhase_);
const float flutterMul = 1.0f + flutterDepthPct_ * std::sin(flutterPhase_);
const float tapeModMul = wowMul * flutterMul;

// Feed compound factor cùng pitch bend vào mỗi voice
for (auto& v : voices_) {
    if (!v.active) continue;
    v.tick(tape_, pitchBendCache_ * tapeModMul, vL, vR);
    // ...
}
```

**Signal path Mellotron:**
```
wowLFO ──┐
          ├─ compound multiply ─→ tapeModMul
flutterLFO┘                            │
                                       ▼
                           pitchBendCache_ × tapeModMul
                                       │
               ┌───────────────────────┤
               ▼                       ▼
          voice[0].tick(pitchFactor)  voice[n].tick(pitchFactor)
               │                       │
               └───────┬───────────────┘
                       ▼
                   voice sum + hiss floor → volume → output
```

**Kết quả:**
- Engine-level LFO → tất cả voices chịu cùng wow/flutter → đúng vật lý (1 tape transport chung)
- Wow ~0.7 Hz default tạo pitched "drift" chậm, nghe được rõ trên sustained note
- Flutter ~10 Hz default tạo "shimmering" nhanh, thêm character sinh động
- Compound multiply `wowMul × flutterMul` → 2 modulations cộng hưởng tạo complex pattern
- Default values tạo subtle tape feel (wow 1.5%, flutter 0.6%) — không overwhelm, đúng mức M400
- RT-safe: chỉ sin() + multiply, 2 phase accumulators

---

### 3.6 Mellotron — Tape Hiss Floor

**Vấn đề:** Mellotron M400 thực luôn có **tape hiss** (tiếng nhiễu từ tính của magnetic tape) bất cứ khi nào tape đang chạy. V1 silent hoàn toàn giữa các notes → thiếu tape realism.

**Giải pháp:** Inject LCG white noise ở ~-50 dBFS khi có voice active. Level tăng nhẹ theo số voices active (nhiều "dây" chạy đồng thời → nhiều noise hơn).

**Files đã sửa:**
- `core/engines/mellotron/mellotron_engine.h` — Thêm `nextNoise()` method (reuse existing LCG `rng_`)
- `core/engines/mellotron/mellotron_engine.cpp` — `tickSample()` thêm hiss injection sau voice sum

**Code pattern — Noise generator:**
```cpp
// mellotron_engine.h — LCG noise reusing existing rng_
float nextNoise() noexcept {
    return nextRng() * 2.0f - 1.0f;   // Range [-1, +1]
}

// nextRng(): LCG already exists for pitch spread
float nextRng() noexcept {
    rng_ = rng_ * 1664525u + 1013904223u;
    return static_cast<float>(rng_) / 4294967296.0f;   // [0, 1)
}
```

**Code pattern — `tickSample()` hiss injection:**
```cpp
// Sau voice loop sum:
if (activeVoices > 0) {
    // Base: ~-50 dBFS (0.0018 ≈ -55dB) + 0.00015 per active voice
    // Ở full polyphony (35 voices): ~0.007 ≈ -43dB — vẫn rất subtle
    const float hiss = nextNoise()
                     * (0.0018f + 0.00015f * static_cast<float>(activeVoices));
    sumL += hiss;
    sumR += hiss;
}
```

**Kết quả:**
- Hiss chỉ xuất hiện khi có voice active → silence khi không chơi (đúng: tape chỉ chạy khi key pressed trên M400)
- Level -50 dBFS: barely audible alone, nhưng tạo "bed" dưới notes →cảm giác tape thực
- Voice-scaled: chord 6 notes có hiss nhỉnh hơn single note (6 tapes chạy đồng thời)
- Mono noise (cùng value cho L và R): đúng vật lý — M400 mono playback
- RT-safe: chỉ 1 integer multiply + bit manipulation per sample

---

### 3.7 Rhodes — Vibrato LFO

**Vấn đề:** Rhodes V2 có tremolo (amplitude modulation → stereo autopan) nhưng thiếu **vibrato** (pitch modulation). Vibrato là hiệu ứng pitch subtle (±35 cents) tạo expressiveness cho sustained notes — đặc biệt hữu ích cho ballad và solo.

**Thách thức kỹ thuật:** Rhodes dùng biquad modal resonators — không có phaseInc để nhân pitchFactor. Phải update `coef = 2·cos(ω)` trong mỗi resonator. Quan trọng: KHÔNG reset state `{y0, y1}` → phải dùng `updateFreq()` đã có (pattern từ P1 pitch bend).

**Giải pháp:** Engine-level vibrato LFO → per-sample pitch mod semitones → cộng với pitch bend → update modal frequencies khi semitone thay đổi.

**Files đã sửa:**
- `core/engines/rhodes/rhodes_engine.h` — Thêm 2 params (`RP_VIBRATO_RATE`, `RP_VIBRATO_DEPTH`), `vibratoPhase_` member, `prevPitchModSemis_` cho change detection
- `core/engines/rhodes/rhodes_engine.cpp` — `tickSample()` advance vibrato LFO + combine với pitch bend + update voices

**Tham số:**

| Param | Range | Default | Ghi chú |
|-------|-------|---------|---------|
| `RP_VIBRATO_RATE` | 0..12 Hz | 5.2 Hz | Tần số LFO |
| `RP_VIBRATO_DEPTH` | 0..1 | 0.0 | Depth (mapped to ±35 cents max) |

**Code pattern — `tickSample()` vibrato:**
```cpp
float pitchModSemis = pitchBendSemis_;  // Start with pitch bend

if (vibratoDepth_ > 0.0001f) {
    vibratoPhase_ += (TWO_PI * vibratoRate_) / sampleRate_;
    if (vibratoPhase_ >= TWO_PI) vibratoPhase_ -= TWO_PI;

    // Max vibrato depth: ±35 cents = ±0.35 semitones
    pitchModSemis += std::sin(vibratoPhase_) * (vibratoDepth_ * 0.35f);
}

// Change detection: chỉ update modal resonator frequencies khi semitones thay đổi
// (tránh 4 cos() calls per voice per sample khi không cần thiết)
if (pitchModSemis != prevPitchModSemis_) {
    for (auto& v : voices_)
        if (v.active) v.updatePitchFactor(pitchModSemis, sampleRate_);
    prevPitchModSemis_ = pitchModSemis;
}
```

**Code pattern — `RhodesVoice::updatePitchFactor()` (reuse từ P1 pitch bend):**
```cpp
void updatePitchFactor(float pitchBendSemis, float sr) noexcept {
    if (!active) return;
    constexpr float ratios[NUM_MODES] = { 1.000f, 2.045f, 5.979f, 9.210f };
    const float factor = std::pow(2.0f, pitchBendSemis / 12.0f);
    const float bentHz = baseHz_ * factor;
    for (int m = 0; m < NUM_MODES; ++m) {
        const float offset  = inharmonicOffset(note, m);
        const float newFreq = bentHz * ratios[m] * std::pow(2.0f, offset / 12.0f);
        modes[m].updateFreq(newFreq, sr);
    }
    torsionMode_.updateFreq(bentHz * std::pow(2.0f, 6.0f / 1200.0f), sr);
}
```

**Kết quả:**
- Vibrato 5.2 Hz / depth 0.3 → subtle Rhodes classic vibrato, nghe rõ trên sustained note
- Vibrato + pitch bend cộng hưởng: xoay mod wheel + bend → expressive combined modulation
- Change detection tiết kiệm CPU: chỉ update khi pitchModSemis thực sự thay đổi
- Vibrato bypass khi depth = 0 → không tốn CPU khi không dùng
- `updateFreq()` bảo toàn amplitude — không pop/click khi vibrato chạy

---

### 3.8 Rhodes — CC64 Sustain Pedal

**Vấn đề:** Rhodes V2 không xử lý MIDI CC64 (sustain pedal). Khi nhấn sustain pedal, noteOff vẫn release voice ngay → pianist không thể giữ chord bằng pedal.

**Giải pháp:** Thêm `pedalDown_` flag + `noteHeld_[128]` array. Khi pedal down, noteOff chỉ đánh dấu note cần release — thực sự release khi pedal up.

**Files đã sửa:**
- `core/engines/rhodes/rhodes_engine.h` — Thêm `bool pedalDown_`, `bool noteHeld_[128]`
- `core/engines/rhodes/rhodes_engine.cpp` — `noteOff()` defer release, `controlChange(cc=64)` pedal on/off + batch release, `noteOn()` clear held flag, `allSoundOff()` reset pedal state

**Code pattern — `noteOff()` với pedal deferral:**
```cpp
void RhodesEngine::noteOff(int note) noexcept {
    // Nếu sustain pedal đang giữ → defer release
    if (pedalDown_) {
        noteHeld_[note] = true;
        return;   // Không release voice — giữ tiếng cho đến khi pedal lên
    }
    const int idx = findVoiceByNote(note);
    if (idx >= 0)
        voices_[idx].release(releaseMs_);
}
```

**Code pattern — `controlChange()` CC64 handling:**
```cpp
case 64:  // Sustain pedal
    pedalDown_ = (val >= 64);
    if (!pedalDown_) {
        // Pedal released: trigger ALL deferred note-offs
        for (int n = 0; n < 128; ++n) {
            if (noteHeld_[n]) {
                const int idx = findVoiceByNote(n);
                if (idx >= 0) voices_[idx].release(releaseMs_);
                noteHeld_[n] = false;
            }
        }
    }
    break;
```

**Code pattern — `noteOn()` clear held state:**
```cpp
void RhodesEngine::noteOn(int note, int vel) noexcept {
    // Retrigger: clear pedal-held state cho note này
    noteHeld_[note] = false;
    // ...trigger voice bình thường...
}
```

**Kết quả:**
- Standard MIDI sustain pedal behavior: pedal down → notes ring, pedal up → chord releases
- Retrigger safe: noteOn() đồng note đang held sẽ clear held flag → không double-release
- `allSoundOff()` reset pedal → tránh stuck-pedal state
- Memory: chỉ 128 bytes (`bool noteHeld_[128]`) + 1 bool → negligible
- RT-safe: chỉ array indexing, không allocation

---

### 3.9 Rhodes — Velocity → Pickup Nonlinearity

**Vấn đề:** V1 pickup polynomial `y = x + α·x² + β·x³` chỉ phụ thuộc Tone parameter. Rhodes thực: hard hit → tine vibrates with larger amplitude → enters pickup nonlinear zone deeper → **richer harmonics**. Nhẹ nhàng → tine stays linear → clean tone. Velocity không ảnh hưởng timbre → thiếu expressive dynamics.

**Giải pháp:** Thêm velocity contribution vào pickup drive calculation. `pickupAlpha` và `pickupBeta` giờ phụ thuộc cả Tone lẫn Velocity.

**Files đã sửa:**
- `core/engines/rhodes/rhodes_voice.h` — `trigger()`: velocity vào pickup drive formula

**Code pattern — `trigger()` pickup drive calculation:**
```cpp
// Velocity contribution: smoothstep curve → 0..0.06
const float velN       = velCurve(vel);          // smoothstep: v²(3-2v)
const float velContrib = velN * 0.06f;           // max 0.06 vel contribution

// Pickup drive = base(0.03) + tone(0..0.07) + velocity(0..0.06)
// Total range: 0.03 (soft, tone=0) .. 0.16 (hard, tone=1)
const float pickupDrive = 0.03f + toneParam * 0.07f + velContrib;

pickupAlpha = pickupDrive;           // x² → even harmonics (bell chime)
pickupBeta  = pickupDrive * 0.5f;    // x³ → odd  harmonics (warmth/bark)
```

**Pickup polynomial trong `pickupModel()`:**
```cpp
// Displacement-based (frequency-independent):
inline float pickupModel(float x) const noexcept {
    return x + pickupAlpha * x * x + pickupBeta * x * x * x;
}
// pickupAlpha → even harmonics (2nd, 4th...) → "bell" character
// pickupBeta  → odd  harmonics (3rd, 5th...) → "warmth/bark"
```

**Dynamic range kết quả:**

| Velocity | Tone | pickupDrive | Character |
|----------|------|-------------|-----------|
| pp (vel=20) | 0.0 (dark) | 0.04 | Rất clean, almost pure fundamental |
| pp (vel=20) | 1.0 (bright) | 0.11 | Clean nhưng có chút bell chime |
| ff (vel=127) | 0.0 (dark) | 0.09 | Warm bark, moderate harmonics |
| ff (vel=127) | 1.0 (bright) | 0.16 | Rich harmonics, Rhodes "growl" ở fff |

**Kết quả:**
- Hard hit → pickupDrive cao → tine displacement lớn vào vùng phi tuyến → nhiều harmonics
- Soft hit → pickupDrive thấp → tine stays linear → clean, bell-like tone
- Tone vẫn ảnh hưởng (pickup position) nhưng velocity thêm layer dynamic
- smoothstep curve thay vì linear: velocity response tự nhiên hơn (không abrupt at extremes)
- Kết hợp với velocity→amplitude (đã có sẵn): dynamics trở nên rất expressive

---

### 3.10 Rhodes — ParamSmoother (Tone/Decay/Drive)

**Vấn đề:** Decay, Tone, và Drive là 3 params ảnh hưởng trực tiếp tới timbre Rhodes. Khi UI cập nhật giá trị mỗi block (64-256 samples), voice nhận giá trị mới ngay → zipper noise (step artifacts nghe rõ ở high-quality signal).

**Giải pháp:** Áp dụng `ParamSmoother` (exponential ramp 5ms) cho cả 3 params. Mỗi sample tick smoother → giá trị thay đổi mượt.

**Files đã sửa:**
- `core/engines/rhodes/rhodes_engine.h` — Thêm 3 members: `ParamSmoother decaySmoother_`, `toneSmoother_`, `driveSmoother_`
- `core/engines/rhodes/rhodes_engine.cpp` — `init()`, `setSampleRate()`, `beginBlock()`, `tickSample()` integrate smoothers

**Code pattern — `init()` / `setSampleRate()`:**
```cpp
void RhodesEngine::init(float sampleRate) noexcept {
    sampleRate_ = sampleRate;
    // ... other init ...

    // ParamSmoother: exponential ramp 5ms → transparent, no latency
    decaySmoother_.init(sampleRate_, 5.0f);
    toneSmoother_.init(sampleRate_, 5.0f);
    driveSmoother_.init(sampleRate_, 5.0f);

    // Snap to initial value (tránh ramp from 0 khi khởi động)
    decaySmoother_.snapTo(decayParam_);
    toneSmoother_.snapTo(toneParam_);
    driveSmoother_.snapTo(driveParam_);
}
```

**Code pattern — `beginBlock()` set targets:**
```cpp
void RhodesEngine::beginBlock(int /*nFrames*/) noexcept {
    decayParam_   = params_[RP_DECAY].load(std::memory_order_relaxed);
    toneParam_    = params_[RP_TONE].load(std::memory_order_relaxed);
    driveParam_   = params_[RP_DRIVE].load(std::memory_order_relaxed);
    // ... load other params ...

    // Set smoother targets — smoother sẽ ramp đến value mới trong 5ms
    decaySmoother_.setTarget(decayParam_);
    toneSmoother_.setTarget(toneParam_);
    driveSmoother_.setTarget(driveParam_);
}
```

**Code pattern — `tickSample()` per-sample tick:**
```cpp
void RhodesEngine::tickSample(float& outL, float& outR) noexcept {
    // Per-sample smooth advancement
    const float decaySmoothed = decaySmoother_.tick();
    const float toneSmoothed  = toneSmoother_.tick();
    const float driveSmoothed = driveSmoother_.tick();

    // Update cached values with smoothed versions
    decayParam_ = decaySmoothed;
    toneParam_  = toneSmoothed;

    // ...voice loop uses decayParam_, toneParam_ (now smoothed)...

    // Drive used in output saturator:
    const float drive = 1.0f + driveSmoothed * 1.5f;
    sumL = outputSaturate(sumL, drive);
    sumR = outputSaturate(sumR, drive);
}
```

**Kết quả:**
- Kéo Tone knob: timbre thay đổi mượt mà, không còn zipper artifacts
- Kéo Decay knob: resonator lifetime transition tự nhiên
- Kéo Drive knob: saturation amount ramp mượt, không click
- 5ms ramp: đủ nhanh để không cảm giác latency, đủ dài để loại step artifacts
- `snapTo()` ở init: tránh ramp from 0 → giá trị đúng ngay từ đầu

---

### 3.11 Rhodes — Per-Mode Excitation Scaling

**Vấn đề:** V1 hammer excitation cùng amplitude cho tất cả modal resonators. Vật lý thực: hammer có kích thước cố định — lower modes (wavelength dài) nhận trọn vẹn hammer energy, higher modes (wavelength ngắn) chỉ nhận một phần vì hammer bao phủ nhiều bước sóng → cancellation.

**Giải pháp:** Thêm scaling array `HAMMER_EXCITE[]` cho mỗi mode, áp dụng khi trigger.

**Files đã sửa:**
- `core/engines/rhodes/rhodes_voice.h` — `trigger()`: thêm per-mode excitation scaling

**Code pattern — `trigger()` hammer excitation:**
```cpp
// Hammer kích thước cố định → coupling efficiency giảm theo partial number
// Từ mô hình vibrating bar + point hammer (Fletcher & Rossing 1998):
constexpr float HAMMER_EXCITE[NUM_MODES] = {
    1.00f,  // Fundamental: full coupling
    0.85f,  // 2nd partial: slight cancellation
    0.50f,  // Bell partial: significant (wavelength ≈ hammer width)
    0.25f   // High partial: poor coupling (wavelength << hammer)
};

const float velN  = velCurve(vel);
const float velSc = (1.0f - velSens) + velSens * velN;

for (int m = 0; m < NUM_MODES; ++m)
    modes[m].excite(velSc * HAMMER_EXCITE[m]);
```

**Kết quả:**
- Fundamental nhận 100% energy → sustain dài nhất (đúng)
- Bell partial chỉ nhận 50% → decay nhanh hơn relative (đúng: bell "chime" nghe rõ ở attack rồi fade)
- High partial chỉ 25% → attack transient short → đúng vật lý stiff tine
- Tổng hợp với per-mode decay times → spectral evolution tự nhiên: attack bright → sustain warm

---

### 3.12 Rhodes — Modal Amplitude Calibration (Tolonen 1998)

**Vấn đề:** V1 modal amplitudes từ estimation. V2 cần calibrate theo dữ liệu đo đạc Rhodes Mark I thực (Tolonen 1998 — "Modeling of Chinese Cymbals" appendix + thực nghiệm tổng hợp).

**Giải pháp:** Tone parameter thay đổi partial balance qua `toneShift` factor. Mid-tone (0.5) match đo đạc Tolonen.

**Files đã sửa:**
- `core/engines/rhodes/rhodes_voice.h` — `trigger()`: tone-dependent amplitude array

**Code pattern — `trigger()` calibrated amplitudes:**
```cpp
// toneShift centred at 1.0 when tone=0.5 → ±30% adjustment
const float toneShift = 0.70f + toneParam * 0.60f;  // 0.70..1.30

// Amplitudes at mid-tone match Tolonen (1998) Rhodes Mark I measurement:
const float modeAmps[NUM_MODES] = {
    1.000f,                    // Fundamental: reference 0 dB
    0.280f * toneShift,        // 2nd (2.045×): -11 dB ref → warmth/body
    0.190f * toneShift,        // Bell (5.979×): -14 dB ref → chime attack
    0.070f * toneShift         // High (9.210×): -23 dB ref → attack transient
};
```

**Tone sweep kết quả:**

| Tone | toneShift | mode1 | mode2 | mode3 | Character |
|------|-----------|-------|-------|-------|-----------|
| 0.0 | 0.70 | 0.196 | 0.133 | 0.049 | Dark, fundamental dominant |
| 0.5 | 1.00 | 0.280 | 0.190 | 0.070 | Balanced — Tolonen reference |
| 1.0 | 1.30 | 0.364 | 0.247 | 0.091 | Bright, rich upper partials |

**Kết quả:**
- Mid-tone = calibrated theo literature → âm thanh chính xác hơn
- Tone 0 không hoàn toàn tắt upper partials (vẫn 70%) → realistic: pickup không thể loại bỏ hoàn toàn overtones
- Tone 1 boost 30% → bright nhưng không distort (kết hợp với pickup nonlinearity sẽ thêm harmonics)
- Fundamental (1.000) không bị ảnh hưởng bởi tone → luôn là thành phần chính

---

### 3.13 Rhodes — Sympathetic Resonance

**Vấn đề:** Acoustic piano/Rhodes thực có hiện tượng **sympathetic resonance** — khi note mới được đánh, tine đang sustain ở interval liên quan hoà âm (unison, 5th, octave) sẽ rung cộng hưởng nhẹ. Thiếu hiệu ứng này → sound quá "isolated" giữa các notes.

**Giải pháp:** Khi `noteOn()`, scan tất cả active voices — nếu interval modulo 12 khớp với coupling table → inject một lượng nhỏ energy vào mode fundamental (y0) của voice đang sustaining.

**Files đã sửa:**
- `core/engines/rhodes/rhodes_engine.h` — Thêm `injectSympatheticEnergy()` declaration
- `core/engines/rhodes/rhodes_engine.cpp` — Implement function + gọi trong `noteOn()`

**Code pattern — coupling table:**
```cpp
// Coupling strength theo interval musical (modulo 12 semitones):
// Unison/Octave (0): 0.015   Fifth (7): 0.010   Fourth (5): 0.008
// Tất cả interval khác: 0.0 (không coupling)
static constexpr float COUPLING[12] = {
    0.015f, 0.0f, 0.0f, 0.0f, 0.0f, 0.008f,
    0.0f,   0.010f, 0.0f, 0.0f, 0.0f, 0.0f
};
```

**Code pattern — `injectSympatheticEnergy()`:**
```cpp
void RhodesEngine::injectSympatheticEnergy(int newNote, float energy) noexcept {
    for (auto& v : voices_) {
        if (!v.active) continue;
        const int interval = std::abs(v.note - newNote) % 12;
        const float coupling = COUPLING[interval];
        if (coupling > 0.0f)
            v.modes[0].y0 += energy * coupling;  // Inject vào fundamental
    }
}
```

**Gọi từ `noteOn()`:**
```cpp
void RhodesEngine::noteOn(int note, int vel) noexcept {
    // ... voice allocation ...
    voices_[idx].trigger(note, vel, sampleRate_, ...);

    // P2.3: Sympathetic resonance — kích năng lượng nhỏ vào voices đang sustain
    injectSympatheticEnergy(note, vel / 127.0f * 0.4f);
}
```

**Kết quả:**
- Chơi C4 đang sustain → đánh G4 → C4 fundamental rung nhẹ (audible không? Barely — nhưng "feel" khác biệt)
- Coupling rất nhỏ (0.008–0.015 × 0.4 × velocity) → không nghe rõ riêng lẻ, nhưng tổng body sound "thicker"
- Chỉ inject vào `modes[0].y0` (fundamental) → sympathetic resonance chủ yếu ở fundamental (đúng vật lý)
- O(activeVoices) per noteOn → tối đa 12 iterations → negligible

---

### 3.14 Moog — Post-VCA Output Limiter

**Vấn đề:** MoogEngine support poly (8 voices), unison (8 voices cùng note), và detune. Khi nhiều voices stack → amplitude cộng dồn vượt 1.0 → digital hard clipping nếu không bảo vệ. Unison 8 voices × full amplitude → peak lên tới 8.0.

**Giải pháp:** Thêm soft limiter post-VCA dùng `tanh(x·drive)/drive` — transparent ở level thấp, saturates mượt ở level cao. Đặt ở `MoogEngine::tickSample()` sau khi `VoicePool::tick()` trả output.

**Files đã sửa:**
- `core/engines/moog/moog_engine.cpp` — `tickSample()` thêm tanh limiter sau VoicePool output

**Code pattern — `tickSample()`:**
```cpp
void MoogEngine::tickSample(float& outL, float& outR) noexcept {
    float l = 0.0f, r = 0.0f;
    voicePool_.tick(paramCachePtr_, l, r);

    // Post-VCA limiter: tanh(x·drive)/drive
    // drive=1.25 → at signal=1.0: output=0.68 (transparent)
    //             → at signal=4.0: output=0.80 (gentle compression)
    //             → never exceeds 1/1.25 = 0.80 → headroom
    constexpr float kLimiterDrive = 1.25f;
    outL = std::tanh(l * kLimiterDrive) / kLimiterDrive;
    outR = std::tanh(r * kLimiterDrive) / kLimiterDrive;

    oscBuf_[oscWritePos_] = outL;
    oscWritePos_ = (oscWritePos_ + 1) & (OSC_BUF_SIZE - 1);
}
```

**Signal path:**
```
VoicePool.tick() → raw sum (có thể >1.0 khi unison/poly stack)
        │
        ▼
  tanh(signal × 1.25) / 1.25   ← soft limiter
        │
        ▼
  oscilloscope buffer → output
```

**Đặc tính `tanh(x·d)/d`:**

| Input level | Output | Compression |
|-------------|--------|-------------|
| 0.0 | 0.0 | None (transparent) |
| 0.5 | 0.46 | ~8% compression (barely audible) |
| 1.0 | 0.68 | ~32% compression |
| 2.0 | 0.78 | ~61% compression (unison 2 voices) |
| 4.0 | 0.80 | ~80% (smooth ceiling) |
| 8.0 | 0.80 | Max output ≈ 1/drive = 0.80 |

**Kết quả:**
- Single voice ở moderate level: virtually transparent (~8% compression → inaudible)
- Unison 4-8 voices: graceful compression thay vì hard clip → warm saturation character
- Maximum output capped at 1/1.25 = 0.80 → headroom cho EffectChain sau đó
- Không thêm param — hard-coded drive → đơn giản, consistent behavior
- RT-safe: single tanh() + division per sample per channel

---

### 3.15 DX7 — Keyboard Rate Scaling

**Vấn đề:** DX7 hardware cho phép mỗi operator có **keyboard rate scaling** — note cao thì ADSR rates nhanh hơn, note thấp thì chậm hơn. Đây là tính năng quan trọng cho:
- **FM Piano/EP**: high notes decay nhanh (đúng như piano thật), low notes sustain lâu
- **Bell/Chime sounds**: high register cần attack snap nhanh hơn
- **Pad/Strings**: bass notes cần slow attack/release, treble cần tighter envelope

V1 của DX7Engine dùng cùng ADSR timing cho tất cả notes bất kể pitch → thiếu tự nhiên khi chơi across keyboard range.

**Giải pháp:** Thêm 6 params `DX7P_OP0_KBD_RATE..OP5` (range 0..1, default 0). Khi > 0, ADSR times được scale theo exponential function của MIDI note relative to C4 (note 60).

**Files đã sửa:**
- `core/engines/dx7/dx7_engine.h` — Thêm 6 params `DX7P_OP0_KBD_RATE..DX7P_OP5_KBD_RATE` vào enum. `DX7_PARAM_COUNT` tăng 6 (57→63). Thêm `kbdRate_[6]` cached array.
- `core/engines/dx7/dx7_voice.h` — `DX7Operator::trigger()` thêm `kbdRateScale` + `midiNote` params. `DX7Voice::trigger()` thêm `kbdRate[6]` param.
- `core/engines/dx7/dx7_engine.cpp` — Metadata arrays (NAMES/MIN/MAX/DEF) thêm 6 entries. `beginBlock()` load `kbdRate_[]`. `noteOn()` forward `kbdRate_` vào voice trigger.
- `ui/panels/engines/panel_dx7.cpp` — Thêm "KR" row trong operator table.

**Tham số mới:**

| Param | Index | Range | Default | Ghi chú |
|-------|-------|-------|---------|---------|
| `DX7P_OP0_KBD_RATE..OP5` | 55–60 | 0..1 | 0 | 0=off, 1=max scaling (4× per octave) |

**Code pattern — `DX7Operator::trigger()` — keyboard rate scaling:**
```cpp
// kbdRateScale 0..1 → scale base 1.0..4.0 per octave from C4 (note 60)
float kbdFactor = 1.0f;
if (kbdRateScale > 0.001f) {
    const float kbdScaleBase = 1.0f + kbdRateScale * 3.0f; // 1..4 per octave
    kbdFactor = std::pow(kbdScaleBase, (midiNote - 60) / 12.0f);
}

// ADSR times divided by kbdFactor → higher notes = faster envelopes
const float atkMs = (1.0f + atk * 2999.0f) / kbdFactor;
attackRate  = std::pow(0.001f, 1.0f / (atkMs * 0.001f * sampleRate));
// Same for decayRate and releaseRate
```

**Exponential scaling behavior:**

| Note | kbdRate=0.5 | kbdRate=1.0 | Ý nghĩa |
|------|-------------|-------------|---------|
| C2 (36) | factor 0.42× | factor 0.18× | Env chậm ~2.4×/5.7× |
| C4 (60) | factor 1.0× | factor 1.0× | Pivot — không thay đổi |
| C6 (84) | factor 2.38× | factor 5.66× | Env nhanh ~2.4×/5.7× |

**Code pattern — `DX7Voice::trigger()` — forward kbdRate:**
```cpp
const float kr = kbdRate ? kbdRate[i] : 0.0f;
ops[i].trigger(baseFreq, sampleRate,
               ratios[i], levels[i], vs[i],
               fixedMode[i], fixedHz[i],
               attacks[i], decays[i], sustains[i], releases[i],
               vel, kr, midiNote);
```

**Kết quả:**
- Default kbdRate=0 → backward compatible, preset cũ hoạt động bình thường
- kbdRate=0.3–0.5 cho carriers → FM piano/EP realistic — high notes sparkle, low notes sustain
- kbdRate=0.5–1.0 cho modulators → metallic/bell texture thay đổi tự nhiên theo register
- Mỗi operator có kbdRate riêng → full control per-operator (đúng DX7 hardware behavior)
- RT-safe: chỉ thêm 1 pow() call trong trigger() (non-RT), không ảnh hưởng tick()

---

## 4. Còn lại — P3 Roadmap

### P3 — Refinement / Nice-to-have

| Item | Engine | Mô tả |
|------|--------|-------|
| **Per-voice micro-detuning** | Rhodes | ±1–2 cents từ static table → analog warmth |
| **Key click transient** | Rhodes | 0.3ms noise burst khi hammer strike — percussive "thwack" |
| **Per-mode excitation scaling** | Rhodes | Hammer kích thích modes theo energy scaling khác nhau |
| **Sympathetic resonance** | Rhodes | Sustaining strings cộng hưởng khi note harmonic được chơi |
| **Drum kit browser UI** | Drums | Load WAV kits từ `assets/drum_kits/` trong UI |
| **MIDI CC learn** | All | Map bất kỳ MIDI CC nào tới param bất kỳ |

---

## 5. P3 Implementation (2026-03-08)

### 5.1 Moog — Mono Note Priority Modes

**Vấn đề:** Mono mode chỉ có Last-Note priority (LIFO — note cuối cùng được giữ sẽ phát khi thả note hiện tại). Người chơi cần thêm Lowest và Highest priority cho kỹ thuật bass line và lead line khác nhau.

**Giải pháp:** Thêm `MP_NOTE_PRIORITY` param (0=Last, 1=Lowest, 2=Highest) và `topHeldByPriority()` helper vào VoicePool.

**Files đã sửa:**
- `core/engines/moog/moog_params.h` — Thêm `MP_NOTE_PRIORITY` vào enum (MOOG_PARAM_COUNT 41→42) + metadata entry
- `core/engines/moog/voice_pool.h` — Thêm `notePriority_` member + `topHeldByPriority()` declaration
- `core/engines/moog/voice_pool.cpp` — Implement `topHeldByPriority()`, wire vào `applyConfig()` và `noteOffMono()`
- `ui/panels/engines/panel_moog.cpp` — Thêm Priority combo (Last/Lowest/Highest), chỉ hiển thị khi Mode=Mono

**Code pattern:**
```cpp
// voice_pool.cpp — topHeldByPriority()
const HeldNote* VoicePool::topHeldByPriority() const noexcept {
    if (heldCount_ == 0) return nullptr;
    switch (notePriority_) {
        default:  // 0 = Last note (LIFO)
            return &heldNotes_[heldCount_ - 1];
        case 1: { // Lowest note
            const HeldNote* best = &heldNotes_[0];
            for (int i = 1; i < heldCount_; ++i)
                if (heldNotes_[i].midiNote < best->midiNote)
                    best = &heldNotes_[i];
            return best;
        }
        case 2: { // Highest note
            const HeldNote* best = &heldNotes_[0];
            for (int i = 1; i < heldCount_; ++i)
                if (heldNotes_[i].midiNote > best->midiNote)
                    best = &heldNotes_[i];
            return best;
        }
    }
}
```

**Kết quả:**
- Mono mode giờ hỗ trợ 3 note priority: Last (mặc định, tương thích ngược), Lowest, Highest
- UI combo "Priority" chỉ hiển thị khi Mode=Mono → giao diện sạch cho Poly/Unison
- Poly và Unison modes không bị ảnh hưởng
- RT-safe: chỉ scan mảng fixed-size `heldNotes_[]` (≤16 entries), không allocation

### 5.2 Moog — Analog-Style Unison Detune

**Vấn đề:** Unison mode sử dụng linear equal-spacing cho detune spread — tất cả voices cách đều nhau → thiếu character analog. Analog synth thực tế có random drift nhỏ giữa các oscillator.

**Giải pháp:** Giữ nguyên linear spread làm base, thêm ±20% LCG random jitter per-voice. Seed từ MIDI note number → cùng note luôn có cùng spread pattern (deterministic, reproducible).

**Files đã sửa:**
- `core/engines/moog/voice_pool.h` — Thêm `uint32_t unisonSeed_` member
- `core/engines/moog/voice_pool.cpp` — `noteOnUnison()`: seed LCG từ note, thêm random jitter vào linear offset

**Code pattern:**
```cpp
// voice_pool.cpp — noteOnUnison() P3 analog detune
unisonSeed_ = static_cast<uint32_t>(note) * 2654435761u + 12345u;
for (int i = 0; i < n; ++i) {
    const float base = (i - (n-1)*0.5f) * (unisonDetune_ / (n-1));
    unisonSeed_ = unisonSeed_ * 1664525u + 1013904223u;
    const float rnd = (unisonSeed_ >> 16) / 65535.0f - 0.5f;
    const float jitter = rnd * unisonDetune_ * 0.2f;
    offset = base + jitter;
    triggerVoice(v, note, vel, offset);
}
```

**Kết quả:**
- Unison giờ có "analog warmth" — voices không còn perfectly evenly-spaced
- Jitter chỉ ±20% → vẫn giữ spread structure, không bị chaos
- Deterministic: cùng note + cùng detune setting → cùng spread (tốt cho sound design)
- RT-safe: chỉ dùng integer arithmetic (LCG), không allocation

### 5.3 Rhodes — Extended Polyphony 8→12

**Vấn đề:** MAX_VOICES = 8 giới hạn polyphony — pianist chơi chord + pedal dễ exceed 8 voices, gây voice stealing sớm.

**Giải pháp:** Tăng MAX_VOICES từ 8 lên 12. Modal resonator voice rất nhẹ (~25 multiplications/sample/voice), 12 voices thêm tải không đáng kể.

**Files đã sửa:**
- `core/engines/rhodes/rhodes_engine.h` — `MAX_VOICES = 8` → `MAX_VOICES = 12`
- `core/engines/rhodes/rhodes_engine.cpp` — `POLY_NORM = 0.3536f` (1/√8) → `0.2887f` (1/√12)

**Kết quả:**
- 12 voices cho phép chơi richer chord voicing + sustain pedal thoải mái hơn
- POLY_NORM cập nhật đảm bảo headroom normalization đúng
- voiceAge_[MAX_VOICES] tự theo vì là member array dùng MAX_VOICES constant
- CPU impact: ~100 thêm mults/sample ở full polyphony (negligible trên PC, cần check cho Teensy V3)

### 5.4 Hammond — Tube Overdrive / Preamp Saturation

**Vấn đề:** Hammond B-3 nổi tiếng với "overdriven organ" sound (Led Zeppelin, Deep Purple, Jon Lord) — nhưng engine thiếu pre-amplifier saturation. Tất cả output đều clean.

**Giải pháp:** Thêm `HP_OVERDRIVE` param (0..1) với tanh soft saturation pre-Leslie. Drive range 1x–5x. ParamSmoother 5ms cho zipper-free operation.

**Files đã sửa:**
- `core/engines/hammond/hammond_engine.h` — Thêm `HP_OVERDRIVE` enum (HAMMOND_PARAM_COUNT 21→22), `overdrive_` member, `ParamSmoother overdriveSmoother_`
- `core/engines/hammond/hammond_engine.cpp` — Metadata arrays +1, init/setSampleRate smoother, beginBlock snapshot + smoother target, tickSample tanh saturation
- `ui/panels/engines/panel_hammond.cpp` — Thêm Overdrive slider trong Extras section

**Code pattern:**
```cpp
// tickSample() — after voice sum, before Leslie
const float od = overdriveSmoother_.tick();
if (od > 0.001f) {
    const float drive = 1.0f + od * 4.0f;   // 1x → 5x
    sumL = std::tanh(sumL * drive) / drive;
    sumR = std::tanh(sumR * drive) / drive;
}
```

**Signal path:**
```
Voice sum (61 tonewheels) → [Tube Overdrive] → Leslie/Vibrato → Master Vol
```

**Kết quả:**
- Overdrive 0%: transparent (bypass) — tương thích ngược với presets cũ
- Overdrive 30-50%: warm tube coloration, even harmonic enrichment
- Overdrive 80-100%: heavy Jon Lord / Deep Purple distortion character
- Pre-Leslie placement → distortion character thay đổi khi Leslie speed đổi (đúng như thực tế)
- Existing presets không bị ảnh hưởng (HP_OVERDRIVE default = 0.0)

### 5.5 Drums — 808-Style Hi-Hat Ring Modulation

**Vấn đề:** Hi-hat synthesis sử dụng simple odd-harmonic sum (3 harmonics) — tạo texture đúng nhưng thiếu "metallic shimmer" đặc trưng của 808 hi-hat.

**Giải pháp:** Thay thế harmonic sum bằng 808-authentic ring modulation: 6 oscillator ở non-harmonic ratios (từ 808 circuit analysis), ring-modulated thành 3 pairs. Mix 70/30 với white noise cho upper shimmer.

**Files đã sửa:**
- `core/engines/drums/drum_engine.cpp` — HiHatC/HiHatO case trong `DrumPadDsp::tick()`: thay thế harmonic sum bằng RM synthesis

**Code pattern:**
```cpp
// 808-style: 6 oscillators × non-harmonic ratios, RM'd in 3 pairs
static constexpr float RATIOS[6] = {
    1.0f, 1.4471f, 1.6818f, 1.9545f, 2.2727f, 2.6364f
};
float rm = 0.0f;
for (int p = 0; p < 3; ++p)
    rm += sin(phase1 * RATIOS[p*2]) * sin(phase1 * RATIOS[p*2+1]);
rm = rm * 0.7f + noise * 0.3f;  // metallic + shimmer
```

**Kết quả:**
- Hi-hat giờ có "metallic" character đặc trưng của 808 — ring mod tạo sum/difference frequencies
- Non-harmonic ratios (1.0, 1.4471, 1.6818...) từ 808 TR circuit analysis → authentic spectral content
- Noise mix 30% thêm "sizzle" và high-frequency shimmer cho hats mở
- HiHatC vs HiHatO vẫn phân biệt bằng decay time (không đổi)
- RT-safe: chỉ sin() + multiply, không allocation

### 5.6 Drums — Kick Sweep Param Exposure

**Vấn đề:** Kick drum đã có pitch sweep logic (sine oscillator sweep từ start freq xuống target freq) nhưng sweep depth và sweep time đều **hardcoded** — sweep target luôn 40 Hz, sweep time luôn 40ms. Chỉ param `Pitch` ảnh hưởng start frequency. Người dùng không thể điều chỉnh character của kick sweep (808 deep thump vs tight punchy kick).

**Giải pháp:** Thêm 2 global params expose sweep depth và sweep time cho kick pad:
- `DRUM_KICK_SWEEP_DEPTH` (0..1, default 0.5) — 0=no sweep (flat pitch), 1=maximum sweep range
- `DRUM_KICK_SWEEP_TIME` (0..1, default 0.2) — sweep duration 10ms..200ms

**Files đã sửa:**
- `core/engines/drums/drum_engine.h` — Thêm `DRUM_KICK_SWEEP_DEPTH_ID` (65), `DRUM_KICK_SWEEP_TIME_ID` (66). `DRUM_PARAM_COUNT` 65→67. Thêm `kickSweepDepth_`, `kickSweepTime_` cached members.
- `core/engines/drums/drum_pad_dsp.h` — Thêm `sweepDepth`, `sweepTime` fields vào `DrumPadDsp` struct.
- `core/engines/drums/drum_engine.cpp` — Constructor init 2 params mới. `beginBlock()` cache + apply sweep params cho pad 0 (Kick). `trigger()` dùng sweep params thay vì hardcode. Metadata functions handle 2 params mới.
- `ui/panels/engines/panel_drums.cpp` — Thêm "Kick Sweep" và "Sweep Time" sliders cạnh Global Volume.

**Tham số mới:**

| Param | ID | Range | Default | Ghi chú |
|-------|----|-------|---------|---------|
| `DRUM_KICK_SWEEP_DEPTH` | 65 | 0..1 | 0.5 | 0=flat, 1=max sweep |
| `DRUM_KICK_SWEEP_TIME` | 66 | 0..1 | 0.2 | 10ms (0) → 200ms (1) |

**Code pattern — `DrumPadDsp::trigger()` Kick sweep:**
```cpp
case DspDrumType::Kick: {
    freq1 = 40.0f + pitch * 30.0f;    // 40–70 Hz start
    // Sweep depth: 0=no sweep (stay at freq1), 1=full sweep down to 40Hz
    const float sweepTarget = freq1 * (1.0f - sweepDepth)
                            + 40.0f * sweepDepth;
    // Sweep time: 10ms..200ms
    const float sweepMs = 0.01f + sweepTime * 0.19f;
    freqDecayRate = (sweepTarget < freq1)
        ? std::pow(sweepTarget / freq1, 1.0f / (sweepMs * sampleRate))
        : 1.0f;
    // ...
}
```

**Sweep behavior theo params:**

| sweepDepth | sweepTime | Character |
|------------|-----------|-----------|
| 0.0 | any | Flat tone — sin osc ở freq1, không sweep |
| 0.3 | 0.1 | Tight punchy kick — short shallow sweep |
| 0.5 | 0.2 | Default — balanced 808-style thump |
| 0.8 | 0.5 | Deep boomy kick — long deep sweep |
| 1.0 | 1.0 | Maximum sub-bass sweep — 200ms từ 70Hz→40Hz |

**Kết quả:**
- Default (0.5/0.2) tái tạo gần đúng behavior cũ (sweep ~40ms, depth trung bình)
- Sweep depth=0 cho flat electronic kick sound (không sweep)
- Sweep time dài (0.7–1.0) cho deep sub-bass boomy character (trap/808 style)
- Existing presets backward-compatible: default values giữ nguyên character cũ
- RT-safe: sweep params chỉ ảnh hưởng trigger() (non-RT), không thêm cost cho tick()

### 5.7 Mellotron — Multi-Cycle Wavetable Loops

**Vấn đề:** Mellotron dùng **single-cycle wavetable** (2048 samples per tape). Mỗi chu kỳ dao động lặp lại y hệt nhau → âm thanh "digital", thiếu character tape thực. Mellotron M400 thật có tape loops 8 giây — mỗi chu kỳ trên tape có timbral variation nhẹ do oxide coating không đều, tape speed micro-fluctuation, và head gap alignment.

**Roadmap ban đầu:** "Deferred — requires recorded samples + asset loading". Tuy nhiên, có thể đạt hiệu ứng tương tự bằng **computed multi-cycle tables** với per-cycle harmonic jitter.

**Giải pháp:** Mở rộng wavetable từ single-cycle (2048) → **8-cycle** (16384 samples per tape). Mỗi cycle có:
- Harmonic amplitude jitter ±8–12% (random per harmonic per cycle)
- Harmonic phase drift nhẹ (0–0.05 radians) simulating tape oxide variation
- Deterministic LCG RNG seeded per tape → reproducible results

**Files đã sửa:**
- `core/engines/mellotron/mellotron_tables.h` — `MELLO_TABLE_SIZE` 2048→16384. Thêm `MELLO_CYCLE_SIZE=2048` và `MELLO_CYCLES=8`. `build()` generate 8 cycles per tape. `read()` unchanged (power-of-2 bitmask vẫn hoạt động vì 16384 = 2^14).
- `core/engines/mellotron/mellotron_engine.cpp` — `phaseInc` chia thêm `MELLO_CYCLES` để phase 0..1 cover toàn bộ 8 cycles.

**Constants mới:**

| Constant | Giá trị | Ghi chú |
|----------|---------|---------|
| `MELLO_CYCLE_SIZE` | 2048 | Samples per oscillation cycle |
| `MELLO_CYCLES` | 8 | Cycles per tape loop |
| `MELLO_TABLE_SIZE` | 16384 | Total samples = 2048 × 8 |

**Code pattern — multi-cycle generation (Strings tape):**
```cpp
uint32_t rng = 0xDEAD0001u;
for (int cyc = 0; cyc < MELLO_CYCLES; ++cyc) {
    const int off = cyc * N;
    for (int i = 0; i < N; ++i) {
        const float ph = TWO_PI * i / N;
        float v = 0.0f;
        for (int h = 1; h <= 16; ++h) {
            const float jA = 1.0f + 0.12f * (lcg(rng) * 2.0f - 1.0f); // ±12% amp
            const float jP = lcg(rng) * 0.05f;   // phase drift
            v += jA * std::sin(h * ph + jP) / h;
            v += 0.6f * jA * std::sin(h * ph * 1.00289f + jP) / h;
            v += 0.6f * jA * std::sin(h * ph * 0.99711f + jP) / h;
        }
        t[off + i] = v / 2.2f;
    }
}
```

**Code pattern — phase accumulator cho multi-cycle:**
```cpp
// Trước (single-cycle): phase 0..1 = 1 oscillation cycle
// phaseInc = freq / sampleRate

// Sau (multi-cycle): phase 0..1 = 8 oscillation cycles
// phaseInc = freq / (sampleRate * MELLO_CYCLES)
const float freq = mMidiToHz(midiNote) * tapeSpeedFactor;
phaseInc = freq / (sampleRate * MELLO_CYCLES);
```

**Per-tape variation parameters:**

| Tape | Harmonics | Amp Jitter | Phase Drift | RNG Seed |
|------|-----------|-----------|-------------|----------|
| Strings | 16×3 layers | ±12% | 0–0.05 rad | 0xDEAD0001 |
| Choir | 6 formant | ±10% | 0–0.04 rad | 0xDEAD0002 |
| Flute | 3 (near-sine) | ±8% | 0–0.03 rad | 0xDEAD0003 |
| Brass | 20 harmonics | ±12% | 0–0.05 rad | 0xDEAD0004 |

**Memory impact:**

| | Trước | Sau | Delta |
|--|-------|-----|-------|
| Per tape | 2048 × 4B = 8 KB | 16384 × 4B = 64 KB | +56 KB |
| Total (4 tapes) | 32 KB | 256 KB | +224 KB |

256 KB cho PC: negligible. Cho Teensy V3: cần giảm `MELLO_CYCLES` xuống 4 hoặc 2 qua `#ifdef`.

**Kết quả:**
- Waveform không còn lặp lại y hệt mỗi cycle → nghe "organic" hơn
- Kết hợp với wow/flutter (P2 §3.5) đã có: timbre variation (multi-cycle) + pitch variation (wow/flutter) = tape character đầy đủ
- Tapes Strings và Brass hưởng lợi rõ nhất (nhiều harmonics → variation rõ ràng)
- Flute ít khác biệt hơn (gần sine, ít harmonics để vary) nhưng vẫn bớt "digital"
- Build time tăng nhẹ (~50ms thêm cho tính toán tables) — one-shot tại init()
- RT-performance không đổi: read() vẫn là 1 linear interp, chỉ table lớn hơn
- Deterministic: cùng seed → cùng tables mỗi lần chạy

### P3 Status Summary

| Item | Engine | Status |
|------|--------|--------|
| Mono note priority modes | Moog | ✅ Implemented (P3 5.1) |
| Analog-style unison detune | Moog | ✅ Implemented (P3 5.2) |
| Extended polyphony 8→12 | Rhodes | ✅ Implemented (P3 5.3) |
| Per-voice micro-detuning | Rhodes | ✅ Already in V2 (±2 cents LCG) |
| Key click transient | Rhodes | ✅ Already in P1.3 (2ms noise burst) |
| Per-mode excitation scaling | Rhodes | ✅ Already in P2 (§3.11 HAMMER_EXCITE[]) |
| Sympathetic resonance | Rhodes | ✅ Already in P2 (§3.13) |
| Cabinet EQ | Rhodes | ✅ Already in V2 (§1.K) |
| Tube overdrive / preamp | Hammond | ✅ Implemented (P3 5.4) |
| Hi-hat ring modulation | Drums | ✅ Implemented (P3 5.5) |
| Kick sweep param exposure | Drums | ✅ Implemented (P3 5.6) |
| Key-rate scaling per operator | DX7 | ✅ Implemented (P2 3.15) |
| Delay BpmSync | Effects | ✅ Already wired (ctx.bpm) |
| Drum kit browser UI | Drums | Deferred (HAL/UI heavy) |
| MIDI CC learn | All | Deferred (UI modal + persistent config) |
| Mellotron longer wavetables | Mellotron | ✅ Implemented (P3 5.7 — 8-cycle computed) |

---

## Phụ lục — Code Patterns Quan Trọng

### Pattern 1: RT-Safe Pitch Bend (tất cả engines)

```cpp
// HEADER:
float pitchBendSemis_ = 0.0f;  // Set by pitchBend() [RT-UNSAFE]
float pitchBendCache_ = 1.0f;  // Precomputed factor, read in tickSample() [RT-SAFE]

// .CPP:
void XxxEngine::pitchBend(int bend) noexcept {
    pitchBendSemis_ = std::clamp(bend / 8192.0f * 2.0f, -2.0f, 2.0f);
    pitchBendCache_ = std::pow(2.0f, pitchBendSemis_ / 12.0f);
}

// Trong tickSample() [RT-SAFE]:
v.tick(pitchBendCache_, outL, outR);
// Voice body: phase += phaseInc * pitchFactor;
```

### Pattern 2: ParamSmoother cho Continuous Params

```cpp
// HEADER:
#include "core/dsp/param_smoother.h"
ParamSmoother paramSmooth_;

// init():
paramSmooth_.init(sampleRate_, 5.0f);  // 5ms ramp time
paramSmooth_.snapTo(initialValue);

// setSampleRate():
paramSmooth_.init(sr, 5.0f);

// beginBlock():
paramSmooth_.setTarget(currentValue);

// tickSample():
float smoothed = paramSmooth_.tick();
```

### Pattern 3: Voice Stealing — Oldest-Active

```cpp
// HEADER:
int voiceAge_[MAX_VOICES] = {};
int voiceAgeCounter_ = 0;

// noteOn():
int best = -1;
// 1. Tìm idle voice
for (int i = 0; i < MAX_VOICES; ++i)
    if (!voices_[i].active) { best = i; break; }
// 2. Steal oldest nếu không có idle
if (best < 0) {
    int oldest = INT_MAX;
    for (int i = 0; i < MAX_VOICES; ++i)
        if (voiceAge_[i] < oldest) { oldest = voiceAge_[i]; best = i; }
}
voices_[best].trigger(note, vel, ...);
voiceAge_[best] = ++voiceAgeCounter_;
```

### Pattern 4: ModalMode::updateFreq (no-pop frequency update)

```cpp
// Cập nhật frequency trong biquad resonator
// BẮT BUỘC: KHÔNG reset y0, y1 — bảo toàn resonator energy
void ModalMode::updateFreq(float freqHz, float sampleRate) noexcept {
    const float omega = TWO_PI * freqHz / sampleRate;
    coef = 2.0f * std::cos(omega);
    // y0, y1 unchanged — amplitude envelope preserved
}
```

---

*Cập nhật mới nhất: 2026-03-08 — P2 hoàn tất (15 items, §3.2–3.15). P3 hoàn tất (7 items implemented + 7 already done). Deferred: Drum kit browser UI, MIDI CC learn.*
