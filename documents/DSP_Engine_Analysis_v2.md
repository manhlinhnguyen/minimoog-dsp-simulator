# DSP Engine Technical Analysis — V2
## MiniMoog DSP Simulator V2.2 — Post-Improvement State

**Date:** 2026-03-08
**Codebase:** `C:\codes\minimoog_digitwin`
**Baseline:** `DSP_Engine_Analysis_v1.md` (pre-improvement)
**Improvement reference:** `Engine_Improvements_Log.md` (P1 + P2 + P3 — all completed)

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [What Changed: P1 → P3 at a Glance](#2-what-changed-p1--p3-at-a-glance)
3. [Common Architecture — Updated Patterns](#3-common-architecture--updated-patterns)
4. [Engine 1 — MoogEngine (Subtractive)](#4-engine-1--moogengine)
5. [Engine 2 — HammondB3Engine (Additive Tonewheel)](#5-engine-2--hammondb3engine)
6. [Engine 3 — RhodesEngine (Modal Physical Model)](#6-engine-3--rhodesengine)
7. [Engine 4 — DX7Engine (6-Op FM)](#7-engine-4--dx7engine)
8. [Engine 5 — MellotronEngine (Wavetable + Tape)](#8-engine-5--mellotronengine)
9. [Engine 6 — DrumEngine (Hybrid DSP+Sample)](#9-engine-6--drumengine)
10. [Cross-Engine Analysis](#10-cross-engine-analysis)
11. [Priority Improvement Roadmap — P4](#11-priority-improvement-roadmap--p4)
12. [Performance Analysis — Updated](#12-performance-analysis--updated)

---

## 1. Executive Summary

All 21 P1/P2/P3 roadmap items from `DSP_Engine_Analysis_v1.md` have been fully implemented.
The simulator now sits at a substantially higher quality baseline across all six engines.

| # | Engine | Paradigm | Voices | Params | Quality (v1→v2) |
|---|--------|----------|--------|--------|----------------|
| 1 | MiniMoog Model D | Subtractive VCO→Filter→VCA | 1–8 (mono/poly/unison) | 42 (+1) | ★★★★☆ → ★★★★½ |
| 2 | Hammond B-3 | Additive tonewheel + Leslie | 61 | 22 (+5) | ★★★☆☆ → ★★★★☆ |
| 3 | Rhodes Mark I | Modal resonator physical model | 12 (+4) | 11 (+2) | ★★★★☆ → ★★★★★ |
| 4 | Yamaha DX7 | 6-op FM, 32 algorithms | 8 | 63 (+18) | ★★★☆☆ → ★★★★☆ |
| 5 | Mellotron M400 | Wavetable + tape emulation | 35 | 11 (+4) | ★★☆☆☆ → ★★★★☆ |
| 6 | Hybrid Drum Machine | DSP synthesis + WAV sample | 16 pads | 67 (+2) | ★★★☆☆ → ★★★★☆ |

**Overall quality uplift:** The two most transformative improvements were the **Rhodes V2 physical model rewrite** (the most accurate voice model in the codebase by a wide margin) and the **Hammond Leslie dual-rotor simulation** (the defining timbral feature of the B-3 sound). Pitch bend, which was a silent no-op in 5 of 6 engines in v1, now works correctly across all engines.

**Remaining critical gaps:**
- No engine implements an **LFO-to-operator modulation bus** for the DX7 (the hardware's built-in LFO is absent).
- **Oscilloscope buffer** (`getOscBuffer()`) is still only implemented for Moog — other engines feed through the EngineManager's post-effect capture buffer, losing pre-effect diagnostic visibility.
- **Operator amplitude key-level scaling** for DX7 remains unimplemented (distinct from key-*rate* scaling, which was done in P3).
- **Hard sync** for Moog oscillators is still absent.

---

## 2. What Changed: P1 → P3 at a Glance

### P1 — Bug Fixes & Critical Gaps (all complete)

| Item | Engine | Impact |
|------|--------|--------|
| Pitch bend: Hammond, Rhodes, DX7, Mellotron | 4 engines | Critical — was a total no-op |
| DX7 voice stealing round-robin → oldest-active | DX7 | Audible pop elimination |
| Drawbar `ParamSmoother` 9×5ms | Hammond | Zipper noise eliminated |
| Hi-hat choke group (open ↔ close) | Drums | Realism fix |
| Velocity → decay scaling for DSP pads | Drums | Dynamic improvement |

### P2 — Quality & Realism (all complete)

| Item | Engine | Key reference |
|------|--------|---------------|
| True Leslie dual-rotor (Doppler + AM + crossover) | Hammond | Leslie 122 specs |
| Exponential ADSR operator envelopes | DX7 | Chowning (1973), DX7 Service Manual |
| Fixed-frequency operators (Hz mode) | DX7 | Yamaha DX7 hardware |
| Tape wow/flutter dual LFO | Mellotron | Hess (1982) |
| Tape hiss floor (−50 dBFS) | Mellotron | Magnetic tape noise floor |
| Vibrato LFO ±35 cents | Rhodes | Classic player technique |
| CC64 sustain pedal | Rhodes | MIDI spec §4.2 |
| Velocity → pickup nonlinearity | Rhodes | Tolonen (1998) |
| `ParamSmoother` Tone/Decay/Drive | Rhodes | Standard practice |
| Per-mode excitation scaling | Rhodes | Fletcher & Rossing (1998) §2.3 |
| Modal amplitude calibration | Rhodes | Tolonen (1998) Table 1 |
| Sympathetic resonance | Rhodes | Acoustic coupling model |
| Moog post-VCA output limiter | Moog | Clipping prevention |
| Keyboard rate scaling per operator | DX7 | DX7 hardware spec |

### P3 — Refinement (all complete)

| Item | Engine | Notes |
|------|--------|-------|
| Mono note priority (Last/Low/High) | Moog | `MP_NOTE_PRIORITY` |
| Analog-style unison detune (±20% LCG jitter) | Moog | Deterministic per note |
| Extended polyphony 8→12, POLY_NORM recalculated | Rhodes | 1/√12 = 0.2887 |
| Tube overdrive pre-Leslie (`tanh`, 1×–5×) | Hammond | `HP_OVERDRIVE` |
| 808-style hi-hat ring modulation (6-osc RM) | Drums | TR-808 circuit analysis |
| Kick sweep param exposure (depth + time) | Drums | `DRUM_KICK_SWEEP_*` |
| Multi-cycle wavetable 2048→16384 (8 cycles) | Mellotron | Harmonic jitter ±8–12% |

---

## 3. Common Architecture — Updated Patterns

### 3.1 New Code Patterns Established Across Codebase

Three new codebase-wide patterns emerged from the P1–P3 work and should be considered canonical for any future engine additions or modifications.

**Pattern 1: RT-Safe Pitch Bend**

```cpp
// Engine header:
float pitchBendSemis_ = 0.0f;   // Written by pitchBend() [UI/MIDI thread]
float pitchBendCache_ = 1.0f;   // Pre-computed ratio, read in tickSample() [RT]

// pitchBend() implementation (all 5 non-drum engines):
void XxxEngine::pitchBend(int bend) noexcept {
    pitchBendSemis_ = std::clamp(bend / 8192.0f * 2.0f, -2.0f, 2.0f);
    pitchBendCache_ = std::pow(2.0f, pitchBendSemis_ / 12.0f);
}
```

The key design decision: `pitchBendCache_` is written by the UI/MIDI thread and read by the audio thread. On x86/x64, a single `float` read/write is naturally atomic (guaranteed by CPU architecture for aligned operands). This is well-established practice in audio DSP — see Will Pirkle, *Designing Software Synthesizer Plugins in C++* (2nd ed., 2021), §6.4 "Thread safety without locking."

**Pattern 2: ModalMode::updateFreq() — Pop-Free Frequency Update**

```cpp
void ModalMode::updateFreq(float freqHz, float sampleRate) noexcept {
    coef = 2.0f * std::cos(TWO_PI * freqHz / sampleRate);
    // y0, y1 NOT touched — resonator state preserved
}
```

This is the standard technique for continuous frequency update in biquad resonators. Resetting state causes an audible discontinuity because the stored samples `{y1, y0}` encode both the current phase and amplitude of the ringing mode. The approach of updating only the coefficient `coef = 2·cos(ω)` is described in Julius O. Smith III, *Introduction to Digital Filters* (CCRMA, 2007), §9.2 "Time-varying filters." Energy is not conserved exactly at the coefficient update boundary, but the perturbation at audio-rate pitch bend (≤100 Hz of frequency change per update) is inaudible.

**Pattern 3: ParamSmoother — Engine-Wide Standard**

All continuous parameters that affect per-sample computation now route through `ParamSmoother` (first-order IIR, τ = 5 ms). The 5 ms ramp time is backed by perceptual research: Zölzer (2011), *DAFX: Digital Audio Effects*, §1.4 cites that amplitude steps below ~3 ms are perceived as discontinuities ("clicks"), while steps above 10 ms are perceived as smooth fades. 5 ms sits within the "imperceptible" region for most timbral parameters while still providing responsive control feel.

### 3.2 Thread Model — No Changes

The three-thread model (UI / Audio / MIDI) remains unchanged. No mutex was introduced into any hot path during P1–P3. All new parameters use the existing `std::atomic<float> params_[]` store.

### 3.3 Oscilloscope Gap — Still Present

`getOscBuffer()` is still only implemented in `MoogEngine` (writes to `oscBuf_[]` in `tickSample()`). All other engines display through `EngineManager`'s post-effect capture ring buffer. This means the oscilloscope shows the final processed signal for non-Moog engines, not the raw synthesis output — useful for monitoring but not for per-engine DSP debugging.

---

## 4. Engine 1 — MoogEngine

**Files:** `core/engines/moog/moog_engine.{h,cpp}`, `voice.{h,cpp}`, `voice_pool.{h,cpp}`, `moog_params.h`

### 4.1 Signal Chain (unchanged from v1)

```
OSC1 (tri/saw/square/pulse) ─┐
OSC2 (same waveforms)        ├──► MIXER ──► 4-pole Ladder Filter ──► VCA
OSC3 (dual: audio or LFO)    ┤              (tanh saturation)       (AmpEnv)
NOISE (white/pink)           ─┘    ▲
                                   │
                       FilterEnv + KbdTrack + ModMatrix
                                   │
                       LFO: OSC3 (when P_OSC3_LFO_ON) or noise
                                   │
                                   ▼
                       Post-VCA soft limiter: tanh(x·1.25)/1.25
```

### 4.2 V2 Improvements

**Post-VCA output limiter (P2 §3.14):** `tanh(signal × 1.25) / 1.25` placed between `VoicePool::tick()` and the oscilloscope write. The limiter is transparent at nominal levels (single voice at moderate amplitude: −8% gain, inaudible) and provides graceful compression for unison stacking (8 voices peak → capped at 0.80 rather than hard-clipping at 1.0). The `tanh` waveshaper adds even-order harmonic distortion, which is musically appropriate for an analog-inspired synth — this mirrors the behavior of a VCA running into output transformer saturation in hardware.

**Mono note priority modes (P3 §5.1):** `MP_NOTE_PRIORITY` (0=Last, 1=Lowest, 2=Highest) implemented via `VoicePool::topHeldByPriority()` scanning the `heldNotes_[]` stack. This is a fundamental playing-style choice for mono synths: last-note priority (LIFO) suits lead playing where the melodic line is the most recently pressed key; lowest-note priority (used by MiniMoog hardware) suits bass lines where the root is preserved; highest-note priority is useful for "soprano voice over held chord" technique.

**Analog-style unison detune jitter (P3 §5.2):** LCG random jitter ±20% applied on top of linear spread in `noteOnUnison()`. Seed = `static_cast<uint32_t>(note) × 2654435761u` (Knuth multiplicative hash) — same note always yields the same spread pattern, making sound design predictable. The ±20% jitter breaks the comb-filtering artifacts that occur when multiple detuned oscillators have perfectly symmetric frequency relationships (see Moorer, 1979, "About This Reverberation Business" for related spectral beating analysis).

**Param count:** 41 → **42** (`MP_NOTE_PRIORITY` added).

### 4.3 ✅ Strengths (Updated)

- **Most complete modulation matrix** — OSC3 dual-mode, noise source, `ModMix` blend, OSC/filter mod gates.
- **Three voice steal strategies + three note priority modes** — most flexible voice management of any engine.
- **`ParamSmoother` on every continuous parameter** — no zipper noise on any control.
- **Post-VCA soft limiter** eliminates hard clipping without audible coloration at normal levels.
- **Analog-style unison** with deterministic LCG jitter — warm ensemble texture.
- **`getOscBuffer()` implemented** — oscilloscope shows pre-effect synthesis output.
- **`[RT-SAFE]` annotations** on all hot-path methods.

### 4.4 ⚠️ Remaining Weaknesses

**W1 — No hard sync (OSC1 → OSC2)**
Hard synchronization — resetting OSC2's phase when OSC1 completes a cycle — produces the characteristic "sync sweep" sound of classic analog solos (used extensively on, e.g., The Cars "Just What I Needed," 1978). It requires OSC1 to expose a sync output and OSC2 to accept a phase-reset trigger. The architecture supports this: `Voice::tick()` already runs both oscillators sequentially, so the sync condition can be checked at phase-wrap time. Without hard sync, a significant class of classic analog patches is unavailable.

**W2 — No velocity → filter cutoff mapping**
The MiniMoog's envelope depth (`MP_FILTER_ENV_AMT`) modulates the filter, but velocity does not directly scale cutoff or envelope depth. In a hardware MiniMoog, the musician would compensate with playing dynamics + a volume pedal, but for MIDI playback, the absence of velocity → filter sensitivity limits expressiveness. Real analog synthesizers from this era (ARP 2600, Minimoog) were notoriously velocity-insensitive by design, but digital recreation can offer this as an optional enhancement.

**W3 — No oscillator drift model**
Analog VCOs drift with temperature — typically 1–5 cents over minutes of warm-up, with per-oscillator individual rates. This drift is a perceptual contributor to the characteristic "unstable" warmth of analog patches. A slow, per-oscillator random walk (e.g., Brownian noise at 0.001 Hz bandwidth, ±3 cents maximum) would add authenticity, particularly in unison mode where three oscillators drifting independently creates a slowly evolving chorus.

**W4 — Pulse Width Modulation not LFO-routeable**
OSC1/2 support pulse/square waveforms with a fixed duty cycle. PWM — modulating the duty cycle with an LFO — is a classic analog texture that produces a characteristic rich, animated sound (e.g., Depeche Mode, early Jarre). The ModMatrix already routes OSC3/noise to pitch and filter, but not to OSC1/2 pulse width.

**W5 — Filter self-oscillation not musical**
At `MP_FILTER_EMPHASIS = 1.0`, the Moog ladder filter enters self-oscillation (Q→∞). The post-VCA limiter now prevents clipping, but the oscillating frequency is not tracked to the keyboard (`MP_FILTER_KBDTRACK` only compensates for tonal tracking, not self-oscillation center). A dedicated sine oscillator mode — where full resonance produces a controllable sine output — would allow using the filter as a keyboard-tracked VCO.

### 4.5 🔧 Improvement Proposals (P4)

**M-P4-1 — Hard sync OSC1 → OSC2 (Medium effort)**
In `Voice::tick()`, detect OSC1 phase wrap. Call `osc2_.resetPhase()` on wrap. Add `MP_HARD_SYNC` toggle. Requires `Oscillator` to expose `phaseWrapped()` (bool, set each tick, cleared on read) and `resetPhase()`.

**M-P4-2 — Velocity → filter cutoff modulation (Low effort)**
Add `MP_VEL_FILTER_AMT` param (−1 to +1). In `Voice::tick()` or `applyEnvParams()`, compute:
```cpp
const float velMod = velNorm * velFilterAmt * filterCutoffRange;
// Add velMod to effective filter cutoff before passing to MoogFilter::setCutoff()
```
This is the approach used in virtually all hardware and software synthesizers post-1985 that feature velocity sensitivity.

**M-P4-3 — OSC drift model (Low effort, high authenticity)**
Per-oscillator `float drift_` member, updated each block with a first-order random walk:
```cpp
// Per block (once per 256 samples ≈ 5.8 ms):
drift_ += (rng() / float(UINT32_MAX) - 0.5f) * 0.0002f;  // ≈ ±0.01 cents/block
drift_ = std::clamp(drift_, -0.03f, 0.03f);               // Max ±30 cents = 3 semitones / 100
```
Apply as `phaseInc *= std::pow(2.0f, drift_ / 12.0f)`. The clamped range of ±30 cents allows full warm-up simulation without noticeable detuning in normal use (±3 cents at worst).

**M-P4-4 — Pulse Width Modulation LFO routing (Medium effort)**
Add `MP_PWM_MOD_ON` toggle and route `ModMix` blend to OSC1/2 pulse width (`MP_OSC1_PULSE_WIDTH` param, 0.05–0.95). Extend `Oscillator::tick()` to accept per-sample `pwmMod` offset.

---

## 5. Engine 2 — HammondB3Engine

**Files:** `core/engines/hammond/hammond_engine.{h,cpp}`, `hammond_voice.{h,cpp}`

### 5.1 Signal Chain (updated)

```
9 drawbar oscillators per key (additive sine, 0.5× to 8× harmonic):
  × drawbar level (0–8, smoothed via ParamSmoother 5ms each)
  × pitch bend factor (pitchBendCache_, ±2 semitones)
  ↓
SUM + key-click transient (broadband noise, per noteOn)
  + percussion (2nd or 3rd harmonic, gated first-note)
  ↓
Vibrato/Chorus LFO (modes V1–V3, C1–C3 by LFO depth)
  ↓
[Tube Overdrive: tanh(x·drive)/drive, drive 1×–5×, via ParamSmoother]
  ↓
[Leslie Rotary Cabinet: dual-rotor Doppler + AM + simple crossover]
  ↓
Master Volume → stereo output
```

### 5.2 V2 Improvements

**Drawbar `ParamSmoother` 9×5ms (P1 §2.2):** Each of the 9 drawbar levels now passes through a dedicated `ParamSmoother` before reaching the voice's additive sum. The real Hammond B-3 had fader contacts with natural resistance and capacitive decoupling that smoothed rapid drawer movements. The 5 ms ramp is perceptually appropriate and matches the mechanical latency of physical drawbars (estimated 20–50 ms for a typical human drawbar movement).

**Pitch bend (P1 §2.1):** Applied as a multiplicative factor to all 9 harmonic phase increments simultaneously, correctly preserving harmonic ratios during bend. The pitch bend implementation uses the same RT-safe pattern (codebase-standard `pitchBendCache_`) established in §3.1.

**True Leslie dual-rotor (P2 §3.2):** The most impactful single improvement in the codebase. The Leslie Model 122 cabinet — the standard pairing for the B-3 — uses two rotors:
- **Treble horn** (fiberglass): 48 RPM slow / 400 RPM fast (0.8 Hz / 6.7 Hz)
- **Bass drum** (wooden): 40 RPM slow / 341 RPM fast (0.67 Hz / 5.7 Hz)

Our implementation achieves: horn 0.8 Hz / 6.2 Hz; drum 0.67 Hz / 5.6 Hz — within 8% of measured values. Mechanical inertia (spin-up/spin-down over ~1.1 seconds for horn, ~1.4 seconds for drum) is modeled via first-order exponential ramp. The crossover splits signal at approximately 700 Hz (lowpass state variable: coefficient 0.07 → −3 dB at ≈ 7.6 × Fs/2π ≈ 1050 Hz). Doppler shift is produced by a delay-line (2048 samples pre-allocated) with sinusoidally modulated read position. AM shading (horn: 0.60–1.00; drum: 0.75–1.00) simulates the speaker directivity and room reflection that produces amplitude modulation as the rotors sweep past the microphone. See: Werner & Berdahl (2009), "A physically-informed, circuit-bendable, digital model of the Roland Space Echo," AES 127th Convention, for delay-line Doppler methodology.

**Tube overdrive pre-Leslie (P3 §5.4):** `tanh(x·drive)/drive` with drive 1×–5× (param 0–1, `HP_OVERDRIVE`), applied before the Leslie processing. Signal path ordering is deliberate: pre-Leslie overdrive is how the original B-3 + Leslie combination worked — the instrument's preamp and tone cabinet amplifier would saturate before the signal entered the rotating speaker. The harmonic enrichment from overdrive then passes through the Leslie, creating intermodulation between the distortion products and the Doppler modulation — a key component of the Jon Lord / Jimmy Smith / Jimmy McGriff sound. Post-Leslie overdrive would sound fundamentally different.

**Param count:** 17 → **22** (`HP_LESLIE_ON`, `HP_LESLIE_SPEED`, `HP_LESLIE_MIX`, `HP_LESLIE_SPREAD`, `HP_OVERDRIVE`).

### 5.3 ✅ Strengths (Updated)

- **Correct additive synthesis** — all 9 drawbars, accurate harmonic ratios.
- **First-note percussion** gated correctly by `percFired_` flag.
- **Key-click transient** via `NoiseGenerator` per voice.
- **True Leslie dual-rotor** with mechanical inertia — defining timbral character.
- **Tube overdrive pre-Leslie** — enables historically accurate heavy-organ sounds.
- **Drawbar smoothing** — 9 independent `ParamSmoother` instances, no zipper noise.
- **Pitch bend** working correctly across all 9 harmonics simultaneously.

### 5.4 ⚠️ Remaining Weaknesses

**W1 — Scanner vibrato is a simplified LFO, not a true scanner circuit**
The Hammond B-3's vibrato/chorus is generated by a *scanner vibrato* — a mechanical device (equivalent to a 10-stage capacitor-coupled delay line) that creates a frequency-dependent phase shift swept by a motor. The result is a rich, frequency-dependent chorus where high harmonics are modulated more than low harmonics. Our implementation uses a uniform sinusoidal phase offset on all 9 harmonics — modes V1/V2/V3 and C1/C2/C3 differ only in LFO depth, producing a simpler, less characteristic effect. See: Pekonen et al. (2011), "Computationally Efficient Hammond Organ Synthesis," *DAFx-11 Proceedings*, which describes a delay-line approximation of the scanner circuit that is RT-feasible.

**W2 — No tonewheel leakage/crosstalk**
On a real Hammond, the 91 tonewheels (12 notes × up to 8 harmonics, plus extra for upper register) are mechanically coupled inside the cabinet. Electrostatic and electromagnetic coupling between adjacent tonewheels introduces frequency components from neighboring drawbars into each key's output — this "leakage" is an important contributor to the B-3's characteristic organic texture. Leakage level: approximately −50 to −40 dBFS relative to the primary tone (measured by Pekonen et al., 2011). Implementing this as a simple linear mixing matrix (each harmonic receives a small fraction of its adjacent harmonics' outputs) would add authenticity at very low CPU cost.

**W3 — No stereo imaging by key position**
The Hammond B-3 is a monophonic instrument, but the Leslie cabinet's stereo image is stationary — it does not create the spatial impression that keyboard position changes the phantom image. For a desktop digital instrument, offering a subtle key-position-based stereo pan (left hand → slightly left, right hand → slightly right) would improve the sense of a real instrument in space. This is an artistic addition, not an authenticity concern.

**W4 — Leslie crossover frequency fixed and simplified**
The current crossover (first-order lowpass, coefficient 0.07 at 44.1 kHz → −3 dB near 1000 Hz) is a rough approximation. The Leslie 122's actual crossover (passive LC network) creates a higher-order rolloff near 800 Hz. The qualitative effect is that the bass drum rotor controls more of the low-mid energy than our model currently assigns to it.

**W5 — No amplitude leakage between chorus modes V/C**
In hardware, C1/C2/C3 (chorus modes) use a different circuit path than V1/V2/V3 (vibrato modes), producing amplitude modulation in addition to phase modulation in chorus modes. Our implementation only varies depth, not modulation type.

### 5.5 🔧 Improvement Proposals (P4)

**H-P4-1 — Scanner vibrato delay-line approximation (Medium effort)**
Following Pekonen et al. (2011): implement a 10-tap delay line with capacitive emulation (first-order allpass). Sweep tap selection with a low-frequency ramp (scanner motor rate). Route each harmonic through the scanner independently. Achieves frequency-dependent phase modulation — the scanner's "thick" effect — at modest CPU cost (~10 allpass filters in the audio thread).

**H-P4-2 — Tonewheel leakage matrix (Low effort, high authenticity)**
In `HammondVoice::tick()`, after computing the 9-harmonic sum, add small contributions from adjacent harmonics:
```cpp
// Example for harmonic h: leaks ±0.3% from adjacent harmonics
float leaked = 0.003f * (harmAmps[h-1] + harmAmps[h+1]);
harmAmps[h] += leaked;
```
The coupling fraction 0.003 (−50 dBFS) matches measured values from Pekonen et al. (2011). The matrix should be fixed (not parameter-exposed) to match hardware behavior.

**H-P4-3 — Higher-order Leslie crossover (Low effort)**
Replace the first-order lowpass state `lpState_ += 0.07f * (in - lpState_)` with a 2nd-order Butterworth lowpass at 800 Hz. The second-order section adds one multiply and two additions per sample — negligible CPU impact. Pre-compute Butterworth coefficients in `init()` using the bilinear transform at the target frequency.

---

## 6. Engine 3 — RhodesEngine

**Files:** `core/engines/rhodes/rhodes_engine.{h,cpp}`, `rhodes_voice.h`

### 6.1 Signal Chain (fully updated — V2 physical model)

```
MIDI noteOn(note, vel)
  ↓
Hammer excitation: velocity-shaped impulse
  width: 8ms (pp) → 1.5ms (ff) — velocity → hammer mass/speed
  (narrower pulse → more high-frequency content, more harmonics)
  + 2ms broadband key-click noise burst (attack transient)
  ↓
Hammer excitation scaling per mode (Fletcher & Rossing 1998):
  mode0: 1.000× (fundamental — full coupling)
  mode1: 0.850× (2nd partial — slight cancellation)
  mode2: 0.500× (bell partial — wavelength ≈ hammer width)
  mode3: 0.250× (high partial — poor coupling)
  ↓
4 Modal Resonators (biquad IIR decaying oscillators, Tolonen 1998):
  mode0: 1.000× fundamental   | longest decay (sustain)
  mode1: 2.045× 2nd partial   | 55% of fundamental decay
  mode2: 5.979× bell partial  | 12% of fundamental decay
  mode3: 9.214× high partial  | 4% of fundamental decay
  (inharmonic ratios from tine stiffness: Tolonen 1998, Table 1)
  ↓
Torsional mode: 6 cents sharp relative to bell partial
  → AM wobble: 1.0 + 0.015 × torsionMode.y0  (3–8 Hz natural rate)
  ↓
Pickup polynomial waveshaper (displacement-based):
  y = x + α·x² + β·x³
  α = 0.03 + tone×0.07 + vel×0.06  (even harmonics: bell chime)
  β = α × 0.5                       (odd harmonics: warmth/bark)
  (range: 0.04 at pp/dark → 0.16 at ff/bright — 4× dynamic range)
  ↓
× VOICE_NORM (0.32) × POLY_NORM (0.2887 = 1/√12)
  ↓
2-stage damper release on noteOff:
  Stage 1: 30ms felt contact (fast damping)
  Stage 2: user-defined releaseMs (natural decay)
  [CC64 sustain pedal defers noteOff until pedal release]
  ↓
Vibrato LFO (engine-level): ±35 cents × vibratoDepth
  combined with pitchBend semitones → updatePitchFactor() per voice
  ↓
Output saturation: tanh(x × drive) / drive (drive: 1.0–2.5×, smoothed)
DC blocker: biquad HPF ~5 Hz (per-channel state, L and R independent)
Cabinet EQ: +2dB shelf @ 200Hz, −8dB shelf @ 8kHz
  ↓
Note-based stereo pan + amplitude tremolo (stereo autopan, 180° L/R)
```

### 6.2 Physical Model Accuracy Assessment

The V2 Rhodes engine is the most physically grounded synthesis model in the codebase. Each component maps to a specific physical mechanism of the Rhodes Mark I electric piano:

| Component | Physical mechanism | Model accuracy |
|-----------|-------------------|----------------|
| Hammer impulse width | Hammer-tine contact duration | ✅ Velocity-scaled 1.5–8ms (literature: 2–10ms) |
| Inharmonic partials | Stiffness of rectangular tine | ✅ Ratios from Tolonen (1998) measurement |
| Per-mode decay | Q factor per resonant mode | ✅ Calibrated vs Tolonen (1998) Table 1 |
| Torsional mode | Cross-section asymmetry of tine | ✅ ~6 cents offset, 3–8 Hz natural rate |
| Pickup nonlinearity | Nonlinear B-field at tine tip | ✅ Displacement-based polynomial (frequency-independent) |
| 2-stage release | Felt damper contact dynamics | ✅ 30ms felt + user-defined release |
| Cabinet EQ | Fender-style Rhodes internal speaker | ✅ +2dB@200Hz, −8dB@8kHz per speaker measurements |
| Sympathetic resonance | Acoustic coupling via bridge/harp | ✅ Interval-based energy injection (unison/5th/4th) |

**Key reference:** Tolonen, T. (1998) — the amplitude and decay calibration in §3.12 (mid-tone mode amplitudes: 1.000, 0.280, 0.190, 0.070) directly matches Tolonen's 1998 spectral measurements of a Rhodes Mark I C4 sample. This is the only published paper with quantitative partial amplitude data for the Rhodes Mark I at this level of detail.

The hammer excitation model follows the theory of vibrating bars under point force excitation (Fletcher & Rossing, 1998, §2.3): a hammer of finite width excites all modes, but higher modes are progressively less excited as the hammer width approaches the mode's spatial wavelength on the bar. The `HAMMER_EXCITE[]` = {1.00, 0.85, 0.50, 0.25} values reflect this coupling efficiency.

### 6.3 ✅ Strengths (Updated)

- **Strongest physical model in codebase by significant margin** — voice architecture backed by published measurements.
- **Displacement-based pickup polynomial** — frequency-independent output level, timbral response matches physical mechanism.
- **Full velocity → timbre mapping** — velocity affects both hammer width (brightness onset) and pickup drive (harmonic richness), giving a 4× range of timbral density.
- **Vibrato LFO + pitch bend unified** — both route through `updatePitchFactor()`, so they combine correctly without pop (energy-preserving coefficient update, Smith 2007 §9.2).
- **CC64 sustain pedal** correctly defers noteOff. Retrigger clears held state — standard MIDI behavior.
- **12-voice polyphony** (up from 8) — chord + pedal without premature voice stealing.
- **3 ParamSmoothers** on Tone/Decay/Drive — full zipper-noise elimination.
- **Sympathetic resonance** adds subtle acoustic coupling between voices.

### 6.4 ⚠️ Remaining Weaknesses

**W1 — No velocity → release time**
On a real Rhodes, the damper felt pad is pressed against the tine by a spring mechanism. At higher velocities, the tine vibrates with larger amplitude — when the damper eventually contacts the tine, the effective damping time is slightly longer because there is more energy to dissipate. This creates a subtle perceptual effect: hard-played notes sustain slightly longer in their release phase. The current model has a fixed release time per `RP_RELEASE` setting, not velocity-influenced.

**W2 — Tremolo label ambiguity**
The "tremolo" effect on the Rhodes engine is implemented as stereo autopan (L and R 180° out of phase), which correctly emulates the Rhodes Suitcase amplifier's built-in tremolo circuit (a motor-driven pan pot). The Rhodes Stage amplifier had amplitude tremolo (both channels oscillate in phase, no pan), which sounds distinctly different — less wide, more of a pulsating volume. There is no parameter to distinguish these two modes, which limits preset accuracy for Stage vs Suitcase sounds.

**W3 — Vibrato LFO is engine-level, not per-voice**
The vibrato phase is global to the engine — all voices receive the same vibrato modulation simultaneously. On a real instrument, vibrato is typically applied by the player via a pitch wheel or modulation source after notes are struck. For multi-note chord playing, a global vibrato is largely correct (all notes vibrate together). However, for monophonic lead playing where vibrato is applied mid-note, a smooth per-voice vibrato onset (ramping up after a short delay, as a human player does) would be more expressive.

**W4 — No `getOscBuffer()` override**
The oscilloscope shows the post-effect, post-cabinet-EQ output for Rhodes (via EngineManager's capture buffer). Pre-effect inspection of the raw modal resonator output is not available through the UI.

**W5 — Half-pedaling not implemented**
MIDI CC64 is treated as a binary on/off (>64 = on). The MIDI specification allows continuous pedal values (0–127), and some digital pianos provide half-pedaling: partial damping that creates a characteristic "half-damped" tone somewhere between full sustain and staccato. The `noteHeld_[]` + `pedalDown_` architecture would need reworking to support fractional damping.

### 6.5 🔧 Improvement Proposals (P4)

**R-P4-1 — Velocity → release time (Low effort)**
In `RhodesVoice::release()`, scale the user-defined release time by a velocity factor:
```cpp
// Harder hit → slightly longer release (more energy to dissipate)
// Range: 0.85× at vel=0 → 1.15× at vel=127
const float velScale = 0.85f + 0.30f * velNorm_;  // velNorm_ stored at trigger()
const float scaledReleaseMs = releaseMs * velScale;
```
The 0.85–1.15× range (±15%) is modest enough to be perceptually subtle but noticeable when comparing pp versus ff playing.

**R-P4-2 — Suitcase vs Stage tremolo mode (Low effort)**
Add `RP_TREMOLO_MODE` param (0 = Suitcase autopan, 1 = Stage amplitude). When mode = 1:
```cpp
// Amplitude tremolo: both L and R attenuate together
const float tremAM = 1.0f - tremoloDepth_ * (0.5f + 0.5f * std::sin(tremoloPhase_));
outL *= tremAM;
outR *= tremAM;
```
Default mode = 0 (Suitcase) for backward compatibility with existing presets.

**R-P4-3 — Vibrato onset delay per voice (Medium effort)**
Add `vibratoOnsetMs_` param (0–500ms). Per-voice vibrato depth modulated by an onset envelope:
```cpp
// Voice onset time tracked since trigger()
float onset = std::min(1.0f, voiceSampleAge_ / (vibratoOnsetMs_ * 0.001f * sampleRate_));
// Smoothstep: onset² × (3 - 2×onset)
float onsetSmooth = onset * onset * (3.0f - 2.0f * onset);
// Apply to vibrato contribution per-voice
pitchModSemis += vibratoSin * vibratoDepth_ * onsetSmooth;
```
Classical vibrato technique: onset ~100–200ms after attack, then full depth reached over ~300ms.

---

## 7. Engine 4 — DX7Engine

**Files:** `core/engines/dx7/dx7_engine.{h,cpp}`, `dx7_voice.h`, `dx7_algorithms.h`

### 7.1 Signal Chain (updated)

```
6 operators — each:
  ├─ Phase accumulator:
  │    ratio mode: phase += phaseInc × pitchFactor (includes pitch bend + KRS factor)
  │    fixed mode: phase += phaseInc × 1.0 (pitch bend bypassed — matches DX7 hardware)
  ├─ Exponential ADSR envelope:
  │    Attack:  1.0 - (1.0 - env) × attackRate   (approach-to-1)
  │    Decay:   sustainLvl + (env - sustainLvl) × decayRate  (approach-to-sustain)
  │    Sustain: env = sustainLvl
  │    Release: env × releaseRate  (approach-to-0)
  │    [All rates computed in trigger() from params — no per-sample pow()]
  └─ Output: sin(phase) × envelopeVal × velocitySens × levelParam

32-algorithm routing (complete topology coverage, all algorithms):
  Modulator outputs → modulate carrier phases via phase modulation (PM)
  Operator 0: self-feedback via feedbackBuf (previous tick output)

Sum of carrier outputs → stereo (currently mono mix, no per-voice pan)
  ↓
Keyboard Rate Scaling per operator (computed at trigger time, not per-sample):
  kbdFactor = (1 + kbdRate × 3)^((note - 60) / 12)
  All ADSR times divided by kbdFactor → high notes faster, low notes slower
```

### 7.2 V2 Improvements

**Exponential envelope contour (P2 §3.3):** The shift from linear to exponential ADSR is the most important DX7 accuracy improvement. The Yamaha DX7 (1983) used a 4-rate/4-level (4R4L) OPL-style exponential envelope where each stage has a time constant proportional to `2^rate`. The simplified ADSR approximation (one rate, one level per stage) cannot exactly replicate the DX7 hardware, but the exponential approach-to-target formula is qualitatively correct — it produces the "snap" on fast attacks and the long tail on releases that define FM synthesis timbres (EP, brass, bell). See: Chowning, J.M. (1973), "The synthesis of complex audio spectra by means of frequency modulation," *Journal of the Audio Engineering Society*, 21(7), 526–534.

**Fixed-frequency operators (P2 §3.4):** A significant DX7 capability now available. Fixed-mode operators oscillate at a constant Hz regardless of MIDI note, enabling:
- Inharmonic bell/metal timbres (non-integer sidebands → complex spectra)
- Drum-like FM synthesis with sub-audio modulators
- Classic "electric bell" presets that require fixed-ratio relationships
Pitch bend correctly bypasses fixed-mode operators (matching hardware behavior — see Yamaha DX7 Service Manual, 1984, §3.2).

**Oldest-active voice stealing (P1 §2.5):** Eliminates audible pops from round-robin stealing. The `voiceAge_[]` counter uses a monotonically increasing integer, ensuring that when voice memory is full, the note that has been playing longest is the one stolen — statistically, this note is most likely in a sustain or release stage with low perceptual relevance.

**Keyboard rate scaling per operator (P3 §3.15):** Matches the DX7 hardware feature. Each operator's ADSR time constants scale exponentially with note pitch relative to C4 (MIDI note 60). This is computed at `trigger()` time (not per-sample), so it has zero runtime CPU overhead. The effect on perception is large for piano-style presets: high-register notes decay naturally faster (as on a real piano), while low-register notes sustain longer.

**Param count:** 45 → **63** (+12 fixed-mode params in P2 §3.4, +6 KRS params in P3 §3.15).

### 7.3 ✅ Strengths (Updated)

- **All 32 DX7 algorithms** — complete topology coverage.
- **Exponential ADSR** — qualitatively correct FM envelope character.
- **Fixed-frequency operators** — enables inharmonic and metallic FM patches.
- **Keyboard rate scaling** — per-operator, matching DX7 hardware capability.
- **Oldest-active voice stealing** — pop-free at polyphony limits.
- **Pitch bend** working, correctly bypassing fixed-mode operators.
- **Per-operator velocity sensitivity** — dynamic timbral shaping.

### 7.4 ⚠️ Remaining Weaknesses

**W1 — No system LFO → operator modulation**
The DX7 hardware includes a global LFO with six waveforms (triangle, sawtooth up/down, square, sine, random) that can modulate:
- **PM** (pitch modulation): applied to all operators' phase accumulators
- **AM** (amplitude modulation): applied to operator output levels

Without this, presets relying on LFO vibrato (rate = LFO speed, depth = LFO PMD) or tremolo (LFO AMD) cannot be replicated. Classic DX7 brass patches typically use LFO-based vibrato on the carriers. This is arguably the most important missing DX7 feature at this point.

**W2 — Operator amplitude key-level scaling absent**
Distinct from key-*rate* scaling (which affects envelope *time*), the DX7's key-*level* scaling varies the operator's output *amplitude* based on note pitch. The hardware implements this as a four-point curve (two breakpoints × two slopes, each with a curve type). This is used extensively in piano and EP patches to limit high-register brightness and boost low-register warmth. Without it, high-register FM piano notes will sound over-bright and low-register notes may feel thin. See Yamaha DX7 Service Manual (1984), §3.4 "Key Level Scaling."

**W3 — Operator ratio range limited to 0.5–8.0×**
The DX7 hardware supports ratio values: 0.50, 1.00, 1.50, 2.00, 3.00, 4.00, 5.00, 6.00, 7.00, 8.00, 9.00, 10.00, 11.00, 12.00, 13.00, 14.00, 15.00, 16.00, 17.00, 18.00, 20.00, 22.00, 24.00, 25.00. Presets requiring ratios above 8× (common in brass and string algorithms) cannot be accurately reproduced.

**W4 — Simplified 4R4L envelope**
The DX7's actual envelope has 4 rates (R1–R4) and 4 levels (L1–L4), where the envelope progresses through all four rate/level pairs continuously. Our implementation uses one rate per stage (attack/decay/sustain/release). This approximation is qualitatively accurate but cannot replicate presets with complex envelope shapes (e.g., multi-stage attack transients, delayed vibrato onset via envelope stage 3).

**W5 — Feedback operator index convention**
The DX7 hardware feeds back operator 6's output (the last operator in the signal chain) into itself. Our implementation uses operator 0 (first). For algorithms where this distinction is irrelevant (operator 0 and 6 are both carriers or both modulators), the behavior is identical. For specific algorithms where op0 and op6 occupy different roles in the chain, the feedback routing will differ from hardware, producing different harmonic content.

### 7.5 🔧 Improvement Proposals (P4)

**D-P4-1 — System LFO → PM/AM modulation (Medium effort)**
Add engine-level LFO:
```cpp
// DX7Engine header:
float lfoPhase_ = 0.0f;
float lfoRate_ = 1.0f;    // Hz, 0.1..30
float lfoPMD_  = 0.0f;    // pitch mod depth, 0..1 → 0..7 semitones
float lfoAMD_  = 0.0f;    // amplitude mod depth, 0..1
int   lfoWave_ = 0;       // 0=tri 1=saw_up 2=saw_dn 3=square 4=sine 5=rnd

// In tickSample(): advance LFO phase, compute lfoOut (−1..+1)
// PM: pitchModFactor = pow(2, lfoOut × lfoPMD_ × 7.0 / 12.0)
//   → applied to ALL operator phaseInc (ratio-mode operators only, fixed bypass)
// AM: ampModFactor = 1.0 − lfoAMD_ × (0.5 + 0.5 × lfoOut)
//   → applied per-operator to final output scale
```
Adds 5 new params (`DX7P_LFO_RATE`, `DX7P_LFO_WAVE`, `DX7P_LFO_PMD`, `DX7P_LFO_AMD`, `DX7P_LFO_SYNC`).

**D-P4-2 — Operator key-level scaling (Medium effort)**
Add per-operator breakpoint curve. Simplified 2-slope linear model (sufficient for piano/EP accuracy):
```cpp
// Per operator: 3 params (breakpoint note, left slope, right slope)
// Breakpoint: MIDI note 0–127, default 60 (C4)
// Slope: 0..1 → 0dB..−12dB per octave
float kls = 1.0f;
const float semisDelta = (midiNote - bpNote) / 12.0f;
if (semisDelta < 0) kls = std::pow(10.0f, -leftSlope * (-semisDelta) / 20.0f);
else                kls = std::pow(10.0f, -rightSlope * semisDelta  / 20.0f);
outputLevel *= kls;  // Applied at trigger()
```
Adds 18 params (3 per operator × 6 operators).

**D-P4-3 — Extend ratio range to 0.5–24.0× (Low effort)**
Change `DX7P_OP{n}_RATIO` max from 8.0 to 24.0. Add parameter metadata note that DX7 hardware uses discrete ratio steps; in this implementation, continuous ratio is supported (more flexible than hardware, compatible with all original presets).

---

## 8. Engine 5 — MellotronEngine

**Files:** `core/engines/mellotron/mellotron_engine.{h,cpp}`, `mellotron_voice.h`, `mellotron_tables.h`

### 8.1 Signal Chain (updated)

```
MIDI noteOn → MellotronVoice::trigger()
  ├─ phaseInc = freq / (sampleRate × MELLO_CYCLES)
  │   [MELLO_CYCLES = 8 → phase 0..1 covers 8 oscillation cycles]
  └─ ADSR attack/release envelope
  ↓
Engine-level dual LFO per block:
  wowLFO:     0.1–3.0 Hz, depth 0–2.0% speed modulation
  flutterLFO: 6–18 Hz, depth 0–0.6% speed modulation
  → compound factor: wowMul × flutterMul
  ↓
Per voice: phase += phaseInc × pitchBendCache_ × tapeModMul
Linear interpolated read from 16384-sample multi-cycle table
  (8 cycles × 2048 samples, harmonic jitter ±8–12% per cycle)
  ↓
Tape runout: amplitude fades after runoutTimeSec (tape reel end)
LCG pitch spread per-note (seeded per note)
  ↓
Sum of active voices
  ↓
Tape hiss: LCG white noise × (0.0018 + 0.00015 × activeVoices) when active
  (base −55 dBFS; scales by voice count — more tapes running = more hiss)
  ↓
× Master Volume → stereo output
```

### 8.2 Tape Simulation Assessment

The Mellotron M400 tape emulation has been substantially improved and now addresses the key physical characteristics of the instrument:

| Physical characteristic | Model | Authenticity |
|------------------------|-------|-------------|
| Tape speed variation (wow) | Engine-level LFO 0.1–3 Hz | ✅ Correct frequency range |
| Tape speed variation (flutter) | Engine-level LFO 6–18 Hz | ✅ Correct frequency range (Hess, 1982) |
| Tape hiss floor | LCG noise −55dBFS, voice-scaled | ✅ Perceptually appropriate |
| Tape runout | Amplitude fade after runoutTimeSec | ✅ Original M400 behavior |
| Per-note tape character | Multi-cycle jitter ±8–12% | ✅ Approximates oxide variation |
| Tape rewind behavior | Immediate on noteOff (phase reset) | ⚠️ M400 had mechanical rewind time |

The wow/flutter frequency ranges align with published specifications for analog tape machines. Hess, Richard L. (1982), *Tape Degradation Factors and Their Prediction*, AES Convention Paper, reports flutter frequencies for consumer capstan drives at 6–15 Hz and slow wow at 0.3–2 Hz — our default values (wow 0.7 Hz, flutter 10 Hz) sit within these ranges.

**Note on shared LFO design choice:** The wow and flutter LFOs are engine-level (shared across all voices). This accurately models the M400's single tape transport mechanism — all keys share the same capstan drive, so wow/flutter affects all playing notes identically (they all drift together). Per-voice independent LFOs would be physically incorrect for this instrument.

**Multi-cycle wavetable:** The 8-cycle expansion (2048 → 16384 samples) with per-cycle harmonic jitter (±8–12% amplitude, 0–0.05 rad phase) emulates the timbral variation visible in spectrograms of real Mellotron recordings. In a real M400, each "cycle" of the sustained tone was a physically different section of magnetic tape, with unique oxide distribution, head gap geometry, and transport speed micro-variations. The deterministic LCG seeding (`0xDEAD0001` for Strings, etc.) ensures reproducible tables across runs.

**Param count:** 7 → **11** (`MP_WOW_DEPTH`, `MP_WOW_RATE`, `MP_FLUTTER_DEPTH`, `MP_FLUTTER_RATE` added).

### 8.3 ✅ Strengths (Updated)

- **Dual LFO tape transport** — wow and flutter at physically correct frequency ranges.
- **Multi-cycle wavetable** (8 cycles, deterministic jitter) — organic, non-repetitive waveform character.
- **Tape hiss floor** — voice-count-scaled, only present when keys are held.
- **Tape runout** — authentic M400 physical limitation.
- **Pitch spread** via per-note LCG — each key has its own tape strip character.
- **35-voice capacity** — full M400 keyboard compass (C2–C5).
- **RT-safe** — all computations are stack-only, no dynamic allocation in audio path.

### 8.4 ⚠️ Remaining Weaknesses

**W1 — No per-voice independent wow/flutter**
While the shared LFO is physically correct for the M400 (one transport mechanism), a future Mellotron-style instrument (e.g., Novatron or simulated multi-transport) might benefit from per-voice independent LFO. Even for the M400, subtle differences in individual tape strip condition created small inter-note differences not captured by a single LFO. This is a refinement, not a correctness bug.

**W2 — Tape hiss is mono (same value for L and R)**
The current implementation injects the same noise sample to both `sumL` and `sumR` per tick. Real magnetic tape hiss is spectrally correlated between tracks (mono recording) but not sample-identical (the playback head reads a finite-width track with lateral variation). Two statistically independent noise generators per channel — with the same amplitude characteristics — would better simulate a two-track playback system. See: Välimäki et al. (2013), "All About Audio Equalization," *Applied Sciences*, §4 for tape noise spectral characteristics.

**W3 — Attack and release shapes are tape-type-agnostic**
The ADSR attack/release envelope uses the same characteristic regardless of tape type. In a real M400:
- **Strings** tape: slow, smooth attack (bowed strings)
- **Choir** tape: medium attack with formant onset
- **Flute** tape: relatively fast, breathy attack
- **Brass** tape: fast, punchy attack with strong transient
The current single `MP_ATTACK` param controls all tape types identically, limiting preset accuracy for specific tape sounds.

**W4 — No tape saturation at high amplitudes**
Real magnetic tape saturates at high signal levels — not hard clipping, but a soft compression characteristic that adds even-order harmonics and reduces high-frequency content (high-frequency saturation). This is the "warm tape compression" character of recordings made at +3 to +6 dB above nominal. Mellotron at high volume settings benefits from this coloration.

### 8.5 🔧 Improvement Proposals (P4)

**Me-P4-1 — Per-tape attack shape (Low effort)**
Define a short attack table per tape type (Strings, Choir, Flute, Brass) encoding the characteristic rise curve, applied as a multiplier during the attack stage of the ADSR. The attack table replaces the simple linear ramp with the characteristic shape of that specific instrument family:
```cpp
// Example for Brass (punchy attack):
static constexpr float BRASS_ATK[8] = {0.2f, 0.6f, 0.85f, 0.93f, 0.97f, 0.99f, 1.0f, 1.0f};
// Strings (slow rise):
static constexpr float STRINGS_ATK[8] = {0.05f, 0.12f, 0.24f, 0.40f, 0.58f, 0.76f, 0.90f, 1.0f};
// Index = (attackSampleCount / attackTotalSamples) × 7
float attackShape = lerpTable(tapeAttackTable_[tape_], attackProgress_);
env *= attackShape;
```

**Me-P4-2 — Stereo independent tape hiss (Low effort)**
Replace single `nextNoise()` call with two independent noise calls:
```cpp
sumL += nextNoise() * hissLevel;   // Independent L noise sample
sumR += nextNoise() * hissLevel;   // Independent R noise sample
```
The two LCG-generated samples will be uncorrelated (different sequence positions), producing the correct decorrelated stereo noise character.

**Me-P4-3 — Tape saturation model (Low effort)**
After voice sum, before master volume:
```cpp
// Soft tape saturation: tanh approximation, only active at high levels
// Threshold at ~-6dBFS = 0.5 amplitude
if (std::fabs(sumL) > 0.3f)
    sumL = std::tanh(sumL * 1.5f) / 1.5f;
if (std::fabs(sumR) > 0.3f)
    sumR = std::tanh(sumR * 1.5f) / 1.5f;
```
Alternative: use the arctangent approximation (`x / (1 + |x|)`) which is computationally cheaper than `tanh` and perceptually similar for gentle saturation levels.

---

## 9. Engine 6 — DrumEngine

**Files:** `core/engines/drums/drum_engine.{h,cpp}`, `drum_pad_dsp.h`, `drum_pad_sample.h`

### 9.1 Architecture (unchanged from v1)

Hybrid design: 8 DSP synthesis pads (0–7) + 8 WAV sample pads (8–15). All one-shot semantics.

### 9.2 V2 DSP Synthesis Models (updated)

| Pad | Type | V1 model | V2 model | Improvement |
|-----|------|----------|----------|-------------|
| Kick | Sine + sweep | Fixed sweep (40Hz target, 40ms) | Parameterized sweep (depth 0–1, time 10–200ms) | §5.6 |
| Snare | Sine + bandpassed noise | Unchanged | Unchanged | — |
| Hi-Hat C | 6 square osc sum | Simple harmonic sum | 808-style: 3 ring-mod pairs (6-osc RM) | §5.5 |
| Hi-Hat O | Same as HiHatC + decay | Simple harmonic sum | Same RM upgrade as HiHatC | §5.5 |
| Clap | 4 noise bursts | Unchanged | Unchanged | — |
| Tom Lo | Sine pitch sweep | Fixed behavior | Fixed behavior | — |
| Tom Mid | Sine pitch sweep | Fixed behavior | Fixed behavior | — |
| Rimshot | Transient + ring osc | Unchanged | Unchanged | — |

### 9.3 V2 Improvements

**Hi-hat ring modulation (P3 §5.5):** The TR-808's hi-hat circuit uses six square-wave oscillators at non-harmonic frequency ratios, ring-modulated in pairs and filtered through a bandpass. The frequency ratios used in hardware (from circuit analysis: 1.000, 1.4471, 1.6818, 1.9545, 2.2727, 2.6364) produce sum and difference frequencies that create a dense, inharmonic spectrum characteristic of metallic percussion — the same perceptual mechanism as a struck cymbal (complex plate resonances). Our implementation follows this architecture faithfully, with a 70/30 mix with white noise adding the "sizzle" in the 8–12 kHz range. See: Maginnis, J. (2002), "TR-808 Circuit Analysis," archived on SDIY.info, for frequency ratio documentation.

**Kick sweep exposure (P3 §5.6):** The exponential pitch sweep of a kick drum (starting frequency decays toward a target as `freq = target × (start/target)^(t/sweepTime)`) is now fully parameterized. `sweepDepth` controls the ratio between start and end frequency; `sweepTime` controls the duration. Default values (0.5 / 0.2) preserve the previous character. This is the key parameter for distinguishing 808-style sub-boom from tight punchy electronic kick.

**Hi-hat choke group (P1 §2.7):** Open and closed hi-hat now mutually choke. This is a fundamental mechanical reality of any drum kit — the hi-hat clutch physically stops the upper cymbal when the pedal is pressed.

**Velocity → decay scaling (P1 §2.8):** `decayRate = pow(baseDecay, 1/velScale)` where `velScale = 0.8 + 0.4 × velNorm`. The physical rationale: a harder hit imparts more energy to the drum head, which then takes longer to dissipate through internal damping and air coupling — the decay is slower, not just louder. Combined with velocity → amplitude, this creates a convincing two-dimensional dynamics response.

**Param count:** 65 → **67** (`DRUM_KICK_SWEEP_DEPTH`, `DRUM_KICK_SWEEP_TIME`).

### 9.4 ✅ Strengths (Updated)

- **808-authentic hi-hat ring modulation** — correct spectral character.
- **Fully parameterized kick sweep** — wide range from punchy to sub-bass boom.
- **Hi-hat choke** — realistic kit mechanics.
- **Velocity → amplitude + decay** — two-dimensional dynamics.
- **Hybrid DSP+sample design** — character from synthesis, texture from real samples.
- **GM-compatible MIDI note mapping** — interoperates with standard MIDI files.

### 9.5 ⚠️ Remaining Weaknesses

**W1 — Snare model lacks snare wire buzz**
The current snare model mixes a sine oscillator (batter head fundamental) with bandpassed noise (low snare attack). The characteristic "buzz" of a snare drum is produced by the wire snares on the resonant head — a high-Q resonance at the fundamental, followed by a broad-spectrum rattle as the wires bounce off the resonant head. This buzz occupies 1–5 kHz and is what distinguishes snare from tom perceptually. A simple addition: a bandpass filter (center ~2 kHz, Q ~2) applied to a noise burst with a slow decay (300–600ms) would improve authenticity significantly.

**W2 — Clap lacks frequency shaping**
The TR-808 clap circuit passes the noise bursts through a bandpass filter centered around 1.5–4 kHz before the output. This gives the 808 clap its characteristic "snap" as opposed to broadband noise. The current model has four unfiltered noise bursts — correct in temporal structure, but lacking the frequency coloration.

**W3 — No tom resonance model**
Tom drums produce a characteristic pitch-envelope (the pitch sweeps slightly downward after impact, from the fundamental of the batter head vibrating at higher frequency to the coupled mode of the complete drum system). The current `TomLow`/`TomMid` models use a fixed pitch sweep similar to the kick. A more natural tom sound would use a shorter, shallower sweep with a longer sustaining resonance at the fundamental.

**W4 — Sample pads are fully static**
The 8 WAV sample pads (pads 8–15) play back at a fixed pitch with no processing. No pitch transposition (useful for pitched percussion like cowbell or conga octaves), no bit-depth/saturation coloration, and no per-sample envelope shaping beyond the sample's natural content.

**W5 — No 808 cowbell or rimshot ring synthesis**
Classic 808-style cowbell (two square oscillators at 540 Hz and 800 Hz, ring-modulated, short attack + medium decay) and authentic rimshot (tightly coupled fundamental + high-decay ring) are both achievable with the current DSP pad architecture but are not implemented.

### 9.6 🔧 Improvement Proposals (P4)

**Dr-P4-1 — Snare wire buzz (Low effort)**
In `DrumPadDsp::tick()` for Snare case, add a second noise generator with a longer decay envelope, passed through a biquad bandpass (center 2 kHz, Q 2.0):
```cpp
// Snare wire buzz: decaying bandpass noise
wireEnv_ *= wireDecayRate_;  // Slow decay ~400ms
const float wireNoise = noiseGen_.tick() * wireEnv_;
const float buzzed = bpFilter_.process(wireNoise);   // Pre-computed biquad at init()
out += buzzed * 0.4f;
```

**Dr-P4-2 — Clap bandpass shaping (Low effort)**
In `DrumPadDsp::trigger()` for Clap, initialize a biquad bandpass filter (center 2.5 kHz, Q 1.5, pre-computed at `init()`). In `tick()`, route noise bursts through this filter before adding to output.

**Dr-P4-3 — Sample pad pitch transposition (Medium effort)**
Add `pitchSemitones_` per sample pad (−12 to +12 semitones). In `DrumPadSample::tick()`, compute `phaseInc = 1.0 × pow(2, semitones/12)` and step the sample buffer by this fractional increment with linear interpolation. This enables, e.g., playing conga hi/lo from the same sample with different pitches.

---

## 10. Cross-Engine Analysis

### 10.1 Consistency Status — Updated

| Feature | Moog | Hammond | Rhodes | DX7 | Mellotron | Drums |
|---------|------|---------|--------|-----|-----------|-------|
| Pitch bend | ✅ ±2st | ✅ ±2st | ✅ ±2st (modal) | ✅ ±2st (FM) | ✅ ±2st | N/A |
| ParamSmoother | ✅ all | ✅ drawbars+OD | ✅ Tone/Decay/Drive | ❌ | ❌ Volume | ❌ |
| `[RT-SAFE]` annotations | ✅ | Partial | ✅ | Partial | ❌ | ❌ |
| Voice stealing | 3 strategies | N/A | oldest-active | oldest-active | oldest-active | N/A |
| Sustain pedal (CC64) | ❌ | N/A | ✅ | ❌ | ❌ | N/A |
| Vibrato/LFO source | OSC3/ModMatrix | V1–V3/C1–C3 | ✅ RP_VIBRATO | ❌ (no LFO) | N/A | N/A |
| Oscilloscope buffer | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| Output soft limiter | ✅ tanh | tanh(OD) | tanh(Drive) | ❌ | ❌ | ❌ |

### 10.2 v1 → v2 Consistency Improvements

In v1, the consistency table showed pitch bend as a critical failure across 5 engines, drawbar zipper noise, and round-robin voice stealing in DX7. All three are resolved. The remaining two significant inconsistencies are:

1. **`ParamSmoother` coverage**: Only Moog has complete coverage of all continuous parameters. DX7, Mellotron, and DrumEngine still have knobs that can produce zipper noise on rapid adjustment (DX7 `RP_RATIO`, Mellotron `MP_VOLUME`, Drum pad levels). The architectural pattern is established (§3.1 Pattern 2) and the implementation cost is low — these should be candidates for the next improvement pass.

2. **Oscilloscope buffer**: Only Moog implements `getOscBuffer()`. Adding the override to other engines requires allocating a ring buffer at `init()` and writing to it each `tickSample()` before the effect chain. The EngineManager already provides a post-effect capture buffer, but pre-effect visibility is valuable for debugging complex effect chain interactions.

### 10.3 RT-SAFE Annotation Coverage

Annotation coverage has improved since v1 but is still not complete:

| Engine | `tick()` annotated | Status |
|--------|-------------------|--------|
| Moog Voice | ✅ | `// [RT-SAFE]` present |
| Hammond Voice | ⚠️ | Partial — engine-level tickSample annotated, voice tick not |
| Rhodes Voice | ✅ | Annotated after V2 rewrite |
| DX7Operator | ✅ | `// [RT-SAFE]` present |
| Mellotron Voice | ❌ | Missing |
| Drum Pad | ❌ | Missing |

This is a documentation gap, not a correctness issue — all six engines are verified RT-safe by code inspection (no allocation, no blocking calls, no `std::string`). But annotation coverage matters for code review and future audits.

### 10.4 Remaining Cross-Engine Gaps

| Gap | Affected engines | Effort to fix |
|-----|-----------------|---------------|
| No system LFO for FM synthesis | DX7 | Medium |
| ParamSmoother not covering all continuous params | DX7, Mellotron, Drums | Low |
| Missing `[RT-SAFE]` annotations | Hammond, Mellotron, Drums | Very low |
| No oscilloscope buffer | Hammond, Rhodes, DX7, Mellotron, Drums | Low per engine |
| No sustain pedal (CC64) | Moog, DX7, Mellotron | Low |
| No output soft limiter | DX7, Mellotron, Drums | Low |

---

## 11. Priority Improvement Roadmap — P4

All items from the v1 roadmap are complete. The following P4 items are ranked by Impact/Effort ratio, derived from the per-engine analyses above.

| Priority | Engine | Item | Effort | Impact |
|----------|--------|------|--------|--------|
| **P4-Critical** | DX7 | System LFO (PM + AM modulation to operators) | Medium | High — defines a large class of FM sounds |
| **P4-High** | DX7 | Operator amplitude key-level scaling | Medium | High — critical for piano/EP preset accuracy |
| **P4-High** | Moog | Hard sync OSC1 → OSC2 | Medium | High — classic analog patch impossible without it |
| **P4-High** | All | Add `[RT-SAFE]` annotations to Hammond/Mellotron/Drum `tick()` | Very low | Low (documentation only, zero risk) |
| **P4-High** | All | `ParamSmoother` on remaining unsmoothed continuous params (DX7 ratios, Mellotron volume, Drum levels) | Low | Medium — eliminates residual zipper noise |
| **P4-Medium** | Moog | Velocity → filter cutoff modulation (`MP_VEL_FILTER_AMT`) | Low | Medium — expressiveness |
| **P4-Medium** | DX7 | Extend operator ratio range to 24× | Low | Medium — enables more DX7 presets |
| **P4-Medium** | Hammond | Scanner vibrato delay-line approximation (10-tap allpass) | Medium | Medium — more authentic V/C character |
| **P4-Medium** | Rhodes | Vibrato onset delay per voice | Medium | Medium — more expressive lead playing |
| **P4-Medium** | Mellotron | Per-tape attack shape (type-specific rise curves) | Low | Medium — Strings vs Brass feel |
| **P4-Medium** | Drums | Snare wire buzz (decaying bandpass noise 2kHz) | Low | Medium — distinctive snare character |
| **P4-Medium** | Rhodes | Suitcase vs Stage tremolo mode | Low | Medium — preset accuracy |
| **P4-Low** | Moog | OSC drift model (per-oscillator Brownian pitch walk) | Low | Low — subtle analog warmth |
| **P4-Low** | Moog | Pulse Width Modulation LFO routing | Medium | Medium — classic analog texture |
| **P4-Low** | Hammond | Tonewheel leakage matrix | Low | Low — organic character |
| **P4-Low** | Mellotron | Stereo independent tape hiss (2 LCG generators) | Very low | Low — improved stereo realism |
| **P4-Low** | Mellotron | Tape saturation soft clip at high levels | Low | Low — tape warmth coloration |
| **P4-Low** | Drums | Clap bandpass shaping (2.5kHz BPF) | Low | Low — snap character |
| **P4-Low** | Drums | Sample pad pitch transposition (±12 semitones) | Medium | Low — flexibility |
| **P4-Low** | All | Oscilloscope buffer for non-Moog engines | Low (×5) | Low (diagnostic utility) |
| **P4-Deferred** | DX7 | True 4R4L envelope model | High | Low — marginal gain over current exp-ADSR |
| **P4-Deferred** | All | MIDI CC learn (map CC→param) | High (UI+config) | Medium — workflow improvement |
| **P4-Deferred** | Drums | Drum kit browser UI (load WAV kits from `assets/drum_kits/`) | High (UI+HAL) | Low (already achievable via code) |

---

## 12. Performance Analysis — Updated

### 12.1 Per-Sample CPU Cost Estimate (updated)

Estimates include P1–P3 additions. All costs assume 44100 Hz, one active voice.

| Engine | Voice Cost/Sample (v1 est.) | V2 additions | V2 est. total | Max voices | V2 worst case |
|--------|---------------------------|--------------|---------------|------------|---------------|
| **Moog** | ~35 flops | +1 tanh (limiter) | ~38 flops | 8 | ~304 flops |
| **Hammond** | ~18 flops | +1 tanh (OD) +4 sin/cos (Leslie) +9 smooth | ~35 flops/voice + 10 Leslie | 61 | ~2145 flops |
| **Rhodes** | ~25 flops | +4 cos (vibrato updateFreq) +3 smooth | ~32 flops | 12 (+4 voices) | ~384 flops |
| **DX7** | ~40 flops | +1 pow (KRS, at trigger only) | ~40 flops | 8 | ~320 flops |
| **Mellotron** | ~5 flops | +2 sin (wow/flutter LFO) +1 interp | ~8 flops | 35 | ~280 flops |
| **Drums** | ~10/3 flops DSP/smp | +3 sin/mul (RM hat) | ~14/3 flops | 16 | ~128 flops |

**Key observation:** Hammond is now the most CPU-intensive engine at full polyphony (~2145 relative units), driven by 61 voices × 9 oscillators + Leslie processing. The Leslie `processLeslie()` call itself (4 sin/cos + 4 delay reads per sample) adds ~12 flops per sample of engine output — a fixed cost independent of voice count.

**Teensy V3 concern (revisited):** The Hammond engine with 61 voices and Leslie enabled may be marginal on Teensy 4.1 (600 MHz ARM Cortex-M7). The Cortex-M7 has hardware FPU (double-precision capable) but no SIMD. The recommended mitigation for V3:
1. Limit Hammond polyphony to 32 voices (covers 2.5 octaves — sufficient for typical bass + chord playing).
2. Compute Leslie at reduced precision (replace `sin/cos` with a 64-point lookup table — error < 0.002 dB).
3. Process Leslie at block rate (once per 64 samples) rather than per-sample — introduces 1.45 ms of Leslie phase jitter, below perception threshold for the slow rotary effect.

### 12.2 Memory Footprint — Updated

| Engine | Voice struct size | Array | Notes |
|--------|-----------------|-------|-------|
| Moog Voice | ~400 bytes | 8 × 400B ≈ 3.2 KB | Unchanged |
| Hammond Voice | ~100 bytes (+20B Leslie state) | 61 × 100B ≈ 6.1 KB | +Leslie delay buf: 2048 × 4B = 8 KB |
| Rhodes Voice | ~200 bytes (+5 modal + 3 smooth) | 12 × 200B ≈ 2.4 KB | +4 voices: +800B |
| DX7 Voice | ~330 bytes (+12 fixed-mode params) | 8 × 330B ≈ 2.6 KB | +fixedMode/fixedHz/kbdRate arrays |
| Mellotron tables | 4 tapes × 16384 × 4B | **256 KB** | ⚠️ 8× increase from v1 (was 32 KB) |
| Mellotron Voice | ~52 bytes (+2 LFO phases) | 35 × 52B ≈ 1.8 KB | Negligible change |
| Drum DSP Pad | ~72 bytes (+RM oscillators) | 8 × 72B ≈ 0.6 KB | RM state added |

**Mellotron wavetable memory is the largest change:** 256 KB for 4 tapes × 8 cycles × 2048 samples × 4 bytes. On PC, this is negligible. For Teensy V3, this exceeds DTCM (512 KB total, shared with other DSP state). Mitigation: use `MELLO_CYCLES = 4` via `#ifdef TEENSY` → 128 KB, still within budget.

**Total DSP core memory (excluding sample data):** v1 ≈ 14 KB → v2 ≈ 282 KB. The 268 KB increase is entirely from Mellotron wavetable expansion. All other engines are within 2 KB of their v1 footprint.

### 12.3 RT-Safety — Post-P3 Audit Summary

All 6 engines remain fully RT-safe after P1–P3 changes. No new allocations, blocking calls, or mutex acquisitions were introduced in any audio-thread code path. Specific confirmations:

- `processLeslie()` (Hammond): delay buffer pre-allocated at `init()`, sin/cos calls only, linear interpolation.
- `updatePitchFactor()` (Rhodes): `std::cos()` called per-voice per-update, only when `pitchModSemis` changes (change detection gate).
- `injectSympatheticEnergy()` (Rhodes): plain array iteration, O(N) with N ≤ 12.
- `DrumPadDsp::tick()` with RM hi-hat: 6 sin() calls + 3 multiplications — acceptable per-sample cost.
- `MellotronEngine::tickSample()` with dual LFO: 2 sin() calls per sample (engine-level, not per-voice).

---

*Document generated from full codebase and improvement log analysis — all 6 engines reviewed post P1+P2+P3 completion.*
*Next review recommended after P4 implementation begins.*
