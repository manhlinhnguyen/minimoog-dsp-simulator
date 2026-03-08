# MiniMoog DSP Simulator — Preset Reference Guide

> **Version:** 2.2  
> **Last Updated:** 2025  
> **Total Presets:** 85 (50 Engine Presets + 35 Global Presets)

---

## Mục Lục (Table of Contents)

1. [Tổng Quan (Overview)](#1-tổng-quan-overview)
2. [Bảng Tổng Hợp Preset (Master Preset Table)](#2-bảng-tổng-hợp-preset-master-preset-table)
   - 2.1 [Engine Presets](#21-engine-presets)
   - 2.2 [Global Presets](#22-global-presets)
3. [Moog Engine Presets](#3-moog-engine-presets)
4. [Hammond B-3 Engine Presets](#4-hammond-b-3-engine-presets)
5. [Rhodes Mark I Engine Presets](#5-rhodes-mark-i-engine-presets)
6. [DX7 Engine Presets](#6-dx7-engine-presets)
7. [Mellotron M400 Engine Presets](#7-mellotron-m400-engine-presets)
8. [Global Presets — Moog](#8-global-presets--moog)
9. [Global Presets — Hammond B-3](#9-global-presets--hammond-b-3)
10. [Global Presets — Rhodes Mark I](#10-global-presets--rhodes-mark-i)
11. [Global Presets — Yamaha DX7](#11-global-presets--yamaha-dx7)
12. [Global Presets — Mellotron M400](#12-global-presets--mellotron-m400)
13. [Global Presets — Drum Machine](#13-global-presets--drum-machine)
14. [Hướng Dẫn Sử Dụng Preset (How to Use)](#14-hướng-dẫn-sử-dụng-preset-how-to-use)
15. [Tham Khảo Effect Chain (Effect Reference)](#15-tham-khảo-effect-chain-effect-reference)

---

## 1. Tổng Quan (Overview)

MiniMoog DSP Simulator V2.2 cung cấp **85 preset** được thiết kế sẵn, bao gồm:

- **50 Engine Presets**: Mỗi engine có 10 preset chuyên biệt, chỉ chứa tham số engine (không có effect).
- **35 Global Presets**: Preset toàn cục bao gồm cả tham số engine VÀ effect chain, sẵn sàng sử dụng cho biểu diễn/sáng tác.

### Phân Loại Preset

| Engine | Engine Presets | Global Presets | Tổng |
|--------|:---:|:---:|:---:|
| MiniMoog Model D | 10 | 7 | 17 |
| Hammond B-3 | 10 | 5 | 15 |
| Rhodes Mark I | 10 | 5 | 15 |
| Yamaha DX7 | 10 | 6 | 16 |
| Mellotron M400 | 10 | 5 | 15 |
| Hybrid Drum Machine | — | 7 | 7 |
| **Tổng cộng** | **50** | **35** | **85** |

### Thư Mục Lưu Trữ

| Loại | Thư mục |
|------|---------|
| Moog Engine Presets | `assets/moog_presets/` |
| Hammond Engine Presets | `assets/hammond_presets/` |
| Rhodes Engine Presets | `assets/rhodes_presets/` |
| DX7 Engine Presets | `assets/dx7_presets/` |
| Mellotron Engine Presets | `assets/mellotron_presets/` |
| Global Presets (tất cả engine) | `assets/global_presets/` |

---

## 2. Bảng Tổng Hợp Preset (Master Preset Table)

### 2.1 Engine Presets

| # | File | Engine | Tên Preset | Thể Loại | Mô Tả Ngắn |
|---|------|--------|-----------|----------|-------------|
| 1 | `moog_presets/01_classic_bass.json` | Moog | Classic Bass | Bass | Bass Moog kinh điển, 2 OSC saw |
| 2 | `moog_presets/02_fat_unison_bass.json` | Moog | Fat Unison Bass | Bass | 3 OSC saw unison dày dặn |
| 3 | `moog_presets/03_squelchy_acid.json` | Moog | Squelchy Acid | Bass | Acid bass resonance cao |
| 4 | `moog_presets/04_warm_pad.json` | Moog | Warm Pad | Pad | Pad ấm, attack chậm |
| 5 | `moog_presets/05_screaming_lead.json` | Moog | Screaming Lead | Lead | Lead xé, resonance + filter mod |
| 6 | `moog_presets/06_funky_clav.json` | Moog | Funky Clav | Keys | Clavinet funk nhịp nhàng |
| 7 | `moog_presets/07_brass_stab.json` | Moog | Brass Stab | Brass | Brass đâm sắc nét |
| 8 | `moog_presets/08_percussive_pluck.json` | Moog | Percussive Pluck | Keys | Pluck gõ ngắn, không sustain |
| 9 | `moog_presets/09_sub_bass.json` | Moog | Sub Bass | Bass | Sub bass triangle thuần |
| 10 | `moog_presets/10_cosmic_sweep.json` | Moog | Cosmic Sweep | FX | Sweep vũ trụ, LFO→Filter |
| 11 | `hammond_presets/01_jimmy_smith_jazz.json` | Hammond | Jimmy Smith Jazz | Jazz | Jazz organ kinh điển 88 8000 000 |
| 12 | `hammond_presets/02_gospel_full.json` | Hammond | Gospel Full | Gospel | Full drawbar 88 8888 888 |
| 13 | `hammond_presets/03_rock_overdrive.json` | Hammond | Rock Overdrive | Rock | Jon Lord rock sound |
| 14 | `hammond_presets/04_soft_ballad.json` | Hammond | Soft Ballad | Ballad | Ballad nhẹ nhàng 00 8420 000 |
| 15 | `hammond_presets/05_booker_t.json` | Hammond | Booker T | Soul | Green Onions style |
| 16 | `hammond_presets/06_cathedral_organ.json` | Hammond | Cathedral Organ | Classical | Pipe organ style 60 8080 006 |
| 17 | `hammond_presets/07_reggae_bubble.json` | Hammond | Reggae Bubble | Reggae | Reggae bubble 80 0800 000 |
| 18 | `hammond_presets/08_gospel_shout.json` | Hammond | Gospel Shout | Gospel | Gospel shout mạnh mẽ |
| 19 | `hammond_presets/09_jazz_trio.json` | Hammond | Jazz Trio | Jazz | Jazz trio nhẹ nhàng |
| 20 | `hammond_presets/10_progressive_rock.json` | Hammond | Progressive Rock | Rock | Prog rock full organ |
| 21 | `rhodes_presets/01_classic_ep.json` | Rhodes | Classic EP | Keys | Rhodes kinh điển |
| 22 | `rhodes_presets/02_suitcase_tremolo.json` | Rhodes | Suitcase Tremolo | Keys | Suitcase tremolo đặc trưng |
| 23 | `rhodes_presets/03_dyno_bright.json` | Rhodes | Dyno Bright | Keys | Dyno-My bright EQ boost |
| 24 | `rhodes_presets/04_jazz_ballad.json` | Rhodes | Jazz Ballad | Jazz | Jazz ballad ấm áp |
| 25 | `rhodes_presets/05_funk_rhodes.json` | Rhodes | Funk Rhodes | Funk | Funk clavinet-like |
| 26 | `rhodes_presets/06_glass_keys.json` | Rhodes | Glass Keys | Keys | Pha lê sáng, không drive |
| 27 | `rhodes_presets/07_lo_fi_keys.json` | Rhodes | Lo-Fi Keys | Keys | Lo-fi vintage drive cao |
| 28 | `rhodes_presets/08_wurlitzer_style.json` | Rhodes | Wurlitzer Style | Keys | Wurlitzer vibe, bright+drive |
| 29 | `rhodes_presets/09_dream_pad.json` | Rhodes | Dream Pad | Pad | Pad mơ màng, release dài |
| 30 | `rhodes_presets/10_neo_soul.json` | Rhodes | Neo Soul | Keys | Neo soul ấm, gentle trem |
| 31 | `dx7_presets/01_electric_piano.json` | DX7 | E.Piano 1 | Keys | DX7 E.Piano kinh điển (Algo 5) |
| 32 | `dx7_presets/02_dx_bass.json` | DX7 | DX Bass | Bass | FM bass đặc trưng (Algo 1) |
| 33 | `dx7_presets/03_brass_stab.json` | DX7 | Brass Section | Brass | FM brass section (Algo 22) |
| 34 | `dx7_presets/04_marimba.json` | DX7 | Wooden Marimba | Perc | Marimba gỗ (Algo 5) |
| 35 | `dx7_presets/05_strings.json` | DX7 | Strings Pad | Pad | Dây pad FM (Algo 4) |
| 36 | `dx7_presets/06_tubular_bells.json` | DX7 | Tubular Bells | Perc | Chuông ống (Algo 1) |
| 37 | `dx7_presets/07_organ.json` | DX7 | DX Organ | Keys | FM organ (Algo 4) |
| 38 | `dx7_presets/08_synth_lead.json` | DX7 | Synth Lead | Lead | FM lead xé (Algo 1) |
| 39 | `dx7_presets/09_vibraphone.json` | DX7 | Vibraphone | Perc | Vibraphone FM (Algo 5) |
| 40 | `dx7_presets/10_harmonica.json` | DX7 | Harmonica | Lead | Harmonica reedy (Algo 1) |
| 41 | `mellotron_presets/01_orchestral_strings.json` | Mellotron | Orchestral Strings | Strings | Strings tape kinh điển |
| 42 | `mellotron_presets/02_angelic_choir.json` | Mellotron | Angelic Choir | Pad | Choir thiên thần |
| 43 | `mellotron_presets/03_solo_flute.json` | Mellotron | Solo Flute | Lead | Sáo solo rõ ràng |
| 44 | `mellotron_presets/04_power_brass.json` | Mellotron | Power Brass | Brass | Kèn mạnh mẽ |
| 45 | `mellotron_presets/05_lo_fi_strings.json` | Mellotron | Lo-Fi Strings | Strings | Strings xuống cấp, wow mạnh |
| 46 | `mellotron_presets/06_ethereal_choir.json` | Mellotron | Ethereal Choir | Pad | Choir siêu thực, attack chậm |
| 47 | `mellotron_presets/07_vintage_flute.json` | Mellotron | Vintage Flute | Lead | Sáo vintage, wow nhẹ |
| 48 | `mellotron_presets/08_dark_strings.json` | Mellotron | Dark Strings | Strings | Strings tối, tape chậm |
| 49 | `mellotron_presets/09_mellow_brass.json` | Mellotron | Mellow Brass | Brass | Kèn dịu, attack chậm |
| 50 | `mellotron_presets/10_degraded_tape.json` | Mellotron | Degraded Tape | FX | Tape hư hỏng cực độ |

### 2.2 Global Presets

| # | File | Engine | Tên Preset | Thể Loại | Effects | Mô Tả Ngắn |
|---|------|--------|-----------|----------|---------|-------------|
| 1 | `global_presets/01_moog_arena_lead.json` | Moog | Arena Lead | Lead | Delay → Reverb → EQ | Lead sân khấu lớn |
| 2 | `global_presets/02_moog_deep_sub.json` | Moog | Deep Sub | Bass | EQ → Reverb | Sub bass sâu thẳm |
| 3 | `global_presets/03_moog_acid_house.json` | Moog | Acid House | Bass | Flanger → Delay(BPM) | Acid 303-style |
| 4 | `global_presets/04_moog_ambient_pad.json` | Moog | Ambient Pad | Pad | Chorus → Reverb → Tremolo | Pad ambient mênh mông |
| 5 | `global_presets/05_moog_retro_funk.json` | Moog | Retro Funk | Lead | Phaser → Gain(OD) | Funk retro kiểu 70s |
| 6 | `global_presets/06_moog_dark_drone.json` | Moog | Dark Drone | FX | Reverb → Delay → EQ | Drone tối, không gian lớn |
| 7 | `global_presets/07_moog_80s_poly.json` | Moog | 80s Poly Synth | Keys | Chorus → Delay → Reverb | Synth poly kiểu 80s |
| 8 | `global_presets/08_hammond_gospel_live.json` | Hammond | Gospel Live | Keys | Gain(OD) → Reverb | Gospel biểu diễn live |
| 9 | `global_presets/09_hammond_jazz_club.json` | Hammond | Jazz Club | Keys | Reverb → EQ(warm) | Jazz club ấm áp |
| 10 | `global_presets/10_hammond_prog_rock.json` | Hammond | Progressive Rock | Keys | Gain(dist) → Phaser → Delay | Prog rock đầy năng lượng |
| 11 | `global_presets/11_hammond_soul_groove.json` | Hammond | Soul Groove | Keys | Tremolo → Chorus → Reverb | Soul groove mượt mà |
| 12 | `global_presets/12_hammond_church.json` | Hammond | Church Organ | Keys | Reverb(cathedral) → EQ | Nhà thờ trang nghiêm |
| 13 | `global_presets/13_rhodes_neo_soul.json` | Rhodes | Neo Soul Keys | Keys | Chorus → Reverb | Neo soul đương đại |
| 14 | `global_presets/14_rhodes_jazz_lounge.json` | Rhodes | Jazz Lounge | Keys | Reverb → EQ(warm) | Jazz lounge sang trọng |
| 15 | `global_presets/15_rhodes_funk_wah.json` | Rhodes | Funk Wah | Keys | Phaser → Tremolo | Funk wah sôi động |
| 16 | `global_presets/16_rhodes_chill_wave.json` | Rhodes | Chill Wave | Pad | Chorus → Delay → Reverb | Chill wave thư giãn |
| 17 | `global_presets/17_rhodes_vintage_amp.json` | Rhodes | Vintage Amp | Keys | Gain(drive) → Tremolo → Reverb | Amp vintage 60s |
| 18 | `global_presets/18_dx7_80s_ep.json` | DX7 | 80s Electric Piano | Keys | Chorus → Reverb | EP kiểu 80s kinh điển |
| 19 | `global_presets/19_dx7_crystal_bells.json` | DX7 | Crystal Bells | FX | Delay → Reverb(shimmer) | Chuông pha lê lấp lánh |
| 20 | `global_presets/20_dx7_synth_brass.json` | DX7 | Synth Brass | Brass | Chorus → Delay | FM brass tổng hợp |
| 21 | `global_presets/21_dx7_ambient_bell.json` | DX7 | Ambient Bell | FX | Reverb(cathedral) → Delay | Chuông ambient mênh mông |
| 22 | `global_presets/22_dx7_bass_stab.json` | DX7 | Bass Stab | Bass | Gain(boost) → EQ(bass) | Bass stab FM sắc nét |
| 23 | `global_presets/23_dx7_fm_organ.json` | DX7 | FM Organ | Keys | Chorus → Reverb(room) | FM organ ấm áp |
| 24 | `global_presets/24_mellotron_strawberry.json` | Mellotron | Strawberry Fields | Pad | Flanger → Reverb(hall) | Kiểu Beatles 1967 |
| 25 | `global_presets/25_mellotron_space_choir.json` | Mellotron | Space Choir | Pad | Delay → Reverb → Chorus | Choir vũ trụ |
| 26 | `global_presets/26_mellotron_vintage_orch.json` | Mellotron | Vintage Orchestra | Pad | EQ(warm) → Reverb(plate) | Dàn nhạc vintage |
| 27 | `global_presets/27_mellotron_dark_ambient.json` | Mellotron | Dark Ambient | FX | Reverb(huge) → Delay → EQ | Ambient tối, không gian vô tận |
| 28 | `global_presets/28_mellotron_brass_section.json` | Mellotron | Brass Section | Brass | Gain(boost) → Reverb(room) | Dàn kèn vintage |
| 29 | `global_presets/29_drums_808_boom.json` | Drums | 808 Boom Kit | Drums | EQ(bass+) → Gain(light) | 808 kick sâu, sub bass |
| 30 | `global_presets/30_drums_rock_kit.json` | Drums | Rock Kit | Drums | Reverb(room) → EQ | Kit rock cân bằng |
| 31 | `global_presets/31_drums_jazz_brush.json` | Drums | Jazz Brush Kit | Drums | Reverb(room) → EQ(warm) | Kit jazz nhẹ nhàng |
| 32 | `global_presets/32_drums_electronic.json` | Drums | Electronic Kit | Drums | Delay(1/16) → Reverb | Kit điện tử sắc nét |
| 33 | `global_presets/33_drums_lo_fi.json` | Drums | Lo-Fi Kit | Drums | Gain(OD) → EQ(dark) → Reverb | Kit lo-fi bẩn |
| 34 | `global_presets/34_drums_dub_echo.json` | Drums | Dub Echo Kit | Drums | Delay(long) → Reverb(big) | Kit dub echo sâu |
| 35 | `global_presets/35_drums_industrial.json` | Drums | Industrial Kit | Drums | Gain(dist) → Flanger → Delay | Kit industrial hung hãn |

---

## 3. Moog Engine Presets

Thư mục: `assets/moog_presets/`  
Engine: **MiniMoog Model D** — Bộ tổng hợp analog 3 oscillator với Moog Ladder Filter 4-pole.

### 3.1 Classic Bass
**File:** `01_classic_bass.json`  
**Thể loại:** Bass  

Âm bass Moog kinh điển nhất — nền tảng của vô số bản nhạc từ thập niên 70. Sử dụng 2 oscillator sóng Sawtooth ở quãng tám 16' và 8', detuned nhẹ để tạo độ dày. Filter cutoff thấp (0.35) với resonance vừa phải (0.6) tạo âm ấm và tròn trịa. Chế độ Mono với Glide 0.15s cho phép trượt note mượt mà.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| OSC1 Waveform | Sawtooth | Harmonics phong phú |
| OSC1 Range | 16' | Quãng tám thấp |
| OSC2 Waveform | Sawtooth | Detuned +5 cents |
| OSC2 Range | 8' | Quãng tám giữa |
| Filter Cutoff | 0.35 | Cắt phần sáng |
| Filter Resonance | 0.6 | Nhấn mạnh cutoff |
| Voice Mode | Mono | Đơn âm |
| Glide Time | 0.15 | Trượt note |

**Cách sử dụng:** Phù hợp cho dòng bass funk, R&B, disco. Thử chơi staccato 1/8 note hoặc legato với glide.

---

### 3.2 Fat Unison Bass
**File:** `02_fat_unison_bass.json`  
**Thể loại:** Bass  

Bass cực kỳ dày dặn với 3 oscillator Sawtooth trong chế độ Unison. Cả 3 OSC được detune mạnh (0.35) tạo hiệu ứng chorus tự nhiên. Filter cutoff 0.4 giữ sự ấm áp nhưng vẫn có đủ harmonics.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| OSC1/2/3 | Sawtooth 16' | 3 saw cùng quãng |
| Unison Detune | 0.35 | Detune mạnh |
| Filter Cutoff | 0.4 | Thấp-trung |
| Voice Mode | Unison | 3 voice cùng note |

**Cách sử dụng:** Dubstep, EDM bass drops. Bấm giữ note thấp cho âm rung massive.

---

### 3.3 Squelchy Acid
**File:** `03_squelchy_acid.json`  
**Thể loại:** Bass  

Tái tạo âm acid bass kinh điển kiểu TB-303, với resonance cực cao (0.85) và filter envelope mạnh (0.9). Decay ngắn tạo "squelch" đặc trưng. Một OSC Sawtooth 16' đơn giản nhưng filter biến hóa khôn lường.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| OSC1 Waveform | Sawtooth | Đơn giản nhưng hiệu quả |
| Filter Resonance | 0.85 | Gần self-oscillation |
| Filter Env Amount | 0.9 | Sweep lớn |
| Amp Env Decay | 0.15 | Ngắn, nhấp |
| Voice Mode | Mono + Glide | Acid slide |

**Cách sử dụng:** Acid house, techno. Dùng sequencer 1/16 note với accent pattern. Filter envelope tạo "wah" tự động.

---

### 3.4 Warm Pad
**File:** `04_warm_pad.json`  
**Thể loại:** Pad  

Pad ấm áp với 2 oscillator — Triangle (nền ấm) + Sawtooth (harmonics nhẹ). Attack chậm (0.45) cho âm thanh từ từ xuất hiện. Chế độ Poly cho phép chơi hợp âm mềm mại.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| OSC1 | Triangle 8' | Nền ấm, ít harmonics |
| OSC2 | Sawtooth 8' | Thêm texture |
| Amp Attack | 0.45 | Dần dần xuất hiện |
| Amp Sustain | 0.8 | Duy trì lâu |
| Voice Mode | Poly | Hợp âm |
| Filter Cutoff | 0.5 | Trung bình |

**Cách sử dụng:** Ambient, chillout, ballad. Chơi hợp âm 3-4 note, để ring dài.

---

### 3.5 Screaming Lead
**File:** `05_screaming_lead.json`  
**Thể loại:** Lead  

Lead kiểu rock/synth sắc bén — Square + Sawtooth với resonance 0.7 và filter modulation mạnh. Attack nhanh cho phép chơi nhanh. Chế độ Mono + Glide cho portamento biểu cảm.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| OSC1 | Square 8' | Giàu bội âm lẻ |
| OSC2 | Sawtooth 4' | Sáng, quãng tám trên |
| Filter Resonance | 0.7 | Nhấn mạnh |
| Filter Mod | 0.6 | LFO→Filter |
| Glide Time | 0.1 | Trượt nhanh |

**Cách sử dụng:** Solo synth, rock lead. Bend note mạnh, sử dụng glide giữa các note xa.

---

### 3.6 Funky Clav
**File:** `06_funky_clav.json`  
**Thể loại:** Keys  

Clavinet funk kiểu Stevie Wonder — Square wave 8' với decay nhanh và sustain thấp. Keyboard tracking 2/3 cho âm sáng dần lên phím cao. Snappy và rhythmic.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| OSC1 | Square 8' | Clavinet-like |
| Amp Decay | 0.2 | Nhanh |
| Amp Sustain | 0.15 | Gần như không |
| Kbd Tracking | 0.67 | Sáng dần lên |
| Voice Mode | Poly | Hợp âm funk |

**Cách sử dụng:** Funk, disco. Chơi muted offbeat pattern, staccato 1/16.

---

### 3.7 Brass Stab
**File:** `07_brass_stab.json`  
**Thể loại:** Brass  

Brass section tổng hợp — 2 Sawtooth detuned với attack vừa phải (0.15) mô phỏng kèn thổi. Filter envelope 0.7 tạo "blat" đặc trưng. Chế độ Poly cho phép chơi hợp âm brass.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| OSC1/2 | Sawtooth 8' | Detuned nhẹ |
| Amp Attack | 0.15 | Vừa phải — không tức thì |
| Filter Env Amount | 0.7 | Sweep mở filter |
| Voice Mode | Poly | Hợp âm kèn |

**Cách sử dụng:** Disco, pop horn section, stab chords. Chơi hợp âm ngắn với nhấn mạnh.

---

### 3.8 Percussive Pluck
**File:** `08_percussive_pluck.json`  
**Thể loại:** Keys  

Âm pluck gõ rõ ràng — Sawtooth với decay cực ngắn, zero sustain. Filter envelope nhanh tạo transient sáng rồi tắt gọn. Rất phù hợp cho arpeggio và sequence.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| OSC1 | Sawtooth 8' | Harmonics phong phú |
| Amp Decay | 0.15 | Rất ngắn |
| Amp Sustain | 0.0 | Không sustain |
| Filter Env Fast | Yes | Transient sáng |
| Voice Mode | Poly | Arpeggio |

**Cách sử dụng:** Arpeggiator patterns, sequencer lines, pluck melodies.

---

### 3.9 Sub Bass
**File:** `09_sub_bass.json`  
**Thể loại:** Bass  

Sub bass cực thuần — Triangle 32' (quãng tám thấp nhất) không có resonance. Sine-like tone chỉ có fundamental, lý tưởng để layer dưới các âm khác. Chế độ Mono.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| OSC1 | Triangle 32' | Gần sine, rất sạch |
| Filter Resonance | 0.0 | Không resonance |
| Filter Cutoff | 0.25 | Cực thấp |
| Voice Mode | Mono | Một note |

**Cách sử dụng:** Layer dưới kick drum hoặc bass khác. Chơi root note, cực thấp.

---

### 3.10 Cosmic Sweep
**File:** `10_cosmic_sweep.json`  
**Thể loại:** FX  

Effect pad vũ trụ — OSC3 ở chế độ LFO modulate filter, 2 Sawtooth detuned tạo nền texture. Attack rất chậm (0.6) cho âm thanh "nổi" dần lên. Filter sweep tự động tạo chuyển động liên tục.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| OSC1/2 | Sawtooth detuned | Nền texture |
| OSC3 | LFO mode | Modulate filter |
| Filter Mod | 0.7 | Sweep lớn |
| Amp Attack | 0.6 | Rất chậm |
| Voice Mode | Poly | Hợp âm ambient |

**Cách sử dụng:** Ambient intro, film score, soundscape. Giữ hợp âm và để LFO sweep.

---

## 4. Hammond B-3 Engine Presets

Thư mục: `assets/hammond_presets/`  
Engine: **Hammond B-3** — 9 drawbar tonewheel organ với percussion, vibrato/chorus và Leslie speaker simulation.

### Hệ Thống Drawbar

Hammond B-3 sử dụng 9 drawbar tương ứng các bội âm harmonics:

| Drawbar | Footage | Interval | Ý Nghĩa |
|---------|---------|----------|----------|
| 1 | 16' | Sub-octave | Fundamental thấp |
| 2 | 5⅓' | 5th | Quãng 5 trên |
| 3 | 8' | Unison | Fundamental chính |
| 4 | 4' | 8va | Quãng tám trên |
| 5 | 2⅔' | 12th | Quãng 5 + 8va |
| 6 | 2' | 15th | 2 quãng tám |
| 7 | 1⅗' | 17th | Quãng 3 trưởng + 2 8va |
| 8 | 1⅓' | 19th | Quãng 5 + 2 8va |
| 9 | 1' | 22nd | 3 quãng tám |

Giá trị drawbar: **0** (tắt) → **8** (tối đa). Ký hiệu viết tắt: `88 8000 000` nghĩa là drawbar 1=8, 2=8, 3=8, còn lại=0.

---

### 4.1 Jimmy Smith Jazz
**File:** `01_jimmy_smith_jazz.json`  
**Thể loại:** Jazz  
**Drawbar:** `88 8000 000`

Âm jazz organ kinh điển của Jimmy Smith — chỉ dùng 3 drawbar đầu (16' + 5⅓' + 8') tạo nền ấm, đầy đặn nhưng không quá sáng. Percussion bật ở 2nd harmonic, tốc độ nhanh. Leslie chậm cho hiệu ứng xoay nhẹ.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Drawbars | 88 8000 000 | Ba drawbar đầu full |
| Percussion | ON, 2nd, Fast | Click đặc trưng |
| Vibrato/Chorus | C3 | Chorus phổ biến |
| Leslie | ON, Slow | Xoay nhẹ nhàng |
| Overdrive | 0.25 | Nhẹ |

**Cách sử dụng:** Jazz combo, walking bass accompaniment. Chơi hợp âm trái tay, solo phải tay.

---

### 4.2 Gospel Full
**File:** `02_gospel_full.json`  
**Thể loại:** Gospel  
**Drawbar:** `88 8888 888`

Full drawbar — tất cả 9 drawbar ở mức tối đa! Âm thanh cực kỳ đầy đặn, tất cả harmonics hiện diện. Leslie xoay nhanh kết hợp overdrive nhẹ tạo "shout" organ đặc trưng của nhạc Gospel.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Drawbars | 88 8888 888 | ALL drawbar max |
| Leslie | ON, Fast | Xoay nhanh |
| Overdrive | 0.3 | Nhẹ-vừa |

**Cách sử dụng:** Gospel, praise & worship. Kết hợp glissando, trills, và đổi Leslie slow/fast.

---

### 4.3 Rock Overdrive
**File:** `03_rock_overdrive.json`  
**Thể loại:** Rock  
**Drawbar:** `88 8800 000`

Jon Lord / Deep Purple style — drawbar chính + 4' với percussion 3rd harmonic tạo bite. Overdrive 0.6 cho crunch mạnh. Leslie nhanh cho hiệu ứng spinning speaker đặc trưng rock organ.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Drawbars | 88 8800 000 | Core + 4' |
| Percussion | 3rd, Fast | Bite mạnh hơn |
| Leslie | Fast | Spinning nhanh |
| Overdrive | 0.6 | Crunch mạnh |

**Cách sử dụng:** Classic rock, prog rock. Power chords, fast runs.

---

### 4.4 Soft Ballad
**File:** `04_soft_ballad.json`  
**Thể loại:** Ballad  
**Drawbar:** `00 8420 000`

Ballad nhẹ nhàng — không có 16' sub, chỉ từ 8' trở lên với mức giảm dần. Chorus C3 cho ấm áp, Leslie chậm. Không overdrive, không percussion — thanh thóat và sang trọng.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Drawbars | 00 8420 000 | Taper từ 8' |
| Vibrato/Chorus | C3 | Ấm áp |
| Leslie | Slow | Nhẹ nhàng |
| Overdrive | 0.0 | Sạch |

**Cách sử dụng:** Ballad, slow songs. Pad hợp âm dài, ít movement.

---

### 4.5 Booker T
**File:** `05_booker_t.json`  
**Thể loại:** Soul  
**Drawbar:** `88 6800 000`

Green Onions — Booker T. & the M.G.'s. Drawbar thiên về bass (16' + 5⅓' + 8' chủ đạo) với percussion 2nd chậm cho "lazy" feel. Leslie nhanh cho groove.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Drawbars | 88 6800 000 | Bass-heavy |
| Percussion | 2nd, Slow | Lazy feel |
| Leslie | Fast | Groove |

**Cách sử dụng:** Soul, R&B, 60s grooves. Riff-based playing, rhythmic patterns.

---

### 4.6 Cathedral Organ
**File:** `06_cathedral_organ.json`  
**Thể loại:** Classical  
**Drawbar:** `60 8080 006`

Pipe organ style — chỉ sử dụng fundamental (8') và octave (2') harmonics cách quãng, tạo cảm giác "organ nhà thờ". Vibrato V3 (wide, no chorus) cho tremulant effect. Không Leslie.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Drawbars | 60 8080 006 | Spaced harmonics |
| Vibrato | V3 | Tremulant nhà thờ |
| Leslie | OFF | Pipe organ không quay |

**Cách sử dụng:** Classical organ, Bach, hymns. Cần chơi legato, ít staccato.

---

### 4.7 Reggae Bubble
**File:** `07_reggae_bubble.json`  
**Thể loại:** Reggae  
**Drawbar:** `80 0800 000`

Reggae organ bubble — chỉ 16' suboctave + 8' fundamental. Âm đơn giản, tầm thấp, "bubbly". Chorus C1 nhẹ. Không Leslie, không overdrive.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Drawbars | 80 0800 000 | Chỉ sub + fund |
| Vibrato/Chorus | C1 | Rất nhẹ |
| Leslie | OFF | Sạch |

**Cách sử dụng:** Reggae, dub. Offbeat chords, "bubble" rhythm pattern.

---

### 4.8 Gospel Shout
**File:** `08_gospel_shout.json`  
**Thể loại:** Gospel  
**Drawbar:** `86 8868 568`

Gospel shout variation — drawbar gần full với nhấn mạnh harmonics lẻ. Percussion + Leslie nhanh + overdrive 0.4 tạo âm "shouting" mạnh mẽ hơn Gospel Full.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Drawbars | 86 8868 568 | Gần full, harmonics lẻ |
| Percussion | ON, Fast | Bite |
| Leslie | Fast | Mạnh mẽ |
| Overdrive | 0.4 | Vừa phải |

**Cách sử dụng:** Gospel shout, praise breaks. Chơi mạnh, trills, glissando.

---

### 4.9 Jazz Trio
**File:** `09_jazz_trio.json`  
**Thể loại:** Jazz  
**Drawbar:** `83 8000 000`

Jazz trio tinh tế — tương tự Jimmy Smith nhưng nhẹ hơn ở 5⅓'. Percussion 2nd cho click nhẹ. Leslie chậm. Âm thanh gọn gàng, phù hợp trio nhỏ.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Drawbars | 83 8000 000 | Nhẹ hơn Jimmy Smith |
| Percussion | 2nd, Slow | Click nhẹ |
| Leslie | Slow | Tinh tế |

**Cách sử dụng:** Jazz trio, supper club. Comping vừa phải, solo nhẹ nhàng.

---

### 4.10 Progressive Rock
**File:** `10_progressive_rock.json`  
**Thể loại:** Rock  
**Drawbar:** `88 8808 008`

Keith Emerson / Rick Wakeman style — drawbar thiên về harmonics cao với gap tạo character riêng. Leslie nhanh, overdrive 0.5 cho crunch prog rock.

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Drawbars | 88 8808 008 | Gap ở 2⅔' và 1⅓' |
| Leslie | Fast | Spinning mạnh |
| Overdrive | 0.5 | Crunch vừa |

**Cách sử dụng:** Prog rock, art rock. Fast arpeggios, big chords, theatrical playing.

---

## 5. Rhodes Mark I Engine Presets

Thư mục: `assets/rhodes_presets/`  
Engine: **Rhodes Mark I** — Electric piano 2-op FM tine model với tremolo, vibrato và drive.

### Tham Số Rhodes

| Tham số | ID | Range | Mô tả |
|---------|---:|-------|-------|
| Decay | 0 | 0–1 | Thời gian tắt dần |
| Tone | 1 | 0–1 | Sáng ← → tối |
| Drive | 2 | 0–1 | Amp overdrive |
| Velocity Sens | 3 | 0–1 | Độ nhạy velocity |
| Mod Ratio | 4 | 0–1 | FM modulator ratio |
| Trem Rate | 5 | 0–10 Hz | Tốc độ tremolo |
| Trem Depth | 6 | 0–1 | Độ sâu tremolo |
| Vib Rate | 7 | 0–12 Hz | Tốc độ vibrato |
| Vib Depth | 8 | 0–1 | Độ sâu vibrato |
| Stereo Spread | 9 | 0–1 | Độ rộng stereo |
| Volume | 10 | 0–1 | Âm lượng |

---

### 5.1 Classic EP
**File:** `01_classic_ep.json`  
**Thể loại:** Keys  

Rhodes chuẩn mực — cân bằng hoàn hảo giữa ấm áp và sáng sủa. Decay vừa phải, tone trung tính, drive nhẹ. Đây là xuất phát điểm tốt nhất.

| Tham số chính | Giá trị |
|---------------|---------|
| Decay | 0.45 |
| Tone | 0.5 |
| Drive | 0.25 |
| Velocity | 0.7 |

**Cách sử dụng:** Mọi thể loại. Baseline sound trước khi tweak.

---

### 5.2 Suitcase Tremolo
**File:** `02_suitcase_tremolo.json`  
**Thể loại:** Keys  

Fender Rhodes Suitcase model — tremolo 4.8 Hz với depth 0.55, drive nuance 0.3. Đây là âm tremolo đặc trưng nhất của Rhodes thập niên 70.

| Tham số chính | Giá trị |
|---------------|---------|
| Trem Rate | 4.8 Hz |
| Trem Depth | 0.55 |
| Drive | 0.3 |
| Stereo | 0.2 |

**Cách sử dụng:** R&B, soul, jazz fusion. Pad hợp âm, để tremolo làm phần còn lại.

---

### 5.3 Dyno Bright
**File:** `03_dyno_bright.json`  
**Thể loại:** Keys  

Dyno-My Rhodes modification — tone rất sáng (0.85), drive cao (0.45), decay ngắn hơn. Âm "bell-like" đặc trưng của 80s EP.

| Tham số chính | Giá trị |
|---------------|---------|
| Tone | 0.85 |
| Drive | 0.45 |
| Decay | 0.35 |

**Cách sử dụng:** Pop, 80s ballad. Melody lines, bright comping.

---

### 5.4 Jazz Ballad
**File:** `04_jazz_ballad.json`  
**Thể loại:** Jazz  

Ấm áp, mềm mại — tone thấp (0.3), decay dài (0.65), vibrato nhẹ 4.5 Hz. Không tremolo, drive rất nhẹ. Âm thanh thân mật cho jazz ballad.

| Tham số chính | Giá trị |
|---------------|---------|
| Tone | 0.3 |
| Decay | 0.65 |
| Vib Rate | 4.5 Hz |
| Vib Depth | 0.15 |

**Cách sử dụng:** Jazz ballad, late night lounge. Voicings rộng, legato.

---

### 5.5 Funk Rhodes
**File:** `05_funk_rhodes.json`  
**Thể loại:** Funk  

Funk tight — decay ngắn (0.25), velocity sensitivity cao (0.9) cho dynamic playing. Tremolo nhẹ cho groove. Chơi mạnh = sáng, nhẹ = ấm.

| Tham số chính | Giá trị |
|---------------|---------|
| Decay | 0.25 |
| Velocity | 0.9 |
| Tone | 0.65 |
| Trem Rate | 3 Hz |

**Cách sử dụng:** Funk, R&B grooves. Muted offbeat, ghost notes, dynamic control.

---

### 5.6 Glass Keys
**File:** `06_glass_keys.json`  
**Thể loại:** Keys  

Pha lê sáng — tone cực cao (0.9), KHÔNG drive, decay ngắn. Âm thanh sạch, crystal clear, tine đặc trưng rõ ràng nhất.

| Tham số chính | Giá trị |
|---------------|---------|
| Tone | 0.9 |
| Drive | 0.0 |
| Decay | 0.2 |

**Cách sử dụng:** Modern pop, chill music. Single note melodies, clean arpeggios.

---

### 5.7 Lo-Fi Keys
**File:** `07_lo_fi_keys.json`  
**Thể loại:** Keys  

Lo-fi vibe — drive rất cao (0.7) tạo warm distortion, tremolo chậm (3.5 Hz). Cảm giác vintage cassette.

| Tham số chính | Giá trị |
|---------------|---------|
| Drive | 0.7 |
| Trem Rate | 3.5 Hz |
| Trem Depth | 0.35 |
| Tone | 0.45 |

**Cách sử dụng:** Lo-fi hip hop, chillhop beats. Pad hợp âm lazy.

---

### 5.8 Wurlitzer Style
**File:** `08_wurlitzer_style.json`  
**Thể loại:** Keys  

Mô phỏng Wurlitzer — tone sáng (0.8), drive mạnh (0.65), tremolo nhanh (6.5 Hz). Âm "nasal" đặc trưng Wurly.

| Tham số chính | Giá trị |
|---------------|---------|
| Tone | 0.8 |
| Drive | 0.65 |
| Trem Rate | 6.5 Hz |
| Trem Depth | 0.5 |

**Cách sử dụng:** Pop rock, 60s-70s vibe. Ray Charles, Supertramp style.

---

### 5.9 Dream Pad
**File:** `09_dream_pad.json`  
**Thể loại:** Pad  

Pad mơ màng — decay rất dài (0.8), release 1200ms, vibrato sâu (0.4). Âm thanh lơ lửng, ethereal.

| Tham số chính | Giá trị |
|---------------|---------|
| Decay | 0.8 |
| Release | 1200ms |
| Vib Depth | 0.4 |
| Stereo | 0.25 |

**Cách sử dụng:** Ambient, dream pop. Sustain hợp âm lâu, layer với reverb.

---

### 5.10 Neo Soul
**File:** `10_neo_soul.json`  
**Thể loại:** Keys  

Neo soul đương đại — tone ấm (0.4), drive vừa (0.35), tremolo nhẹ (4.2 Hz). D'Angelo, Erykah Badu feel.

| Tham số chính | Giá trị |
|---------------|---------|
| Tone | 0.4 |
| Drive | 0.35 |
| Trem Rate | 4.2 Hz |
| Trem Depth | 0.3 |

**Cách sử dụng:** Neo soul, contemporary R&B. Jazzy voicings, ninth chords.

---

## 6. DX7 Engine Presets

Thư mục: `assets/dx7_presets/`  
Engine: **Yamaha DX7** — 6-operator FM synthesis với 32 algorithms.

### Giới Thiệu FM Synthesis

DX7 sử dụng Frequency Modulation: **Operators** (sine oscillators) modulate lẫn nhau. **Carriers** tạo ra âm thanh nghe được, **Modulators** thay đổi timber. **Algorithm** quy định cách kết nối 6 operators.

| Tham số quan trọng | Mô tả |
|---------------------|-------|
| Algorithm (ID 0) | 1–32, cách nối 6 Op |
| Op Ratio (ID 1,6,11,16,21,26) | Tỷ lệ tần số, tạo harmonics |
| Op Attack/Decay/Sustain/Release | ADSR cho mỗi operator |
| Op Level (ID 31–36) | Mức output từng Op |
| Keyboard Rate Scaling (ID 55–60) | Decay nhanh hơn ở note cao |
| Feedback (ID 61) | Self-modulation op cuối |
| Master Volume (ID 62) | Âm lượng tổng |

---

### 6.1 E.Piano 1
**File:** `01_electric_piano.json`  
**Thể loại:** Keys  
**Algorithm:** 5  

DX7 E.Piano #1 — preset nổi tiếng nhất trong lịch sử synth! Algorithm 5 (2 carrier-modulator pairs). Carriers ở ratio 1.0 (fundamental), modulators ở ratio 3.5 và 5.0 tạo tine characteristic. KbdRate scaling 0.3 giúp note cao decay nhanh hơn (tự nhiên hơn).

| Tham số chính | Giá trị | Ghi chú |
|---------------|---------|---------|
| Algorithm | 5 | 2 carrier pairs |
| Op1 Ratio / Op3 Ratio | 1.0 / 3.5 | Carrier / Mod |
| Op2 Ratio / Op4 Ratio | 1.0 / 5.0 | Carrier / Mod |
| Feedback | 0.08 | Nhẹ |
| KbdRate Op1/2 | 0.3 | Scaling tự nhiên |

**Cách sử dụng:** Mọi thể loại 80s. Ballade, pop, jazz fusion.

---

### 6.2 DX Bass
**File:** `02_dx_bass.json`  
**Thể loại:** Bass  
**Algorithm:** 1  

FM bass kinh điển — Algorithm 1 (serial stack toàn bộ). Decay cực ngắn, feedback 0.15 cho bite. Một trong những âm bass digital đầu tiên thay thế bass analog.

| Tham số chính | Giá trị |
|---------------|---------|
| Algorithm | 1 |
| Op1 Ratio | 1.0 |
| Feedback | 0.15 |
| Amp Decay | Nhanh |

**Cách sử dụng:** Funk, R&B, pop bass lines. Slap-style playing.

---

### 6.3 Brass Section
**File:** `03_brass_stab.json`  
**Thể loại:** Brass  
**Algorithm:** 22  

FM brass — Algorithm 22 (3 carriers) tạo harmonics phong phú. Attack chậm hơn (0.12) mô phỏng kèn thổi. KbdRate 0.2 cho brightness scaling tự nhiên.

| Tham số chính | Giá trị |
|---------------|---------|
| Algorithm | 22 |
| Carriers | 3 (Op1/2/3) |
| Attack | 0.12 |
| KbdRate | 0.2 |

**Cách sử dụng:** Pop horn section, disco brass stabs.

---

### 6.4 Wooden Marimba
**File:** `04_marimba.json`  
**Thể loại:** Percussion  
**Algorithm:** 5  

Marimba gỗ — modulator ratio 4.0 tạo harmonics "struck wood". Decay nhanh ở modulator, carrier duy trì fundamental. KbdRate 0.4 làm note cao tắt nhanh (tự nhiên nhất).

| Tham số chính | Giá trị |
|---------------|---------|
| Algorithm | 5 |
| Mod Ratio | 4.0 |
| KbdRate | 0.4 |
| Carrier Decay | Vừa |

**Cách sử dụng:** Latin, jazz, world music. Melodic percussion, arpeggio patterns.

---

### 6.5 Strings Pad
**File:** `05_strings.json`  
**Thể loại:** Pad  
**Algorithm:** 4  

FM strings — Algorithm 4 (parallel carriers) với attack chậm (0.35) và sustain dài. Operators detuned nhẹ tạo hiệu ứng ensemble. KbdRate 0.15.

| Tham số chính | Giá trị |
|---------------|---------|
| Algorithm | 4 |
| Attack | 0.35 |
| Sustain | Cao |
| Detuned | Nhẹ |

**Cách sử dụng:** Orchestral pads, film score, bed tracks. Hợp âm dài.

---

### 6.6 Tubular Bells
**File:** `06_tubular_bells.json`  
**Thể loại:** Percussion  
**Algorithm:** 1  

Mike Oldfield style — Algorithm 1 serial, modulator ratio 3.5 tạo inharmonic partials đặc trưng của bell. Feedback 0.2 thêm metallic edge. Decay tự nhiên dài. KbdRate 0.3.

| Tham số chính | Giá trị |
|---------------|---------|
| Algorithm | 1 |
| Mod Ratio | 3.5 |
| Feedback | 0.2 |
| KbdRate | 0.3 |

**Cách sử dụng:** Intro, ambient. Single notes hoặc slow melody.

---

### 6.7 DX Organ
**File:** `07_organ.json`  
**Thể loại:** Keys  
**Algorithm:** 4  

FM organ — Algorithm 4 parallel, ratios nguyên (1:2:3) tạo harmonics organ. Sustain cao, attack tức thì. Ít feedback cho clean tone.

| Tham số chính | Giá trị |
|---------------|---------|
| Algorithm | 4 |
| Op Ratios | 1:2:3 (integer) |
| Feedback | 0.05 |
| Sustain | Cao |

**Cách sử dụng:** Pop organ, church organ simulation. Hợp âm dài, melody.

---

### 6.8 Synth Lead
**File:** `08_synth_lead.json`  
**Thể loại:** Lead  
**Algorithm:** 1  

FM lead xé — Algorithm 1 serial cho modulation sâu. Detuned nhẹ (1.01) tạo "phasing". Feedback cao (0.35) cho rich harmonics. Screaming synth lead.

| Tham số chính | Giá trị |
|---------------|---------|
| Algorithm | 1 |
| Mod Ratio | 1.01 |
| Feedback | 0.35 |
| Attack | Nhanh |

**Cách sử dụng:** Synth solo, lead lines. Bend và vibrato mạnh.

---

### 6.9 Vibraphone
**File:** `09_vibraphone.json`  
**Thể loại:** Percussion  
**Algorithm:** 5  

Vibraphone — tương tự Marimba nhưng mellow hơn, sustain dài hơn. Modulator ratio 4.0 ở mức thấp hơn. KbdRate 0.25 cho tự nhiên.

| Tham số chính | Giá trị |
|---------------|---------|
| Algorithm | 5 |
| Mod Ratio | 4.0 |
| Mod Level | Thấp |
| KbdRate | 0.25 |

**Cách sử dụng:** Jazz, chill music. Gentle melody, soft touch.

---

### 6.10 Harmonica
**File:** `10_harmonica.json`  
**Thể loại:** Lead  
**Algorithm:** 1  

Harmonica — Algorithm 1, mod ratio 3 tạo "reedy" tone, feedback 0.25 cho breath-like noise. Attack nhanh, sustain tốt.

| Tham số chính | Giá trị |
|---------------|---------|
| Algorithm | 1 |
| Mod Ratio | 3.0 |
| Feedback | 0.25 |

**Cách sử dụng:** Blues, folk. Melody lines, bending notes.

---

## 7. Mellotron M400 Engine Presets

Thư mục: `assets/mellotron_presets/`  
Engine: **Mellotron M400** — Tape replay keyboard với 4 loại tape, wow/flutter simulation.

### Hệ Thống Tape

| Tape ID | Tên | Mô tả |
|---------|-----|-------|
| 0 | Strings | Dàn dây cello/violin |
| 1 | Choir | Giọng hợp xướng |
| 2 | Flute | Sáo, recorder |
| 3 | Brass | Kèn đồng |

### Tham Số Mellotron

| Tham số | ID | Range | Mô tả |
|---------|---:|-------|-------|
| Tape Select | 0 | 0–3 | Chọn loại tape |
| Attack | 1 | 0–1 | Thời gian bắt đầu |
| Release | 2 | 0–1 | Thời gian tắt |
| Wow Depth | 3 | 0–1 | Pitch wobble chậm |
| Wow Rate | 4 | 0–1 | Tốc độ wow |
| Flutter Depth | 5 | 0–1 | Pitch flutter nhanh |
| Tape Speed | 6 | 0.8–1.2 | Tốc độ chạy tape |
| Tape Degradation | 7 | 0–1 | Độ xuống cấp |
| Pitch Spread | 8 | 0–1 | Detuning giữa voice |
| Noise Level | 9 | 0–1 | Tape hiss |
| Volume | 10 | 0–1 | Âm lượng |

---

### 7.1 Orchestral Strings
**File:** `01_orchestral_strings.json`  
**Thể loại:** Strings  
**Tape:** Strings (0)

Strings kinh điển của Mellotron — âm dàn dây cello/violin đặc trưng Beatles, King Crimson. Wow/flutter ở mức mặc định tạo cảm giác vintage nhưng không quá nhiều.

| Tham số chính | Giá trị |
|---------------|---------|
| Tape | Strings |
| Wow Depth | 0.2 |
| Flutter | 0.15 |
| Tape Speed | 1.0 |

**Cách sử dụng:** Progressive rock, art rock, orchestral parts. Pad hợp âm, melody lines.

---

### 7.2 Angelic Choir
**File:** `02_angelic_choir.json`  
**Thể loại:** Pad  
**Tape:** Choir (1)

Choir "thiên thần" — attack chậm (0.25), release dài (0.45). Âm choir xuất hiện từ từ và tan dần. Phù hợp cho ambient và film score.

| Tham số chính | Giá trị |
|---------------|---------|
| Tape | Choir |
| Attack | 0.25 |
| Release | 0.45 |

**Cách sử dụng:** Ambient, film score, church music. Hợp âm dài, sustained chords.

---

### 7.3 Solo Flute
**File:** `03_solo_flute.json`  
**Thể loại:** Lead  
**Tape:** Flute (2)

Sáo solo rõ ràng — attack nhanh, release gọn, wow/flutter tối thiểu. Clean và clear cho melodic use.

| Tham số chính | Giá trị |
|---------------|---------|
| Tape | Flute |
| Attack | 0.03 |
| Release | 0.15 |
| Wow | 0.08 |

**Cách sử dụng:** Melodies, folk, baroque-influenced parts. Single note lines.

---

### 7.4 Power Brass
**File:** `04_power_brass.json`  
**Thể loại:** Brass  
**Tape:** Brass (3)

Kèn mạnh mẽ — attack nhanh, volume cao (0.9). Brass Mellotron kinh điển kiểu Genesis.

| Tham số chính | Giá trị |
|---------------|---------|
| Tape | Brass |
| Attack | 0.05 |
| Volume | 0.9 |

**Cách sử dụng:** Fanfare, stabs, prog rock intros. Powerful chord stabs.

---

### 7.5 Lo-Fi Strings
**File:** `05_lo_fi_strings.json`  
**Thể loại:** Strings  
**Tape:** Strings (0)

Strings xuống cấp nặng — wow 0.6, flutter 0.5, tape speed nhanh (1.1). Như chiếc Mellotron đã bị bỏ quên nhiều năm. Pitch wobble mạnh tạo cảm giác lo-fi tuyệt vời.

| Tham số chính | Giá trị |
|---------------|---------|
| Wow | 0.6 |
| Flutter | 0.5 |
| Tape Speed | 1.1 |
| Degradation | 0.6 |

**Cách sử dụng:** Lo-fi, indie, experimental. Texture layer, atmosphere.

---

### 7.6 Ethereal Choir
**File:** `06_ethereal_choir.json`  
**Thể loại:** Pad  
**Tape:** Choir (1)

Choir siêu thực — attack rất chậm (0.35), wow sâu (0.35), pitch spread (0.2). Âm choir không rõ ràng mà mơ hồ, lơ lửng.

| Tham số chính | Giá trị |
|---------------|---------|
| Tape | Choir |
| Attack | 0.35 |
| Wow | 0.35 |
| Spread | 0.2 |

**Cách sử dụng:** Ambient, dream pop, film score. Background texture, slowly evolving.

---

### 7.7 Vintage Flute
**File:** `07_vintage_flute.json`  
**Thể loại:** Lead  
**Tape:** Flute (2)

Sáo vintage — wow vừa phải (0.25), tape speed chậm hơn (0.95). Cảm giác "cũ" hơn Solo Flute, charming imperfections.

| Tham số chính | Giá trị |
|---------------|---------|
| Tape | Flute |
| Wow | 0.25 |
| Tape Speed | 0.95 |

**Cách sử dụng:** Vintage pop, 70s prog, nostalgic melodies.

---

### 7.8 Dark Strings
**File:** `08_dark_strings.json`  
**Thể loại:** Strings  
**Tape:** Strings (0)

Strings tối — tape speed chậm (0.85) hạ pitch và tạo tonal character tối hơn. Flutter 0.3 thêm instability.

| Tham số chính | Giá trị |
|---------------|---------|
| Tape | Strings |
| Tape Speed | 0.85 |
| Flutter | 0.3 |
| Degradation | 0.5 |

**Cách sử dụng:** Horror, dark ambient, suspense. Slow evolving dark textures.

---

### 7.9 Mellow Brass
**File:** `09_mellow_brass.json`  
**Thể loại:** Brass  
**Tape:** Brass (3)

Kèn dịu — attack chậm hơn (0.15), wow nhẹ. Brass mềm mại hơn Power Brass, phù hợp để blend.

| Tham số chính | Giá trị |
|---------------|---------|
| Tape | Brass |
| Attack | 0.15 |
| Wow | 0.12 |

**Cách sử dụng:** Orchestral blends, soft brass sections, cinematic.

---

### 7.10 Degraded Tape
**File:** `10_degraded_tape.json`  
**Thể loại:** FX  
**Tape:** Strings (0)

Tape hư hỏng cực độ — wow 0.8, flutter 0.7, tape speed 1.15, tất cả degradation lên max. Effect/texture preset, không phải instrument preset.

| Tham số chính | Giá trị |
|---------------|---------|
| Wow | 0.8 |
| Flutter | 0.7 |
| Tape Speed | 1.15 |
| Degradation | 0.8 |

**Cách sử dụng:** Experimental, noise, texture. Layer underneath other instruments.

---

## 8. Global Presets — Moog

Global presets bao gồm **engine parameters + effect chain**, sẵn sàng sử dụng.

### 8.1 Arena Lead
**File:** `global_presets/01_moog_arena_lead.json`  
**Thể loại:** Lead  
**Engine:** MiniMoog Model D  
**Effects:** Delay (0.35s, 35% feedback) → Reverb (hall, 55% mix) → EQ (bright +3dB highs)

Lead synth cho sân khấu lớn. Square + Sawtooth tạo lead xuyên thấu, delay tạo depth, reverb tạo không gian, EQ boost high để cắt qua mix.

**Cách sử dụng:** Solo synth sân khấu, arena rock. Nổi bật trong mix lớn.

---

### 8.2 Deep Sub
**File:** `global_presets/02_moog_deep_sub.json`  
**Thể loại:** Bass  
**Engine:** MiniMoog Model D  
**Effects:** EQ (+8dB bass) → Reverb (light, 15% mix)

Sub bass cực sâu với EQ tăng cường low-end. Reverb rất nhẹ chỉ để thêm chút không gian, tránh làm bẩn sub.

**Cách sử dụng:** EDM, hip hop. Layer dưới kick, chơi root note.

---

### 8.3 Acid House
**File:** `global_presets/03_moog_acid_house.json`  
**Thể loại:** Bass  
**Engine:** MiniMoog Model D  
**Effects:** Flanger (nhẹ) → Delay (BPM sync 1/16)

Acid bass với flanger tạo metallic edge, delay BPM sync tạo rhythmic repeats. Resonance cao + filter envelope = classic squelch.

**Cách sử dụng:** Acid house, acid techno. Sequencer 1/16, vary accent.

---

### 8.4 Ambient Pad
**File:** `global_presets/04_moog_ambient_pad.json`  
**Thể loại:** Pad  
**Engine:** MiniMoog Model D  
**Effects:** Chorus (3 voices) → Reverb (big, 85% size) → Tremolo (nhẹ)

Pad mênh mông — chorus widening, reverb lớn tạo không gian vô tận, tremolo nhẹ thêm movement.

**Cách sử dụng:** Ambient, chillout, meditation music. Sustain hợp âm.

---

### 8.5 Retro Funk
**File:** `global_presets/05_moog_retro_funk.json`  
**Thể loại:** Lead  
**Engine:** MiniMoog Model D  
**Effects:** Phaser (0.8 Hz) → Gain (mild overdrive)

Funk retro 70s — phaser chậm tạo sweep, gain overdrive nhẹ cho warmth. Clavinet-like tone.

**Cách sử dụng:** Funk, disco. Wah-like phaser effect, rhythmic playing.

---

### 8.6 Dark Drone
**File:** `global_presets/06_moog_dark_drone.json`  
**Thể loại:** FX  
**Engine:** MiniMoog Model D  
**Effects:** Reverb (95% size, huge decay) → Delay (0.8s, 70% feedback) → EQ (dark, cut highs)

Drone tối — reverb khổng lồ + delay feedback gần infinite tạo sustain vô tận. EQ cắt highs cho dark character. Cosmic sweep engine tạo slow movement.

**Cách sử dụng:** Dark ambient, drone music, horror score. Giữ 1-2 note, để effects làm phần còn lại.

---

### 8.7 80s Poly Synth
**File:** `global_presets/07_moog_80s_poly.json`  
**Thể loại:** Keys  
**Engine:** MiniMoog Model D  
**Effects:** Chorus (2 voices) → Delay (300ms, 30%) → Reverb (medium 40%)

Synth poly kiểu Juno-60 / Prophet-5 — chorus widening, delay+reverb cho depth. Saw+Square poly pad.

**Cách sử dụng:** 80s pop, synthwave, retrowave. Pad hợp âm, melody lines.

---

## 9. Global Presets — Hammond B-3

### 9.1 Gospel Live
**File:** `global_presets/08_hammond_gospel_live.json`  
**Thể loại:** Keys  
**Engine:** Hammond B-3  
**Effects:** Gain (mild overdrive 1.6x) → Reverb (medium hall, 30% mix)

Gospel organ biểu diễn live — overdrive ấm + reverb phòng. Full drawbar + Leslie fast + percussion.

**Cách sử dụng:** Gospel concert, praise band. Dynamic playing, Leslie switch.

---

### 9.2 Jazz Club
**File:** `global_presets/09_hammond_jazz_club.json`  
**Thể loại:** Keys  
**Engine:** Hammond B-3  
**Effects:** Reverb (small room, 25% mix) → EQ (warm, cut highs)

Jazz club intimate — reverb phòng nhỏ, EQ ấm cắt sáng. Jimmy Smith drawbar 88 3000 000 + Leslie chậm.

**Cách sử dụng:** Jazz club gig, intimate setting. Soft playing, comping.

---

### 9.3 Progressive Rock
**File:** `global_presets/10_hammond_prog_rock.json`  
**Thể loại:** Keys  
**Engine:** Hammond B-3  
**Effects:** Gain (heavy distortion 2.5x) → Phaser (slow) → Delay (350ms, 30%)

Prog rock epic — distortion mạnh + phaser + delay cho sound đầy năng lượng kiểu Emerson.

**Cách sử dụng:** Prog rock solos, epic passages. Fast runs, power chords.

---

### 9.4 Soul Groove
**File:** `global_presets/11_hammond_soul_groove.json`  
**Thể loại:** Keys  
**Engine:** Hammond B-3  
**Effects:** Tremolo (5 Hz) → Chorus (2 voices) → Reverb (room, 25% mix)

Soul groove mượt — tremolo tạo pulse, chorus widening, reverb room nhẹ. Drawbar thiên percussion.

**Cách sử dụng:** Soul, R&B grooves. Rhythmic comping, groove-based.

---

### 9.5 Church Organ
**File:** `global_presets/12_hammond_church.json`  
**Thể loại:** Keys  
**Engine:** Hammond B-3  
**Effects:** Reverb (cathedral, 90% size, 45% mix) → EQ (warm)

Nhà thờ trang nghiêm — reverb cathedral cực lớn tạo không gian thánh đường. Cathedral drawbar + vibrato.

**Cách sử dụng:** Church service, hymns, classical. Legato playing, large reverb tail.

---

## 10. Global Presets — Rhodes Mark I

### 10.1 Neo Soul Keys
**File:** `global_presets/13_rhodes_neo_soul.json`  
**Thể loại:** Keys  
**Engine:** Rhodes Mark I  
**Effects:** Chorus (2 voices, 30% mix) → Reverb (plate, 25% mix)

Neo soul Rhodes đương đại — chorus nhẹ widening, reverb plate subtle. Warm tone + drive nhẹ.

**Cách sử dụng:** Neo soul, contemporary R&B. Jazzy voicings, ninth chords.

---

### 10.2 Jazz Lounge
**File:** `global_presets/14_rhodes_jazz_lounge.json`  
**Thể loại:** Keys  
**Engine:** Rhodes Mark I  
**Effects:** Reverb (room, 20% mix) → EQ (warm, cut highs)

Jazz lounge sang trọng — reverb room nhỏ, EQ ấm. Tone thấp, vibrato nhẹ. Intimate và sophisticated.

**Cách sử dụng:** Jazz lounge, cocktail piano. Gentle touch, wide voicings.

---

### 10.3 Funk Wah
**File:** `global_presets/15_rhodes_funk_wah.json`  
**Thể loại:** Keys  
**Engine:** Rhodes Mark I  
**Effects:** Phaser (1.2 Hz, deep) → Tremolo (4.8 Hz)

Funk wah — phaser sâu tạo wah effect, tremolo thêm pulse. Bright tone, high velocity.

**Cách sử dụng:** Funk, dance. Rhythmic playing, staccato chords, wah feel.

---

### 10.4 Chill Wave
**File:** `global_presets/16_rhodes_chill_wave.json`  
**Thể loại:** Pad  
**Engine:** Rhodes Mark I  
**Effects:** Chorus (3 voices, 40% mix) → Delay (450ms, 35%) → Reverb (large, 40% mix)

Chill wave dreamy — chorus rộng, delay+reverb tạo space lớn. Long decay, vibrato. Lo-fi dream vibe.

**Cách sử dụng:** Chill wave, dream pop, lo-fi. Sustain chords, let effects bloom.

---

### 10.5 Vintage Amp
**File:** `global_presets/17_rhodes_vintage_amp.json`  
**Thể loại:** Keys  
**Engine:** Rhodes Mark I  
**Effects:** Gain (drive 1.8x) → Tremolo (4.5 Hz) → Reverb (spring, 20% mix)

Vintage amp 60s — gain drive ấm, tremolo + reverb spring kiểu Twin Reverb amp.

**Cách sử dụng:** 60s-70s classic. Surf, oldies, vintage pop.

---

## 11. Global Presets — Yamaha DX7

### 11.1 80s Electric Piano
**File:** `global_presets/18_dx7_80s_ep.json`  
**Thể loại:** Keys  
**Engine:** Yamaha DX7  
**Effects:** Chorus (2 voices, 30% mix) → Reverb (medium, 25% mix)

DX7 EP kiểu 80s với chorus classic + reverb. Preset bắt buộc của thập niên 80.

**Cách sử dụng:** 80s pop, ballad, Whitney Houston tracks.

---

### 11.2 Crystal Bells
**File:** `global_presets/19_dx7_crystal_bells.json`  
**Thể loại:** FX  
**Engine:** Yamaha DX7  
**Effects:** Delay (400ms, 35%) → Reverb (shimmer, 80% size, 45% mix)

Chuông pha lê — inharmonic FM bells + delay tạo rhythmic repeats + reverb shimmer. Lấp lánh.

**Cách sử dụng:** Intro, ambient section, film score. Sparse single notes.

---

### 11.3 Synth Brass
**File:** `global_presets/20_dx7_synth_brass.json`  
**Thể loại:** Brass  
**Engine:** Yamaha DX7  
**Effects:** Chorus (2 voices, 25%) → Delay (300ms, 25%)

FM brass tổng hợp — 3-carrier algo 22 + chorus widening + delay. Full, thick brass.

**Cách sử dụng:** Pop horn section, synth brass stabs, big chords.

---

### 11.4 Ambient Bell
**File:** `global_presets/21_dx7_ambient_bell.json`  
**Thể loại:** FX  
**Engine:** Yamaha DX7  
**Effects:** Reverb (cathedral, 90% size, 50% mix) → Delay (700ms, 40%)

Bell ambient — reverb cathedral cực lớn + delay dài tạo sustain infinite. Inharmonic bell tones float trong không gian.

**Cách sử dụng:** Meditation, ambient, soundscape. Single notes, let them ring.

---

### 11.5 Bass Stab
**File:** `global_presets/22_dx7_bass_stab.json`  
**Thể loại:** Bass  
**Engine:** Yamaha DX7  
**Effects:** Gain (clean boost 1.3x) → EQ (+6dB bass, cut mids/highs)

FM bass stab sắc nét — gain boost nhẹ + EQ bass emphasis. Digital bass punch.

**Cách sử dụng:** Funk, pop bass lines. Staccato, punchy.

---

### 11.6 FM Organ
**File:** `global_presets/23_dx7_fm_organ.json`  
**Thể loại:** Keys  
**Engine:** Yamaha DX7  
**Effects:** Chorus (slow, 25%) → Reverb (room, 20% mix)

FM organ ấm áp — parallel carriers tạo organ harmonics, chorus cho Leslie-like effect, reverb room nhẹ.

**Cách sử dụng:** Pop organ, worship. Hợp âm pad, melody support.

---

## 12. Global Presets — Mellotron M400

### 12.1 Strawberry Fields
**File:** `global_presets/24_mellotron_strawberry.json`  
**Thể loại:** Pad  
**Engine:** Mellotron M400  
**Effects:** Flanger (slow) → Reverb (hall, 35% mix)

Kiểu Beatles "Strawberry Fields Forever" 1967 — flute tape + flanger tạo psychedelic vibe + reverb hall.

**Cách sử dụng:** Psychedelic rock, 60s tribute. Melody lines, atmospheric.

---

### 12.2 Space Choir
**File:** `global_presets/25_mellotron_space_choir.json`  
**Thể loại:** Pad  
**Engine:** Mellotron M400  
**Effects:** Delay (550ms, 35%) → Reverb (cathedral, 50% mix) → Chorus (3 voices)

Choir vũ trụ — delay+reverb tạo không gian vô tận, chorus widening. Choir tape với attack chậm.

**Cách sử dụng:** Space ambient, sci-fi score, ethereal pads. Long sustained chords.

---

### 12.3 Vintage Orchestra
**File:** `global_presets/26_mellotron_vintage_orch.json`  
**Thể loại:** Pad  
**Engine:** Mellotron M400  
**Effects:** EQ (warm, +2dB bass, cut highs) → Reverb (plate, 30% mix)

Dàn nhạc vintage — strings tape + EQ ấm + reverb plate. Authentic vintage orchestral feel.

**Cách sử dụng:** Orchestral parts, vintage pop, 70s prog. String sections.

---

### 12.4 Dark Ambient
**File:** `global_presets/27_mellotron_dark_ambient.json`  
**Thể loại:** FX  
**Engine:** Mellotron M400  
**Effects:** Reverb (95% size, 55% mix) → Delay (800ms, 40%) → EQ (dark, cut everything)

Ambient tối — reverb gần infinite + delay dài + EQ cắt mọi thứ sáng. Strings tape với wow/flutter mạnh. Không gian đen tối.

**Cách sử dụng:** Dark ambient, horror, drone. Hold single notes, let effects create texture.

---

### 12.5 Brass Section
**File:** `global_presets/28_mellotron_brass_section.json`  
**Thể loại:** Brass  
**Engine:** Mellotron M400  
**Effects:** Gain (clean boost 1.3x) → Reverb (room, 20% mix)

Dàn kèn vintage — brass tape + gain boost + reverb room. Powerful nhưng vintage character.

**Cách sử dụng:** Fanfare, prog rock brass, vintage orchestral. Chord stabs.

---

## 13. Global Presets — Drum Machine

Engine: **Hybrid Drum Machine** — 8 DSP pads (Kick, Snare, HH Closed, HH Open, Clap, Tom Low, Tom Mid, Rimshot) + 8 Sample pads.

### DSP Pad Layout

| Pad | Tên | Mô tả |
|-----|-----|-------|
| 0 | Kick | Bass drum |
| 1 | Snare | Snare drum |
| 2 | HH Closed | Hi-hat đóng |
| 3 | HH Open | Hi-hat mở |
| 4 | Clap | Vỗ tay |
| 5 | Tom Low | Tom trầm |
| 6 | Tom Mid | Tom trung |
| 7 | Rimshot | Rimshot |

### Tham Số Per-Pad

| Tham số | Offset | Range | Mô tả |
|---------|--------|-------|-------|
| Volume | +0 | 0–1 | Âm lượng pad |
| Pitch | +1 | 0–1 | Tần số (0=thấp, 1=cao) |
| Decay | +2 | 0–1 | Thời gian tắt |
| Pan | +3 | 0–1 | Trái(0) — Giữa(0.5) — Phải(1) |

Param ID = padIndex × 4 + offset. Ví dụ: Kick Volume = 0×4+0 = ID 0, Snare Pitch = 1×4+1 = ID 5.

### Tham Số Global

| Tham số | ID | Default | Mô tả |
|---------|---:|---------|-------|
| Global Volume | 64 | 0.8 | Âm lượng tổng |
| Kick Sweep Depth | 65 | 0.5 | Độ sâu sweep pitch kick |
| Kick Sweep Time | 66 | 0.2 | Thời gian sweep kick |

---

### 13.1 808 Boom Kit
**File:** `global_presets/29_drums_808_boom.json`  
**Thể loại:** Drums  
**Effects:** EQ (+8dB bass, cut highs) → Gain (light 1.2x)

TR-808 style — kick pitch thấp (0.3) + decay dài (0.75) + sweep depth cao (0.8) tạo iconic 808 boom. Snare pitched lên, hi-hat bright. EQ tăng bass, gain thêm warmth.

| Pad | Vol | Pitch | Decay | Pan |
|-----|-----|-------|-------|-----|
| Kick | 0.95 | 0.3 | 0.75 | C |
| Snare | 0.8 | 0.55 | 0.4 | C |
| HH Closed | 0.7 | 0.6 | 0.2 | C |
| HH Open | 0.6 | 0.55 | 0.5 | C |
| Clap | 0.7 | 0.5 | 0.3 | C |
| Tom Low | 0.75 | 0.35 | 0.6 | L |
| Tom Mid | 0.7 | 0.4 | 0.55 | R |

**Cách sử dụng:** Hip hop, trap, R&B. 808 boom bass, trap patterns.

---

### 13.2 Rock Kit
**File:** `global_presets/30_drums_rock_kit.json`  
**Thể loại:** Drums  
**Effects:** Reverb (room, 20%) → EQ (slight mid boost)

Rock kit cân bằng — kick vừa phải, snare nổi bật (0.9), hi-hat clean. Reverb room nhẹ cho không gian phòng thu.

| Pad | Vol | Pitch | Decay | Pan |
|-----|-----|-------|-------|-----|
| Kick | 0.85 | 0.45 | 0.5 | C |
| Snare | 0.9 | 0.55 | 0.45 | C |
| HH Closed | 0.75 | 0.55 | 0.3 | C |
| Tom Low | 0.8 | 0.4 | 0.55 | L |
| Tom Mid | 0.8 | 0.45 | 0.5 | R |

**Cách sử dụng:** Rock, pop. Standard drum patterns, fills.

---

### 13.3 Jazz Brush Kit
**File:** `global_presets/31_drums_jazz_brush.json`  
**Thể loại:** Drums  
**Effects:** Reverb (room, 20%) → EQ (warm, cut highs)

Jazz kit nhẹ nhàng — tất cả volumes thấp hơn mức bình thường, decay mềm. Reverb room + EQ warm cho ambient jazz.

| Pad | Vol | Pitch | Decay | Pan |
|-----|-----|-------|-------|-----|
| Kick | 0.6 | 0.45 | 0.4 | C |
| Snare | 0.55 | 0.5 | 0.5 | C |
| HH Closed | 0.5 | 0.55 | 0.25 | slight L |
| Tom Low | 0.55 | 0.45 | 0.5 | L |
| Rimshot | 0.55 | 0.55 | 0.3 | C |

**Cách sử dụng:** Jazz, lounge, brush patterns. Gentle touch, swing feel.

---

### 13.4 Electronic Kit
**File:** `global_presets/32_drums_electronic.json`  
**Thể loại:** Drums  
**Effects:** Delay (BPM sync 1/16, 20% mix) → Reverb (plate, 25%)

Kit điện tử sắc nét — pitch cao hơn mức bình thường, kick sweep mạnh (0.7). Delay BPM sync 1/16 tạo rhythmic echoes, reverb plate cho sheen.

| Pad | Vol | Pitch | Decay | Pan |
|-----|-----|-------|-------|-----|
| Kick | 0.9 | 0.35 | 0.6 | C |
| Snare | 0.85 | 0.65 | 0.35 | C |
| HH Closed | 0.8 | 0.7 | 0.15 | C |
| Clap | 0.8 | 0.6 | 0.25 | C |

**Cách sử dụng:** EDM, techno, electro. Quantized patterns, machine gun hats.

---

### 13.5 Lo-Fi Kit
**File:** `global_presets/33_drums_lo_fi.json`  
**Thể loại:** Drums  
**Effects:** Gain (overdrive 2x) → EQ (dark, cut highs) → Reverb (small, 15%)

Kit lo-fi bẩn — gain overdrive + EQ dark tạo lo-fi character. Volumes vừa phải, decay nhẹ dài hơn.

| Pad | Vol | Pitch | Decay | Pan |
|-----|-----|-------|-------|-----|
| Kick | 0.85 | 0.4 | 0.55 | C |
| Snare | 0.75 | 0.5 | 0.45 | C |
| HH Closed | 0.65 | 0.5 | 0.3 | C |

**Cách sử dụng:** Lo-fi hip hop, chillhop, beat-making. Lazy beats, SP-404 vibe.

---

### 13.6 Dub Echo Kit
**File:** `global_presets/34_drums_dub_echo.json`  
**Thể loại:** Drums  
**Effects:** Delay (650ms, 70% feedback, 45% mix) → Reverb (big, 40%)

Kit dub — delay feedback gần infinite + reverb tạo echo dub kinh điển. King Tubby style. Kick + snare echo trail dài.

| Pad | Vol | Pitch | Decay | Pan |
|-----|-----|-------|-------|-----|
| Kick | 0.85 | 0.35 | 0.65 | C |
| Snare | 0.8 | 0.5 | 0.5 | C |
| HH Closed | 0.7 | 0.55 | 0.25 | slight L |
| Rimshot | 0.6 | 0.55 | 0.3 | C |

**Cách sử dụng:** Dub, dub techno, reggae. Sparse hits, let echo fill space.

---

### 13.7 Industrial Kit
**File:** `global_presets/35_drums_industrial.json`  
**Thể loại:** Drums  
**Effects:** Gain (heavy distortion 3x) → Flanger (aggressive) → Delay (200ms, 30%)

Kit industrial hung hãn — distortion nặng + flanger metallic + delay short. Tất cả volumes và kick sweep ở mức cao. Nine Inch Nails, Ministry vibe.

| Pad | Vol | Pitch | Decay | Pan |
|-----|-----|-------|-------|-----|
| Kick | 0.95 | 0.3 | 0.55 | C |
| Snare | 0.9 | 0.65 | 0.4 | C |
| HH Closed | 0.85 | 0.7 | 0.15 | C |
| Clap | 0.85 | 0.55 | 0.3 | C |
| Kick Sweep | 0.9 (depth) | — | — | — |

**Cách sử dụng:** Industrial, EBM, noise. Aggressive patterns, distorted everything.

---

## 14. Hướng Dẫn Sử Dụng Preset (How to Use)

### Tải Engine Preset

1. Chọn engine mong muốn từ **Engine Selector** panel
2. Mở tab **Presets** → **Engine Presets**
3. Chọn preset từ danh sách
4. Click **Load** để áp dụng

> **Lưu ý:** Engine preset CHỈ thay đổi tham số engine. Effect chain không bị ảnh hưởng.

### Tải Global Preset

1. Mở tab **Presets** → **Global Presets**
2. Chọn preset từ danh sách
3. Click **Load** để áp dụng

> **Lưu ý:** Global preset sẽ:
> - Chuyển sang engine được chỉ định
> - Áp dụng tất cả tham số engine
> - Thay thế toàn bộ effect chain (slots + params)

### Lưu Preset Mới

1. Điều chỉnh âm thanh theo ý muốn
2. Mở tab **Presets**
3. Nhập tên cho preset mới
4. Click **Save**

### Tips

- **Bắt đầu từ preset gần nhất** với âm thanh mục tiêu, rồi tinh chỉnh
- **Effect chain** có thể thêm bớt sau khi load engine preset
- **Global preset** là cách nhanh nhất để có âm thanh hoàn chỉnh
- **Arpeggiator/Sequencer** nằm riêng trong Music panel, không bị ảnh hưởng bởi preset

---

## 15. Tham Khảo Effect Chain (Effect Reference)

Mỗi global preset có thể sử dụng tối đa 16 effect slots. Dưới đây là 8 loại effect:

### Bảng Effect Types

| Type ID | Tên | Tham số 1 | Tham số 2 | Tham số 3 | Tham số 4 | Tham số 5 | Tham số 6 |
|:-------:|------|-----------|-----------|-----------|-----------|-----------|-----------|
| 0 | **Gain** | Mode (0=Clean,1=OD,2=Dist) | Gain (0.1–10) | Asymmetry (0–1) | Level (0–1) | Tone (0–1) | — |
| 1 | **Chorus** | Depth (0–1) | Rate Hz (0–5) | Voices (1–4) | Mix (0–1) | — | — |
| 2 | **Flanger** | Depth (0–1) | Rate Hz (0–5) | Feedback (0–1) | Mix (0–1) | — | — |
| 3 | **Phaser** | Depth (0–1) | Rate Hz (0–5) | Feedback (0–1) | Mix (0–1) | — | — |
| 4 | **Tremolo** | Depth (0–1) | Rate Hz (0–20) | Shape (0=Sine) | — | — | — |
| 5 | **Delay** | Time s (0–2) | Feedback (0–1) | Mix (0–1) | BpmSync (0/1) | SyncDiv | — |
| 6 | **Reverb** | Size (0–1) | Decay (0–1) | Damping (0–1) | PreDelay s | Mix (0–1) | — |
| 7 | **Equalizer** | 80Hz ±12dB | 320Hz ±12dB | 1kHz ±12dB | 3.5kHz ±12dB | 10kHz ±12dB | Level (0–2) |

### Gain Mode

| Mode | Giá trị | Mô tả |
|------|---------|-------|
| Clean | 0 | Boost sạch, không đổi character |
| Overdrive | 1 | Soft clipping, ấm |
| Distortion | 2 | Hard clipping, hung hãn |

### Delay Sync Division

| Giá trị | Ý nghĩa |
|---------|---------|
| 1 | 1/1 (whole note) |
| 2 | 1/2 (half note) |
| 3 | 1/4 (quarter) |
| 4 | 1/8 (eighth) |
| 5 | 1/16 (sixteenth) |
| 6 | 1/32 |

---

*Tài liệu này được tạo tự động cho MiniMoog DSP Simulator V2.2.*
