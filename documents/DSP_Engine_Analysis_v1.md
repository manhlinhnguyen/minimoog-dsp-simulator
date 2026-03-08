# DSP Engine Technical Analysis
## MiniMoog DSP Simulator V2.2 — All 6 Engines

**Date:** 2026-03-08
**Codebase:** `C:\codes\minimoog_digitwin`
**Author:** Claude Code (analysis pass)

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Common Architecture](#2-common-architecture)
3. [Engine 1 — MoogEngine (Subtractive)](#3-engine-1--moogengine)
4. [Engine 2 — HammondB3Engine (Additive Tonewheel)](#4-engine-2--hammondb3engine)
5. [Engine 3 — RhodesEngine (Modal Resonator)](#5-engine-3--rhodesengine)
6. [Engine 4 — DX7Engine (6-Op FM)](#6-engine-4--dx7engine)
7. [Engine 5 — MellotronEngine (Wavetable)](#7-engine-5--mellotronengine)
8. [Engine 6 — DrumEngine (Hybrid DSP+Sample)](#8-engine-6--drumengine)
9. [Cross-Engine Analysis](#9-cross-engine-analysis)
10. [Priority Improvement Roadmap](#10-priority-improvement-roadmap)
11. [Performance Analysis](#11-performance-analysis)

---

## 1. Executive Summary

The simulator implements six independent synthesis engines behind a unified `IEngine` interface, selectable at runtime without audio interruption. Each engine owns its parameters, voices, and rendering loop; a single shared `EngineManager` provides the music layer (Arp, Seq, Chord, Scale) and effect chain on top.

| # | Engine | Paradigm | Voices | Params | File |
|---|--------|----------|--------|--------|------|
| 1 | MiniMoog Model D | Subtractive (VCO→Filter→VCA) | 1–8 (mono/poly/unison) | 41 | `core/engines/moog/` |
| 2 | Hammond B-3 | Additive tonewheel (9 drawbars) | 61 (one per key) | 17 | `core/engines/hammond/` |
| 3 | Rhodes Mark I | Modal resonator (physical model) | 8 | 9 | `core/engines/rhodes/` |
| 4 | Yamaha DX7 | 6-operator FM, 32 algorithms | 8 | 45 | `core/engines/dx7/` |
| 5 | Mellotron M400 | Wavetable (tape emulation) | 35 | 7 | `core/engines/mellotron/` |
| 6 | Hybrid Drum Machine | DSP synthesis + WAV sample | 16 pads (one-shot) | 65 | `core/engines/drums/` |

**Overall quality:** Moog and Rhodes are the strongest — Moog has the most complete modulation matrix, Rhodes has the most physically accurate voice model. Hammond and DX7 are solid but have well-defined gaps. Mellotron is minimal and correct. DrumEngine is feature-complete but DSP-light.

**Critical bug:** The `IEngine::pitchBend()` method is declared in the interface but is a no-op in 5 of the 6 engines (only Moog implements it). This silently drops all pitch-bend MIDI events for Hammond, Rhodes, DX7, Mellotron, and Drums.

---

## 2. Common Architecture

### 2.1 IEngine Interface

Defined in `core/engines/iengine.h`. All engines implement:

```cpp
virtual void init(float sampleRate) noexcept = 0;
virtual void beginBlock(int nFrames) noexcept {}    // param snapshot
virtual void tickSample(float& outL, float& outR) noexcept = 0;
virtual void noteOn/noteOff/allNotesOff/allSoundOff() noexcept = 0;
virtual void pitchBend(int bend) noexcept = 0;      // -8192..+8191
virtual void controlChange(int cc, int val) noexcept = 0;
virtual void setParam/getParam(int id, float v) noexcept = 0;
```

The `beginBlock()` hook is the key design pattern: each engine snapshots its `std::atomic<float> params_[]` into a plain `float paramCache_[]` once per block, then `tickSample()` reads only from the cache — no atomics in the hot path.

### 2.2 Thread Model

```
UI Thread          → engine->setParam()     → params_[] (atomic store)
Audio Thread       → beginBlock()           → paramCache_[] (atomic load, once/block)
                   → tickSample() × N       → paramCache_[] (plain float reads)
MIDI Thread        → MidiEventQueue (SPSC)  → drained in processBlock()
```

### 2.3 EngineManager

`core/engines/engine_manager.h` owns:
- All 6 engine instances (via `std::unique_ptr<IEngine>`)
- Music layer: `Arpeggiator`, `StepSequencer`, `ChordEngine`, `ScaleQuantizer`
- Oscilloscope ring buffer (stereo, 2048 samples, written post-effect)
- Active engine index via `std::atomic<int> activeIdx_`

Engine switching is wait-free from the UI thread: `switchEngine()` calls `allSoundOff()` on the old engine then atomically updates `activeIdx_`. The audio thread reads `activeIdx_` once per block.

---

## 3. Engine 1 — MoogEngine

**Files:** `core/engines/moog/moog_engine.{h,cpp}`, `voice.{h,cpp}`, `voice_pool.{h,cpp}`, `moog_params.h`

### 3.1 Signal Chain

```
OSC1 (VCO: tri/saw/square/pulse) ─┐
OSC2 (VCO: same waveforms)        ├──► MIXER ──► 4-pole Ladder Filter ──► VCA ──► output
OSC3 (dual: audio or LFO)         ┤              (tanh saturation)       (AmpEnv)
NOISE (white/pink)                ─┘    ▲
                                        │
                            FilterEnv + KbdTrack + ModMatrix
                                        │
                            LFO source: OSC3 (when P_OSC3_LFO_ON)
                            or ModMix blend: OSC3 ↔ Noise
```

**Oscillators:** Phase-accumulator with 6 waveforms: triangle, shark-tooth, reverse saw, sawtooth, square, pulse. Each oscillator has independent range switching (32', 16', 8', 4', 2') and per-semitone frequency offset (±7 semitones). Hard-sync not implemented.

**Filter:** 4-pole Moog ladder with `tanh()` saturation on each stage. Keyboard tracking: 0 / 1/3 / 2/3 of semitone interval. Filter envelope with full ADSR.

**Modulation matrix:** OSC3 (in LFO mode) + noise → `ModMix` blend → OSC pitch mod and/or filter cutoff mod, gated by `MP_OSC_MOD_ON` / `MP_FILTER_MOD_ON`.

**Glide:** `ParamSmoother`-based portamento on note frequency, 0–5000ms (quadratic response curve).

### 3.2 Voice Management

Implemented in `VoicePool`. Three modes (`MP_VOICE_MODE`):
- **Mono** (1 voice, glide-aware, retrigger last note)
- **Poly** (1–8 voices, voice stealing)
- **Unison** (all active voices detune-spread, 1/√N amplitude normalization)

Three voice-steal strategies (`MP_VOICE_STEAL`):
- `0` = oldest-active (default)
- `1` = lowest-note
- `2` = quietest-voice (reads AmpEnv amplitude)

### 3.3 Parameters (41 total)

Complete: Master Tune, Master Volume, Glide On/Time, Mod Mix, OSC Mod/Filter Mod toggles, OSC3 LFO mode, 3 × (On, Range, Freq, Wave), 4 × Mixer levels + Noise Color, Filter Cutoff/Emphasis/Env Amount/KbdTrack, Filter ADSR, Amp ADSR, Voice Mode/Count/Steal, Unison Detune.

### 3.4 ✅ Strengths

- **Most complete modulation matrix** of all 6 engines — OSC3 dual-mode, Noise, ModMix blend.
- **Three voice-steal strategies** — only engine to implement this.
- **`ParamSmoother`** on every continuous parameter — no zipper noise on any knob.
- **Correct `beginBlock()` param snapshot** — no atomics in hot path.
- **`getOscBuffer()`** implemented — oscilloscope works for Moog.
- **RT annotations** (`// [RT-SAFE]`) present on all hot-path methods.
- **Hard sync not present** but architecture allows adding it per-oscillator pair.

### 3.5 ⚠️ Weaknesses

- **No OSC hard sync** — classic Moog patch not achievable.
- **No note priority option** (last-note vs first-note) — VoicePool always steals on `noteOn`, but for mono mode, last-note priority might not match all use cases.
- **Unison spread** uses fixed linear detune spacing; a random or logarithmic distribution would sound more analog.
- **`MP_OSC2_RANGE` / `MP_OSC3_RANGE`** — OSC2 has "LO" range like OSC1, but original Model D OSC2 starts at 32', not LO. Minor documentation mismatch.
- **Filter emphasis at max** (resonance = 1.0) can produce loud self-oscillation without output limiter — can clip to `±1` hard.

### 3.6 🔧 Improvement Proposals

**M1 — Hard sync OSC1 → OSC2 (P2, medium effort)**
Add `syncPhase` flag in `Voice::tick()`. When OSC1 phase wraps, reset OSC2 phase to 0. Requires OSC2 to expose phase reset method.

**M2 — Output limiter after VCA (P2, low effort)**
```cpp
// In Voice::tick(), after amp envelope:
out = std::tanh(out * 0.8f) * 1.25f;  // soft clip ±1
```

**M3 — Analog-style unison detune (P3, low effort)**
Replace linear spacing with LCG-generated offsets seeded per voice, refreshed on `noteOn`:
```cpp
float spread[MAX_VOICES];
for (int i = 0; i < count; ++i)
    spread[i] = (rng() / float(UINT32_MAX) - 0.5f) * maxCents;
```

**M4 — Note priority modes for mono (P3, low effort)**
Add `MP_NOTE_PRIORITY` param (0=last-note, 1=low-note, 2=high-note). Track held-note stack in VoicePool.

---

## 4. Engine 2 — HammondB3Engine

**Files:** `core/engines/hammond/hammond_engine.{h,cpp}`, `hammond_voice.{h,cpp}`

### 4.1 Signal Chain

```
9 drawbar oscillators per key (additive sine):
  16',  5⅓',  8',  4',  2⅔',  2',  1⅗',  1⅓',  1'
  × harmonic ratios: 0.5, 0.75, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0
  × drawbar level   (0–8 integer, normalized to 0.0–1.0)
  ↓
SUM → percussion layer (optional 2nd or 3rd harmonic, decaying envelope)
  ↓
Key-click transient (broadband noise burst per noteOn)
  ↓
Vibrato/Chorus LFO (engine-wide, phase-modulates all voices)
  ↓
Output (mono mix, no stereo spread)
```

**Up to 61 simultaneous voices** (full B-3 keyboard). Each voice is a `HammondVoice` containing 9 phase accumulators and a noise generator for key click.

**Percussion:** First-note trigger only (resets when all keys released). `percFired_` flag implements classic B-3 first-note percussion behavior. Choice of 2nd or 3rd harmonic, fast (600ms) or slow (1500ms) decay.

**Vibrato/Chorus:** Simple sinusoidal LFO applied as a uniform phase offset to all voices simultaneously (modes V1–V3, C1–C3 differentiated by LFO depth). This is a simplification — see §4.5.

### 4.2 Voice Management

No voice stealing needed: organs sustain indefinitely while key is held. `noteOff` calls `voice.release()` (zeroes `clickEnv`, marks inactive immediately — no decay; Hammond keys have no damper).

### 4.3 Parameters (17 total)

`HP_DRAWBAR_1..9` (0–8), `HP_PERC_ON`, `HP_PERC_HARM`, `HP_PERC_DECAY`, `HP_PERC_LEVEL`, `HP_VIB_MODE` (0–6), `HP_VIB_DEPTH`, `HP_CLICK_LEVEL`, `HP_MASTER_VOL`.

### 4.4 ✅ Strengths

- **Correct additive synthesis** — all 9 drawbars, accurate harmonic ratios (0.5× to 8×).
- **First-note percussion** correctly gated by `percFired_` — matches real B-3 behavior.
- **Key-click transient** via `NoiseGenerator` per voice — adds organic attack.
- **61-voice capacity** — realistic for a full keyboard hold.
- **`HP_PERC_HARM`** — user-selectable 2nd or 3rd harmonic percussion.
- **`beginBlock()` param snapshot** correctly implemented.

### 4.5 ⚠️ Weaknesses

- **No true Leslie cabinet simulation** — vibrato/chorus is implemented as a simple sinusoidal phase offset on all voices (makes pitch wobble uniform rather than the characteristic rotary speaker Doppler effect). A real Leslie has: dual rotors (treble horn + bass rotor), different rotation speeds, Doppler pitch shift, amplitude modulation from room reflection.
- **No drawbar click suppression (zipper noise)** — drawbar value is read each `tick()` directly from `paramCache_`; fast drawbar moves create audible steps in the additive sum. Real Hammond uses fader contact resistance and capacitor decoupling.
- **Pitch bend is a no-op** (`pitchBend() {}`) — keyboard pitch bend has no effect.
- **Monophonic output** — no stereo imaging (no voice panning by key position).
- **Vibrato modes V1/V2/V3 and C1/C2/C3** differ only by `vibDepth_` scaling, not by a true scanner vibrato circuit delay-line approximation.
- **No leakage between tonewheels** — real Hammond has crosstalk (adds character).
- **No overdrive/distortion model** — the "overdriven Hammond" sound requires tube amp saturation.

### 4.6 🔧 Improvement Proposals

**H1 — Drawbar smoothing (P1, low effort)**
Apply per-drawbar `ParamSmoother` (5ms) in `beginBlock()`:
```cpp
// In HammondEngine, add:
ParamSmoother drawbarSmooth_[9];
// In beginBlock():
for (int i = 0; i < 9; ++i)
    drawbarsSmoothed_[i] = drawbarSmooth_[i].process(drawbars_[i]);
// Pass drawbarsSmoothed_ to voice.tick()
```

**H2 — True Leslie rotary cabinet (P2, high effort)**
Implement dual-rotor delay-based Leslie:
```cpp
// Two channels: treble (horn, ~400 RPM fast / ~48 RPM slow)
//               bass   (rotor, ~340 RPM fast / ~40 RPM slow)
// Each channel: variable delay line (~2ms swing) modulated by rotor phase
// + AM envelope from room reflection model (cos²)
// Ramp motor speed smoothly on fast/slow switch (inertia ~1s)
```

**H3 — Pitch bend (P1, low effort)**
Track `pitchBendSemis_` and apply to each voice's `baseFreq` × `pow(2, semis/12)`.

**H4 — Tube overdrive (P3, medium effort)**
Post-mix soft saturation (`tanh` with adjustable drive parameter), emulating the Hammond's internal preamp and connected tone cabinet.

---

## 5. Engine 3 — RhodesEngine

**Files:** `core/engines/rhodes/rhodes_engine.{h,cpp}`, `rhodes_voice.h`

### 5.1 Signal Chain (per voice)

```
MIDI noteOn
  ↓
Hammer exciter: velocity-shaped impulse burst (1.5–8ms)
  + key click noise burst (2ms broadband, P1.3)
  ↓
4 Modal Resonators (biquad IIR, decaying):
  mode0: 1.000× fundamental  (longest decay — sustain)
  mode1: 2.045× 2nd partial  (40% decay — body/warmth)
  mode2: 5.979× bell partial (12% decay — attack chime)
  mode3: 9.210× high partial  (3% decay  — attack transient)
  ↓
P3.1: Torsional mode (6 cents sharp, 80% decay) → AM "wobble"
  ↓
Pickup polynomial waveshaper: y = x + α·x² + β·x³
  (α, β driven by Tone + Velocity — even/odd harmonic richness)
  ↓
× VOICE_NORM (0.32)
  ↓
Release damper: 2-stage (30ms fast felt contact + user releaseMs slow)
  ↓
Output saturation (tanh at engine level), DC blocker, cabinet biquad EQ
  ↓
Stereo note-based pan + tremolo autopan
```

### 5.2 Physical Model Accuracy

This is the most physically-grounded voice in the codebase. Key decisions backed by Tolonen (1998) Rhodes Mark I measurements:
- **Inharmonic partial ratios** (2.045×, 5.979×, 9.210×) rather than integer multiples.
- **Per-mode inharmonic offsets** (e.g., mode1 +0.035 semitones, mode2 −0.040) from tine stiffness.
- **Piano-stretch tuning** applied per mode: `(midiNote - 60) × 0.0008` semitones/mode.
- **Velocity → hammer width** (8ms at pp → 1.5ms at ff) for brightness scaling.
- **Micro-detuning seed** (LCG per noteOn) for analog character.
- **Sympathetic resonance** (`injectSympatheticEnergy()` — adjacent voice coupling on noteOn).

### 5.3 ✅ Strengths

- **Strongest physical model** in the codebase — voice architecture follows published tine physics.
- **Torsional mode** for characteristic "wobble" shimmer — unique detail.
- **2-stage damper release** (fast felt contact then slow decay) — realistic damper behavior.
- **Pickup polynomial** with tone + velocity influence — dynamic timbral response.
- **Cabinet EQ** (low shelf +2dB@200Hz, high shelf −8dB@8kHz) — vintage amp coloration.
- **DC blocker** (biquad ~5Hz HPF) — prevents DC drift from pickup nonlinearity.
- **Sustain pedal** (CC64) correctly defers noteOff while pedal is held.
- **Oldest-voice stealing** (correct — same strategy as Moog default).
- **`[RT-SAFE]` annotations** on voice-level `tick()`.

### 5.4 ⚠️ Weaknesses

- **Pitch bend is a no-op** (`pitchBend() {}`) — significant omission for a keyboard instrument.
- **No `ParamSmoother`** on engine-level parameters — moving `RP_TONE` or `RP_DECAY` while playing causes zipper noise (parameters only snap on next noteOn).
- **`injectSympatheticEnergy()` is undocumented** in the engine header — it iterates all voices on each noteOn; with 8 voices this is 8 iterations (negligible, but unannounced).
- **Tremolo is stereo autopan** (L/R 180° out of phase), which is correct for the Rhodes Suitcase, but not labeled as such — could confuse users expecting amplitude tremolo.
- **No vibrato** (pitch LFO) — real players used foot-pedal vibrato or external phaser/chorus.
- **No `getOscBuffer()` override** — oscilloscope shows EngineManager's captured output buffer, not the engine's internal diagnostic buffer.

### 5.5 🔧 Improvement Proposals

**R1 — Pitch bend (P1, low effort)**
```cpp
// In RhodesEngine:
float pitchBendSemis_ = 0.0f;
void pitchBend(int bend) noexcept override {
    pitchBendSemis_ = (bend / 8192.0f) * 2.0f;  // ±2 semitones
}
// In voice->trigger(), multiply baseHz by pow(2, pitchBendSemis_/12)
```

**R2 — ParamSmoother on Tone/Decay/Drive (P1, low effort)**
Add `ParamSmoother toneSmooth_, decaySmooth_, driveSmooth_` and process them in `beginBlock()`. Pass smoothed values to voice's `tick()` for parameters that affect per-sample rendering.

**R3 — Vibrato LFO (P2, medium effort)**
Add `RP_VIBRATO_RATE` and `RP_VIBRATO_DEPTH`. Implement a sinusoidal LFO that modulates the pitch multiplier passed into voice `tick()`. Requires refactoring voice to accept per-sample pitch modulation instead of fixed-at-trigger `baseHz`.

**R4 — Extended polyphony to 12 (P3, low effort)**
Change `MAX_VOICES = 12`. The modal resonator voice is lightweight (~5 multiplications per sample per mode = 25 mults/voice total). 12 voices add minimal CPU burden.

---

## 6. Engine 4 — DX7Engine

**Files:** `core/engines/dx7/dx7_engine.{h,cpp}`, `dx7_voice.h`, `dx7_algorithms.h`

### 6.1 Signal Chain

```
6 operators (each: phase accumulator + linear ADSR envelope + velocity scaling)
  ↓
32-algorithm routing (dx7_algorithms.h lookup table):
  Modulator ops → modulate phase of carrier ops
  Carrier ops → sum to output
  ↓
Feedback (op0 → own phase modulated by op0 previous output × feedback amount)
  ↓
Sum of carriers → stereo output (mono signal, no pan)
```

**Operators:** Each `DX7Operator` is a phase accumulator with:
- Frequency = `baseFreq × ratio` (ratio range: 0.5–8.0)
- Linear ADSR envelope (attack/decay/release rates, sustain level)
- Velocity sensitivity per operator
- Feedback on op0 only

**Algorithm table** (`dx7_algorithms.h`): All 32 original DX7 algorithms encoded as operator dependency graphs.

### 6.2 ✅ Strengths

- **All 32 DX7 algorithms** implemented — complete topology coverage.
- **Per-operator velocity sensitivity** — dynamic timbral shaping.
- **Feedback on op0** — self-FM for brass/organ tones.
- **`beginBlock()` param snapshot** correctly implemented.
- **8-voice polyphony** — adequate for DX7-style voicing.

### 6.3 ⚠️ Weaknesses

**Critical:**
- **Pitch bend is a no-op** (`pitchBend() {}`) — arguably the worst omission since DX7 was a keyboard instrument used extensively for pitch-bend expression.
- **Voice stealing is round-robin** (`voiceIdx_ % 8`): no age, loudness, or note-match awareness. This causes audible pops when stealing a voice mid-sustain.

**DSP accuracy:**
- **Linear ADSR** — real DX7 uses a 4-rate/4-level (4R4L) exponential envelope with logarithmic rates. The simplified linear ADSR approximates the envelope shape but misses the DX7's characteristic "snap" on fast decays.
- **Operator ratio range** is 0.5–8.0 — real DX7 supports 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0 and fixed-frequency operators (Hz mode, not ratio). The 0.5–8.0 restriction limits preset accuracy.
- **No key-rate scaling** — real DX7 operators decay faster in the upper registers (key rate scaling). Without it, high notes sustain too long.
- **Feedback path** is implemented only for op0; DX7 feedback is actually op6→op6 (last operator, 1-sample delay). The implementation uses `feedbackBuf` (previous tick output) which is correct in form, but the operator index convention differs from the original.
- **No operator key-level scaling** — real DX7 has amplitude curves that scale operator output based on note pitch (important for piano and EP patches).

**Architecture:**
- **No `ParamSmoother`** — changing algorithm or ratios mid-note causes pops.
- **`getActiveVoices()`** iterates the `voices_` array on each UI call (no cached count).

### 6.4 🔧 Improvement Proposals

**D1 — Fix voice stealing (P1, low effort)**
Replace round-robin with oldest-active strategy, same as Moog/Rhodes:
```cpp
// In DX7Engine, add:
int   voiceAge_[MAX_VOICES] = {};
int   voiceAgeCounter_ = 0;

// In noteOn():
// 1. Find free slot (active == false)
// 2. If none, find min(voiceAge_[i]) → steal that voice
// 3. voiceAge_[chosen] = voiceAgeCounter_++
```

**D2 — Pitch bend (P1, low effort)**
```cpp
float pitchBendSemis_ = 0.0f;
void pitchBend(int bend) noexcept override {
    pitchBendSemis_ = (bend / 8192.0f) * 2.0f;
}
// In DX7Voice::trigger(), multiply baseFreq by pow(2, pitchBendSemis_/12)
// Or apply per-sample as phaseInc multiplier (cleaner for real-time bend)
```

**D3 — Extend ratio range + fixed-Hz operators (P2, medium effort)**
Add `DX7P_OP{n}_FIXED` toggle (0=ratio, 1=fixed Hz). When fixed, param value directly sets Hz (20–8000Hz range). Extend ratio steps to match original DX7 coarse values.

**D4 — Exponential envelope rates (P2, medium effort)**
Replace linear `envVal += attackRate` with exponential approach:
```cpp
// Attack: envVal = 1.0 - (1.0 - envVal) * attackDecayCoef
// Decay:  envVal = sustainLvl + (envVal - sustainLvl) * decayCoef
```
Pre-compute coefficients from ADSR params in `trigger()`.

**D5 — Key-rate scaling (P2, medium effort)**
Per operator, add `keyRateScale` param (0–7 in DX7 spec). In `trigger()`:
```cpp
float krsMultiplier = 1.0f + keyRateScale * (midiNote - 60) / 12.0f * 0.5f;
decayRate_adjusted = decayRate * krsMultiplier;
```

---

## 7. Engine 5 — MellotronEngine

**Files:** `core/engines/mellotron/mellotron_engine.{h,cpp}`, `mellotron_voice.h`, `mellotron_tables.h`

### 7.1 Signal Chain

```
MIDI noteOn → MellotronVoice::trigger()
  ↓
Phase increment = tableSize × (noteFreq / tableFreq) × tapeSpeed / sampleRate
  ↓
Linear wavetable interpolation (read from mellotron_tables.h static array)
  ↓
ADSR attack/release envelope (tape motor start/stop feel)
  ↓
Tape runout: amplitude fades after runoutTimeSec (tape reel end emulation)
  ↓
Random pitch spread per-note (LCG-generated offset)
  ↓
Output (mono; no stereo per-note)
```

**Wavetables:** 4 tape samples in `mellotron_tables.h`: Strings, Choir, Flute, Brass. Each table is a single cycle (or short loop) of the recorded tape source, pitched-shifted by ratio for each note.

**Up to 35 simultaneous voices** (M400 keyboard compass: C2–C5, 35 keys).

### 7.2 Parameters (7 total)

`MP_TAPE` (0–3 tape type), `MP_VOLUME`, `MP_PITCH_SPREAD` (0–1 random detune), `MP_TAPE_SPEED` (0.8–1.2), `MP_RUNOUT_TIME` (2–12s), `MP_ATTACK` (5–300ms), `MP_RELEASE` (20–1000ms).

### 7.3 ✅ Strengths

- **Tape runout envelope** — genuine M400 characteristic where tape runs out after ~8 seconds of continuous hold.
- **Tape speed control** — slowing/speeding the tape pitch-shifts all voices together.
- **Pitch spread** via LCG per-note — adds the "each key has its own tape strip" detuning character.
- **35-voice capacity** — correct for full M400 keyboard.
- **Simple, RT-safe wavetable read** — linear interpolation, no heap.

### 7.4 ⚠️ Weaknesses

- **No true tape wow/flutter** — real Mellotron tape has ±1–5 cents low-frequency pitch modulation from capstan irregularities. The current implementation has a fixed `tapeSpeed_` with no LFO.
- **Pitch bend is a no-op** (`pitchBend() {}`) — Mellotron doesn't have native pitch bend, but players used it via modulation or a pitch wheel, and the interface advertises it.
- **No per-note tape texture** — all voices of the same tape type play from the same wavetable loop. Real Mellotron: each key had a physically separate tape strip with its own recording quality/noise.
- **Wavetable loop length** — using a single-cycle wavetable loses the evolving texture of a real Mellotron tape (which included room tone, tape hiss, and temporal variation over 8 seconds).
- **No tape noise floor** — Mellotron is famous for its audible tape hiss; even a simple white noise at −40dBFS per active voice would add authenticity.
- **`MP_VOLUME` not applied via `ParamSmoother`** — abrupt volume changes.

### 7.5 🔧 Improvement Proposals

**Me1 — Tape wow/flutter LFO (P1, medium effort)**
Add per-voice independent LFO for phase increment modulation:
```cpp
// In MellotronVoice, add:
float wowPhase_ = 0.0f;    // slow wow: 0.3–1.5 Hz
float flutterPhase_ = 0.0f; // fast flutter: 5–15 Hz
// In tick():
float wow     = 1.0f + 0.003f * std::sin(wowPhase_);     // ±0.3%
float flutter = 1.0f + 0.001f * std::sin(flutterPhase_); // ±0.1%
phase_ += phaseInc_ * wow * flutter;
```
Seed wow/flutter LFO phases randomly per voice (use same LCG as pitchSpread).

**Me2 — Tape hiss (P2, low effort)**
Add engine-level noise floor:
```cpp
// In MellotronEngine::tickSample(), after summing voices:
if (activeVoices > 0)
    outL += noise_.tick() * 0.003f;  // −50dBFS
```

**Me3 — Longer wavetable for temporal evolution (P3, high effort)**
Replace single-cycle tables with 1–2 second recorded loops (from public-domain Mellotron samples), adding temporal character. This requires loading assets at startup (HAL concern, not DSP core).

---

## 8. Engine 6 — DrumEngine

**Files:** `core/engines/drums/drum_engine.{h,cpp}`, `drum_pad_dsp.h`, `drum_pad_sample.h`

### 8.1 Architecture

**Hybrid design:** 16 pads total:
- **Pads 0–7 (DSP synthesis):** `DrumPadDsp` — pure synthesis, 8 hardcoded types
- **Pads 8–15 (sample playback):** `DrumPadSample` — WAV loaded via HAL at startup

**MIDI note mapping:**
```
DSP pads:    Kick(36), Snare(38), HiHatC(42), HiHatO(46),
             Clap(39), TomLow(41), TomMid(43), Rimshot(37)
Sample pads: Notes 44–51
```

### 8.2 DSP Synthesis Models

| Pad | Type | Model |
|-----|------|-------|
| Kick | `DspDrumType::Kick` | Sine oscillator + exponential pitch sweep (f0 → f0×0.1) |
| Snare | `DspDrumType::Snare` | Sine osc + bandpassed noise mix |
| Hi-Hat C | `DspDrumType::HiHatC` | 6 square oscillators + bandpass filter, short decay |
| Hi-Hat O | `DspDrumType::HiHatO` | Same as HiHatC + longer decay |
| Clap | `DspDrumType::Clap` | 4 noise bursts with small timing offsets |
| Tom Lo | `DspDrumType::TomLow` | Sine pitch sweep (lower register) |
| Tom Mid | `DspDrumType::TomMid` | Sine pitch sweep (mid register) |
| Rimshot | `DspDrumType::Rimshot` | Transient click + decaying ring oscillator |

Per-pad parameters: Volume (0–1), Pitch (0–1 → base freq), Decay (0–1 → decay time), Pan (0–1).

### 8.3 ✅ Strengths

- **Hybrid DSP+sample design** — 808-style synthesis for character pads, real samples for texture pads.
- **GM-compatible MIDI note mapping** — standard drum map starting at note 36.
- **One-shot semantics** — `noteOff()` is a no-op (`{}`), correct for drums.
- **Per-pad pan** — stereo kit positioning.
- **`loadSamplePad()`** ownership-transfer interface — HAL loads WAV, transfers `vector<float>` to engine before audio starts. Clean initialization boundary.
- **`beginBlock()` caches all 65 params** into `padCache_[]` — no atomics in hot path.

### 8.4 ⚠️ Weaknesses

**DSP quality:**
- **Kick pitch sweep is hardcoded** in `DrumPadDsp::trigger()` — the start frequency and sweep rate are not exposed as per-pad parameters. User can adjust `pitch` (base frequency range) but not the sweep curve or sweep depth independently.
- **Hi-hat model** uses 6 square oscillators summed without a proper ring modulation network. Real 808 hi-hat: 6 square oscillators cross-modulated (ring mod pairs) then bandpass filtered. The current model produces correct texture but misses the metallic character.
- **Clap model** uses simple noise bursts — adequate, but lacks the frequency-shaping (bandpass around 1–4kHz) that gives the 808 clap its snap.
- **No velocity → envelope time** — velocity only affects amplitude. Real drums: harder hit → faster initial transient (different feel).
- **No voice choke groups** — Open and Closed hi-hat pads do not choke each other. In real drum machines, hi-hat close kills the open hi-hat sound.

**Architecture:**
- **No per-pad reverb/room** — sample pads play completely dry; no convolution or algorithmic reverb on the pads themselves (only available via the global EffectChain).
- **`pitchBend()` is a no-op** — reasonable for drums, but could control global pitch for electronic drum fills.
- **`getActiveVoices()`** counts active DSP pads + sample pads by iterating all 16 pads each call — not cached.

### 8.5 🔧 Improvement Proposals

**Dr1 — Hi-hat choke group (P1, low effort)**
When pad 2 (HiHatC) is triggered, kill pad 3 (HiHatO):
```cpp
// In DrumEngine::triggerPad():
if (padIdx == 2) dspPads_[3].active = false;  // close kills open
if (padIdx == 3) dspPads_[2].active = false;  // open kills close
```

**Dr2 — Velocity → decay scaling (P1, low effort)**
In `DrumPadDsp::trigger()`:
```cpp
// Harder hit → slightly longer decay (more energy imparted)
float velScale = 0.8f + 0.4f * (velocity / 127.0f);
decayRate = std::pow(baseDecay, 1.0f / velScale);
```

**Dr3 — Expose kick sweep params (P1, medium effort)**
Add per-pad `sweepDepth` (0–1 → 0.1–0.9× start freq) and `sweepRate` params. Map to existing per-pad param layout — requires adding 2 more params per DSP pad (breaking change to param count).

**Dr4 — Hi-hat ring modulation (P3, medium effort)**
Replace 6-oscillator sum with 3 ring-modulated pairs:
```cpp
float rm01 = sin(phase_[0]) * sin(phase_[1]);
float rm23 = sin(phase_[2]) * sin(phase_[3]);
float rm45 = sin(phase_[4]) * sin(phase_[5]);
float mix  = (rm01 + rm23 + rm45) / 3.0f;
// Then bandpass filter around 7–12kHz
```

---

## 9. Cross-Engine Analysis

### 9.1 Common Design Patterns (Used Well)

| Pattern | Implementation | All engines? |
|---------|---------------|--------------|
| `beginBlock()` param snapshot | `atomic<float> params_[]` → `float paramCache_[]` | ✅ Yes |
| Pre-allocated voice arrays | `std::array<Voice, MAX_VOICES>` in constructor | ✅ Yes |
| No dynamic allocation in audio path | All buffers at `init()` | ✅ Yes |
| Amplitude normalization | `1/√N` (Moog unison, Rhodes, DrumEngine sums) | Partial |
| IEngine interface compliance | All 6 implement full virtual API | ✅ Yes |
| `std::atomic<float>` param store | No mutex needed for param set/get | ✅ Yes |

### 9.2 RT-Safety Compliance Review

All 6 engines correctly avoid:
- Dynamic allocation in `tickSample()` / `tick()` ✅
- `std::mutex::lock()` in audio path ✅
- `std::string` construction in audio path ✅
- File I/O in audio path ✅

**Gap:** `// [RT-SAFE]` annotations are present on Rhodes voice `tick()` and DX7Operator `tick()`, but missing from Hammond, Mellotron, and Drum pad `tick()` methods. This is a documentation gap, not a correctness issue, but it reduces code auditability.

### 9.3 Consistency Issues

| Issue | Moog | Hammond | Rhodes | DX7 | Mellotron | Drums |
|-------|------|---------|--------|-----|-----------|-------|
| Pitch bend implemented | ✅ | ❌ no-op | ❌ no-op | ❌ no-op | ❌ no-op | N/A |
| ParamSmoother on knobs | ✅ all | ❌ none | ❌ none | ❌ none | ❌ none | ❌ none |
| `[RT-SAFE]` annotations | ✅ | ❌ | Partial | Partial | ❌ | ❌ |
| Voice stealing quality | 3 strategies | N/A | oldest | round-robin ❌ | oldest | N/A |
| Oscilloscope buffer | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Sustain pedal (CC64) | ❌ | N/A | ✅ | ❌ | ❌ | N/A |

### 9.4 Missing Features Comparison

| Feature | Moog | Hammond | Rhodes | DX7 | Mellotron | Drums |
|---------|------|---------|--------|-----|-----------|-------|
| Pitch bend | ✅ ±2st | ❌ | ❌ | ❌ | ❌ | — |
| Vibrato LFO | via ModMatrix | V1–V3 (simplified) | ❌ | ❌ | ❌ | — |
| ParamSmoother | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Sustain pedal | ❌ | — | ✅ | ❌ | ❌ | — |
| Hard sync | ❌ | — | — | — | — | — |
| Voice choke | — | — | — | — | — | ❌ hi-hat |
| Overdrive model | Filter tanh | ❌ | Engine tanh | ❌ | ❌ | ❌ |

---

## 10. Priority Improvement Roadmap

Sorted by Impact/Effort ratio (P1 = highest priority).

| Priority | Impact | Effort | Engine(s) | Description |
|----------|--------|--------|-----------|-------------|
| **P1** | High | Low | Rhodes, DX7, Mellotron, Hammond | Add pitch bend (2-semitone range) to all 4 engines |
| **P1** | High | Low | DX7 | Fix voice stealing: round-robin → oldest-active |
| **P1** | High | Low | Hammond | Add drawbar `ParamSmoother` (5ms) — eliminate zipper noise |
| **P1** | High | Low | Drums | Hi-hat choke group (open kills closed and vice versa) |
| **P1** | Medium | Low | Hammond, Rhodes, DX7, Mellotron | Add `// [RT-SAFE]` annotations to all `tick()` methods |
| **P1** | Medium | Low | Drums | Velocity → decay scaling for DSP pads |
| **P2** | High | Medium | Hammond | True Leslie rotary cabinet (delay-based dual-rotor) |
| **P2** | High | Medium | DX7 | Exponential ADSR rates (4R4L approximation) |
| **P2** | Medium | Medium | DX7 | Key-rate scaling per operator |
| **P2** | Medium | Medium | DX7 | Fixed-frequency operators (Hz mode) |
| **P2** | Medium | Medium | Mellotron | Tape wow/flutter LFO per voice |
| **P2** | Medium | Low | Mellotron | Tape hiss noise floor |
| **P2** | Medium | Low | Rhodes | `ParamSmoother` on Tone/Decay/Drive |
| **P2** | Medium | Medium | Rhodes | Vibrato LFO (RP_VIBRATO_RATE / DEPTH) |
| **P2** | Medium | Low | Moog | Output limiter after VCA (tanh soft clip) |
| **P3** | Low | Medium | Drums | Hi-hat ring modulation (3 ring-mod pairs + bandpass) |
| **P3** | Low | Medium | Drums | Kick sweep param exposure (sweep depth + rate) |
| **P3** | Low | High | Hammond | Tube overdrive / preamp saturation |
| **P3** | Low | Low | Moog | Analog-style unison detune (LCG offset) |
| **P3** | Low | Low | Moog | Note priority modes for mono (last/low/high note) |
| **P3** | Low | High | Mellotron | Longer wavetable loops (1–2s recorded from samples) |

---

## 11. Performance Analysis

### 11.1 Per-Sample CPU Cost Estimate (relative units)

The following estimates assume a single active voice. "Cost" is a relative unit based on operation count and cache behavior.

| Engine | Voice Cost/Sample | Max Voices | Worst-Case Total |
|--------|------------------|------------|-----------------|
| **Moog** | ~35 flops (3 VCOs + filter stages + env) | 8 | ~280 flops |
| **Hammond** | ~18 flops (9 sine lookups + perc + vibrato) | 61 | ~1100 flops |
| **Rhodes** | ~25 flops (4 modal + torsion + pickup + env) | 8 | ~200 flops |
| **DX7** | ~40 flops (6 ops × 6 mults + algorithm routing) | 8 | ~320 flops |
| **Mellotron** | ~5 flops (table lookup + linear interp + env) | 35 | ~175 flops |
| **Drums** | ~10 flops DSP / ~3 flops sample | 16 | ~120 flops |

**Notes:**
- Hammond with all 61 keys held is the most CPU-intensive scenario (~1100 relative units).
- All costs are well within a modern CPU budget at 44100 Hz. On Teensy 4.1 (V3 target), Hammond at full polyphony may need a `NEON`-equivalent optimization (unrolled SIMD for the 9-oscillator sum).
- `ParamSmoother` adds ~1 flop/param/sample — the estimated cost for adding it to all non-Moog engines is negligible.

### 11.2 Memory Footprint

| Engine | Voice struct size | Array size |
|--------|-----------------|------------|
| Moog Voice | ~400 bytes (3 oscs + filter + 2 envs + glide + smoother) | 8 × 400B ≈ 3.2 KB |
| Hammond Voice | ~80 bytes (9 phases + perc + noise) | 61 × 80B ≈ 4.9 KB |
| Rhodes Voice | ~160 bytes (5 modal modes + noise + state) | 8 × 160B ≈ 1.3 KB |
| DX7 Voice | ~320 bytes (6 operators × 48B + state) | 8 × 320B ≈ 2.6 KB |
| Mellotron Voice | ~48 bytes (phase + env + state) | 35 × 48B ≈ 1.7 KB |
| Drum DSP Pad | ~64 bytes (2 phases + env + noise) | 8 × 64B ≈ 0.5 KB |
| Drum Sample Pad | ~24 bytes + sample data heap | 8 pads + WAV data |

All DSP core engine state (excluding sample data) fits comfortably within 32 KB — well within Teensy 4.1 DTCM memory.

---

*Document generated from full codebase analysis of `core/engines/` — all 6 engine implementations reviewed.*
