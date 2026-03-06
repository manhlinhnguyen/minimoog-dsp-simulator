# Hướng Dẫn Sử Dụng — MiniMoog DSP Simulator v1.0

> Phần mềm mô phỏng bộ tổng hợp âm thanh Minimoog Model D huyền thoại,
> được phát triển bằng C++17 với giao diện ImGui và engine DSP độ chính xác cao.

---

## Mục Lục

1. [Khởi động phần mềm](#1-khởi-động-phần-mềm)
2. [Giao diện tổng quan](#2-giao-diện-tổng-quan)
3. [Thanh menu View & Help](#3-thanh-menu-view--help)
4. [Thanh trạng thái (Status Bar)](#4-thanh-trạng-thái-status-bar)
5. [Cửa sổ Controllers](#5-cửa-sổ-controllers)
6. [Cửa sổ Oscillators](#6-cửa-sổ-oscillators)
7. [Cửa sổ Mixer](#7-cửa-sổ-mixer)
8. [Cửa sổ Filter & Envelopes](#8-cửa-sổ-filter--envelopes)
9. [Cửa sổ Music](#9-cửa-sổ-music)
10. [Cửa sổ Oscilloscope](#10-cửa-sổ-oscilloscope)
11. [Cửa sổ Output](#11-cửa-sổ-output)
12. [Cửa sổ Keyboard & Play](#12-cửa-sổ-keyboard--play)
13. [Cửa sổ Presets](#13-cửa-sổ-presets)
14. [Bàn phím QWERTY](#14-bàn-phím-qwerty)
15. [Hướng dẫn tạo âm thanh cơ bản](#15-hướng-dẫn-tạo-âm-thanh-cơ-bản)

---

## 1. Khởi Động Phần Mềm

Chạy file thực thi:

```
build/bin/minimoog_sim.exe
```

Phần mềm sẽ mở một cửa sổ 1400×820 px với tiêu đề **"MiniMoog DSP Simulator v1.0"**.
Âm thanh được phát qua card âm thanh mặc định của hệ thống. Nếu MIDI controller USB được
kết nối trước khi khởi động, nó sẽ được nhận diện tự động.

> **Lưu ý Windows:** Không cần cài đặt thêm DLL — tất cả thư viện đã được
> link tĩnh vào file .exe.

---

## 2. Giao Diện Tổng Quan

Giao diện gồm các thành phần chính:

```
┌──────────────────────────────────────────────────────────────┐
│  [View ▼]  [Help ▼]          ← Thanh menu trên cùng          │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│   ┌─Controllers─┐  ┌─Oscillators──┐  ┌─Mixer─┐             │
│   │ Glide       │  │ OSC 1        │  │ OSC1  │             │
│   │ Mod         │  │ OSC 2        │  │ OSC2  │             │
│   │ BPM         │  │ OSC 3 (LFO)  │  │ OSC3  │             │
│   │ Polyphony   │  │ Master Tune  │  │ Noise │             │
│   └─────────────┘  └──────────────┘  └───────┘             │
│                                                              │
│   ┌─Filter & Envelopes─┐  ┌─Music──────────────────┐       │
│   │ Cutoff / Emphasis  │  │ [Arp][Chord][Scale][Seq]│       │
│   │ Filter ADSR        │  └────────────────────────┘       │
│   │ Amp ADSR           │                                    │
│   └────────────────────┘  ┌─Oscilloscope──────────┐        │
│                            │ ~~~waveform display~~~│        │
│   ┌─Presets─┐  ┌─Output─┐ └───────────────────────┘        │
│   │ Load    │  │ Volume │                                   │
│   │ Save    │  └────────┘  ┌─Keyboard & Play───────┐       │
│   └─────────┘              │ [piano keys display]  │       │
│                            └───────────────────────┘       │
├──────────────────────────────────────────────────────────────┤
│  Voices: 1/1  |  BPM: 120  |  Octave: 4    ← Status bar    │
└──────────────────────────────────────────────────────────────┘
```

Mỗi cửa sổ có thể **kéo thả** tự do, **thu nhỏ** (click vào tiêu đề), hoặc
**ẩn/hiện** qua menu **View**.

---

## 3. Thanh Menu View & Help

### Menu View

Click **View** trên thanh menu để bật/tắt từng cửa sổ:

| Mục menu      | Cửa sổ tương ứng          |
|---------------|---------------------------|
| Controllers   | Controllers               |
| Oscillators   | Oscillators               |
| Mixer         | Mixer                     |
| Filter/Env    | Filter & Envelopes        |
| Music         | Music (Arp/Chord/Scale/Seq)|
| Oscilloscope  | Oscilloscope              |
| Output        | Output                    |
| Keyboard      | Keyboard & Play           |
| Presets       | Presets                   |
| Debug         | Debug (thông tin kỹ thuật)|

Dấu ✓ bên cạnh tên = cửa sổ đang hiển thị. Click để đổi trạng thái.

### Menu Help

Hiển thị thông tin nhanh về bàn phím QWERTY:
- **Z–M** = các nốt C đến B (hàng trắng)
- **Q–U** = quãng tám cao hơn
- **[** / **]** = Octave Down / Octave Up

---

## 4. Thanh Trạng Thái (Status Bar)

Thanh ở **cuối màn hình** hiển thị thời gian thực:

```
Voices: 1/1  |  BPM: 120  |  Octave: 4
```

| Thông tin | Ý nghĩa |
|-----------|---------|
| `Voices: X/Y` | X = số voice đang phát, Y = số voice tối đa đã cài |
| `BPM: 120`    | Tempo hiện tại (ảnh hưởng đến Arpeggiator và Sequencer) |
| `Octave: 4`   | Quãng tám hiện tại của bàn phím QWERTY |

---

## 5. Cửa Sổ Controllers

Cửa sổ **Controllers** chứa các điều khiển toàn cục cho bộ tổng hợp.

### GLIDE (Portamento)

| Điều khiển | Mô tả |
|------------|-------|
| **Glide On** (checkbox) | Bật/tắt hiệu ứng trượt nốt giữa các note liên tiếp |
| **Time** (knob) | Thời gian trượt. Xoay sang phải = trượt chậm hơn |

> Glide chỉ có tác dụng ở chế độ **Mono** (Voice Mode = Mono).
> Ở chế độ poly, mỗi voice trượt độc lập.

### MODULATION

| Điều khiển | Mô tả |
|------------|-------|
| **Mod Mix** (knob) | Tỉ lệ pha trộn nguồn LFO: 0 = OSC3 thuần, 1 = Noise thuần |
| **OSC Mod** (checkbox) | Bật điều chế pitch của OSC1+OSC2 bằng LFO |
| **Filter Mod** (checkbox) | Bật điều chế cutoff filter bằng LFO |

> **OSC3 làm LFO**: Khi checkbox **LFO Mode** trong cửa sổ Oscillators được bật
> cho OSC3, OSC3 không đi vào mixer mà chỉ cấp tín hiệu cho ModMatrix.
> Tần số OSC3 (knob Freq) quyết định tốc độ LFO.

### TEMPO

| Điều khiển | Mô tả |
|------------|-------|
| **BPM** (slider, 20–300) | Tempo chung. Ảnh hưởng đến Arpeggiator và Step Sequencer |

### POLYPHONY

| Điều khiển | Mô tả |
|------------|-------|
| **Voice Mode** | **Mono** = 1 nốt, **Poly** = đa thanh, **Unison** = nhiều voice chồng lên 1 nốt |
| **Voices** (slider, 1–8) | Số voice tối đa. Chỉ ý nghĩa ở Poly và Unison |
| **Voice Steal** | Cách lấy voice khi đã hết: **Oldest** / **Lowest** / **Quietest** |
| **Detune** (slider) | Độ lệch pitch giữa các voice (chỉ ở Unison). Cao hơn = âm dày hơn |

---

## 6. Cửa Sổ Oscillators

Ba bộ dao động (OSC1, OSC2, OSC3) hiển thị song song. Mỗi OSC có các điều khiển:

### Các điều khiển mỗi OSC

| Điều khiển | Mô tả |
|------------|-------|
| **ON** (checkbox) | Bật/tắt oscillator |
| **Range** (combo) | Dải tần: `LO` (sub-audio/LFO), `32'`, `16'`, `8'`, `4'`, `2'` |
| **Wave** (combo) | Dạng sóng (xem bảng dưới) |
| **Freq** (knob) | Tinh chỉnh pitch (fine tune). 0.5 = đúng note, thấp hơn/cao hơn = detuned |

### Các dạng sóng

| Số | Tên | Đặc điểm âm thanh |
|----|-----|-------------------|
| 0 | Triangle | Mềm, ít hài âm — cơ bản nhất |
| 1 | Triangle-Sawtooth | Chuyển tiếp giữa tam giác và răng cưa |
| 2 | Reverse Sawtooth | Răng cưa ngược — sáng nhưng khác Saw |
| 3 | Sawtooth | Răng cưa — giàu hài âm, sáng, cổ điển Moog |
| 4 | Square | Vuông — ấm, hollow, nhiều bậc lẻ |
| 5 | Pulse | Xung hẹp — mỏng, nasal, điện tử |

### Dải tần Range

| Ký hiệu | Ý nghĩa |
|---------|---------|
| `LO` | Dưới ngưỡng âm thanh (~1 Hz) — dùng làm LFO |
| `32'` | Thấp nhất (sub bass) |
| `16'` | Bass sâu |
| `8'` | **Chuẩn** — đây là dải tần tiêu chuẩn |
| `4'` | Cao hơn 1 quãng tám so với `8'` |
| `2'` | Cao hơn 2 quãng tám so với `8'` |

### OSC3 — Chế độ đặc biệt

- **LFO Mode** (checkbox): Khi bật, OSC3 hoạt động như LFO, không phát ra âm thanh trực tiếp.
  Nốt nhạc không ảnh hưởng đến tần số OSC3 khi ở LFO Mode.
- Khi tắt LFO Mode: OSC3 hoạt động như OSC1/2, tín hiệu đi vào Mixer.

### Master Tune

Thanh trượt **Master Tune** ở cuối cửa sổ: điều chỉnh pitch toàn bộ
instrument lên/xuống (±100 cents).

---

## 7. Cửa Sổ Mixer

Cân bằng âm lượng của các nguồn âm trước khi vào Filter.

| Knob | Mô tả |
|------|-------|
| **OSC1** | Mức âm lượng Oscillator 1 (0 = tắt, 1 = full) |
| **OSC2** | Mức âm lượng Oscillator 2 |
| **OSC3** | Mức âm lượng Oscillator 3 (chỉ khi LFO Mode = off) |
| **Noise** | Mức âm lượng tiếng ồn |
| **Noise Color** (combo) | **White** = tất cả tần số đều; **Pink** = nghiêng về bass |

> Kéo knob bằng chuột trái + giữ. Giữ **Ctrl** khi kéo để điều chỉnh chính xác.
> **Double-click** để nhập giá trị thủ công.

---

## 8. Cửa Sổ Filter & Envelopes

Đây là trung tâm của bộ tổng hợp — Moog Ladder Filter 4-pole nổi tiếng.

### MOOG FILTER

| Điều khiển | Mô tả |
|------------|-------|
| **Cutoff** (knob, 0–1) | Tần số cắt. 0 = tối (~20 Hz), 1 = mở hoàn toàn (~20 kHz) |
| **Emphasis** (knob, 0–1) | Cộng hưởng (resonance). Tăng cao → âm "rít", ~0.9 = tự dao động |
| **Env Amt** (knob, 0–1) | Mức độ Filter Envelope ảnh hưởng lên Cutoff. 0 = không ảnh hưởng |
| **KBD Track** (combo) | Cutoff tự điều chỉnh theo nốt đang nhấn: **Off**, **1/3**, **2/3** |

> **KBD Track = 2/3** là thiết lập phổ biến nhất — làm cho âm thanh nhất quán hơn
> trên toàn bộ bàn phím (filter mở ra ở nốt cao, đóng lại ở nốt thấp).

### FILTER ENV (Bao âm Filter)

Điều khiển cách Cutoff thay đổi khi bạn nhấn và thả nốt:

| Knob | Tên đầy đủ | Mô tả |
|------|-----------|-------|
| **A** | Attack | Thời gian từ khi nhấn nốt đến khi Cutoff đạt đỉnh. 0 = tức thì |
| **D** | Decay | Thời gian từ đỉnh về đến mức Sustain |
| **S** | Sustain | Mức Cutoff duy trì khi đang giữ nốt (0 = đóng hoàn toàn) |
| **R** | Release | Thời gian Cutoff đóng lại sau khi thả nốt |

Biểu đồ ADSR hiển thị ngay bên dưới 4 knob để xem trực quan.

### AMP ENV (Bao âm Biên độ)

Điều khiển hình dạng âm lượng (volume envelope):

| Knob | Mô tả |
|------|-------|
| **A** | Fade in — từ im lặng đến đỉnh. 0 = tức thì (âm sắc nét) |
| **D** | Giảm từ đỉnh xuống Sustain sau khi nhấn |
| **S** | Mức âm lượng duy trì khi đang giữ nốt |
| **R** | Fade out sau khi thả nốt |

> **Mẹo:** Để có tiếng organ/pad: A và R cao, S = 1.0, D thấp.
> Để có tiếng bass/pluck: A = 0, D cao, S thấp, R thấp.

---

## 9. Cửa Sổ Music

Cửa sổ **Music** gom 4 tính năng nhạc lý vào một nơi, với 4 tab:

### Tab Arpeggiator

Arpeggiator tự động phát các nốt bạn đang giữ theo trình tự.

**Cách dùng:** Bật Enable → giữ nhiều nốt cùng lúc → Arp tự phát.

| Điều khiển | Mô tả |
|------------|-------|
| **Enable** | Bật/tắt Arpeggiator |
| **Mode** | Thứ tự phát: `Up`, `Down`, `Up/Down`, `Down/Up`, `Random`, `As Played` |
| **Octaves** (1–4) | Số quãng tám lặp lại. 2 = phát từng nốt rồi lặp lại 1 quãng tám cao hơn |
| **Rate** | Tốc độ: `1/1`, `1/2`, `1/4`, `1/8`, `1/16`, `3/8`, `3/16`, `1/4T` |
| **Gate** (knob) | Độ dài mỗi nốt so với step. 1.0 = legato, 0.1 = staccato |
| **Swing** (knob) | Độ "swing" nhịp. 0 = thẳng, 0.5 = swing cực đại |

> **Rate** phụ thuộc vào **BPM** trong cửa sổ Controllers.
> `1/4` ở BPM=120 = 2 nốt mỗi giây.

### Tab Chord

Chord Engine mở rộng mỗi nốt bạn nhấn thành một hợp âm đầy đủ.

**Cách dùng:** Bật Enable → nhấn 1 nốt → nghe cả hợp âm.

| Điều khiển | Mô tả |
|------------|-------|
| **Enable** | Bật/tắt Chord Engine |
| **Chord Type** | Loại hợp âm (Major, Minor, Dom7, Maj7, Min7, Sus4, Dim, Aug...) |
| **Inversion** | Đảo thế: `Root` (gốc), `1st Inv`, `2nd Inv`, `3rd Inv` |

> Chord Engine cần **Voice Mode = Poly** và đủ số Voices mới phát hết được tất cả nốt.

### Tab Scale

Scale Quantizer tự động "snap" (gần nhất) các nốt bạn nhấn vào đúng nốt trong
thang âm đã chọn. Không bao giờ lạc điệu!

| Điều khiển | Mô tả |
|------------|-------|
| **Enable** | Bật/tắt Scale Quantizer |
| **Root** | Nốt gốc của thang âm: C, C#, D, D#, E, F, F#, G, G#, A, A#, B |
| **Scale** | Loại thang âm (Major, Minor, Pentatonic, Blues, Dorian, Phrygian...) |

### Tab Sequencer

Bộ giải trình tự bước (Step Sequencer) với tối đa 16 bước.

**Cách dùng:**
1. Bật **Enable**
2. Click vào ô bước để bật/tắt bước đó
3. Cuộn chuột lên/xuống trên ô để thay đổi nốt nhạc
4. Click **Play** để bắt đầu phát

| Điều khiển | Mô tả |
|------------|-------|
| **Enable** | Bật/tắt Sequencer |
| **Play/Stop** | Bắt đầu/dừng phát |
| **Clear** | Tắt tất cả các bước |
| **Steps** (1–16) | Số bước trong vòng lặp |
| **Rate** | Tốc độ mỗi bước (giống Rate của Arp) |
| **Gate** (knob) | Độ dài mỗi nốt |
| **Swing** (knob) | Độ swing |

**Thao tác trên lưới Step:**

| Thao tác | Tác dụng |
|----------|---------|
| Click chuột trái | Bật/tắt bước (màu sáng = bật) |
| Click chuột phải | Bật/tắt TIE (nối với bước tiếp theo) |
| Cuộn chuột lên | Tăng nốt (+1 semitone) |
| Cuộn chuột xuống | Giảm nốt (−1 semitone) |

> Bước đang phát được đánh dấu khác màu. Sequencer tự động sync với BPM.

---

## 10. Cửa Sổ Oscilloscope

Hiển thị dạng sóng âm thanh đầu ra thời gian thực.

| Điều khiển | Mô tả |
|------------|-------|
| **Trigger** (checkbox) | Khi bật: dạng sóng được đồng bộ theo điểm qua 0 (stable). Khi tắt: cuộn tự do |
| **Scale** (slider, 0.1x–10x) | Phóng to/thu nhỏ biên độ. 1x = tự động scale |
| **Peak** (hiển thị) | Biên độ đỉnh hiện tại |

**Lưới hiển thị:** Đường kẻ ngang chia ±50% và ±100%. Đường dọc chia 4 phần thời gian.
Đường sóng màu xanh lá.

> Oscilloscope hữu ích để kiểm tra dạng sóng, phát hiện clipping (sóng bị cắt phẳng),
> và so sánh hình dạng envelope.

---

## 11. Cửa Sổ Output

| Điều khiển | Mô tả |
|------------|-------|
| **Master Volume** (knob) | Âm lượng tổng đầu ra. 0 = im lặng, 1 = tối đa |

> Master Volume khác với **Volume** trong cửa sổ Keyboard & Play —
> cả hai đều là cùng tham số `P_MASTER_VOL`.

---

## 12. Cửa Sổ Keyboard & Play

### Master Volume

Knob **Volume** ở đầu cửa sổ — điều chỉnh âm lượng tổng đầu ra.

### Bàn phím nhấp chuột

Phần piano có thể click bằng chuột:
- **Click chuột trái** vào phím → phát nốt (Note On)
- **Giữ chuột** → nốt tiếp tục
- **Thả chuột** → dừng nốt (Note Off)

### Hướng dẫn bàn phím QWERTY

```
┌───┐ ┌───┐   ┌───┐ ┌───┐ ┌───┐   ┌───┐   ┌───┐ ┌───┐
│ S │ │ D │   │ G │ │ H │ │ J │   │ L │   │ ; │ │ ' │  ← Phím đen
└───┘ └───┘   └───┘ └───┘ └───┘   └───┘   └───┘ └───┘
┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐ ┌───┐
│ Z │ │ X │ │ C │ │ V │ │ B │ │ N │ │ M │ │ , │ ...  ← Phím trắng
└───┘ └───┘ └───┘ └───┘ └───┘ └───┘ └───┘ └───┘
  C     D     E     F     G     A     B     C+1
```

Hàng phím trên (Q–U) tương ứng quãng tám cao hơn hàng Z–M.

---

## 13. Cửa Sổ Presets

### Các nút điều khiển

| Nút | Tác dụng |
|-----|---------|
| **Reset to Defaults** | Đặt lại TẤT CẢ tham số về giá trị nhà máy |
| **Refresh** | Đọc lại danh sách file preset từ thư mục `./presets/` |
| **Save** | Lưu cài đặt hiện tại vào file. Nhập tên vào ô text trước |
| **Load** | Tải preset được chọn trong danh sách |

### Quy trình tải preset

1. Click **Refresh** (nếu danh sách trống)
2. Click chọn tên preset trong danh sách
3. Click **Load**

### Quy trình lưu preset

1. Nhập tên vào ô text (mặc định: `MyPreset`)
2. Click **Save** → tạo file `TênBạn.json` trong thư mục `./presets/`

### Danh sách preset có sẵn

**Bass:**
| File | Mô tả |
|------|-------|
| `bass_classic` | Bass cổ điển Minimoog |
| `bass_fat` | Bass dày với filter |
| `bass_taurus` | Moog Taurus — bass sâu cực thấp |
| `bass_funk` | Bass funk percussive |
| `bass_sub` | Sub bass dưới 60 Hz |

**Lead:**
| File | Mô tả |
|------|-------|
| `lead_acid` | Acid lead với filter sweep |
| `lead_mellow` | Lead mềm mại |
| `lead_emerson` | Keith Emerson / Rick Wakeman style |
| `lead_lucky_man` | Solo "Lucky Man" — ELP 1970 |
| `lead_prog_rock` | Lead prog rock hung hăng |
| `lead_theremin` | Giả lập Theremin với LFO vibrato |
| `lead_unison_fat` | 6-voice unison, detuned dày |

**Pad:**
| File | Mô tả |
|------|-------|
| `pad_warm` | Pad ấm áp |
| `pad_strings` | Dây giả |
| `pad_analog_strings` | Dây analog detuned dày |
| `pad_solar_wind` | Atmospheric — OSC3 LFO filter sweep |

**Brass & Keys:**
| File | Mô tả |
|------|-------|
| `brass_stab` | Brass stab — nhạc funk/rock |
| `keys_pluck` | Pluck có tính phím |

**FX & Demo:**
| File | Mô tả |
|------|-------|
| `fx_sweep` | Sweep hiệu ứng |
| `arp_sequence` | Demo Arpeggiator |

---

## 14. Bàn Phím QWERTY

### Bảng ánh xạ đầy đủ

| Phím | Nốt | Loại |
|------|-----|------|
| **Z** | C  | Trắng |
| **S** | C# | Đen   |
| **X** | D  | Trắng |
| **D** | D# | Đen   |
| **C** | E  | Trắng |
| **V** | F  | Trắng |
| **G** | F# | Đen   |
| **B** | G  | Trắng |
| **H** | G# | Đen   |
| **N** | A  | Trắng |
| **J** | A# | Đen   |
| **M** | B  | Trắng |
| **,** | C+1| Trắng |

Hàng trên cùng (Q–U) chơi quãng tám cao hơn:

| Phím | Nốt |
|------|-----|
| **Q** | C+1 |
| **2** | C#  |
| **W** | D   |
| **3** | D#  |
| **E** | E   |
| **R** | F   |
| **5** | F#  |
| **T** | G   |
| **6** | G#  |
| **Y** | A   |
| **7** | A#  |
| **U** | B   |
| **I** | C+2 |

### Điều chỉnh Octave

| Phím | Tác dụng |
|------|---------|
| **[** | Giảm 1 quãng tám (Octave Down) |
| **]** | Tăng 1 quãng tám (Octave Up) |

Octave hiện tại hiển thị ở góc dưới phải màn hình (Status Bar).

> **Lưu ý:** Phần mềm cần được focus (cửa sổ active) để nhận phím từ bàn phím QWERTY.

---

## 15. Hướng Dẫn Tạo Âm Thanh Cơ Bản

### Phát nốt đầu tiên

1. Khởi động phần mềm
2. Nhấn **]** vài lần để đưa Octave lên 4 hoặc 5
3. Nhấn phím **V** (nốt F4) — bạn sẽ nghe âm thanh

Nếu không nghe:
- Kiểm tra card âm thanh của Windows đã bật chưa
- Kiểm tra **Master Volume** trong cửa sổ Output hoặc Keyboard & Play > 0
- Kiểm tra các **OSC** đã bật và **Mix** knob > 0

### Tạo bass cổ điển Minimoog

1. **Controllers:** Voice Mode = Mono, Glide On, Glide Time ~0.3
2. **Oscillators:** OSC1 On, Range=16', Wave=Sawtooth; OSC2 On, Range=16', Wave=Square, Freq=0.501 (detuned nhẹ)
3. **Mixer:** OSC1=1.0, OSC2=0.7
4. **Filter:** Cutoff=0.3, Emphasis=0.5, Env Amt=0.6, KBD Track=Off
5. **Filter ENV:** A=0, D=0.4, S=0.1, R=0.3
6. **Amp ENV:** A=0, D=0.5, S=0.7, R=0.3

### Tạo tiếng "wah" khi nhấn nốt (Lucky Man style)

1. **OSC1:** Sawtooth 8'; **OSC2:** Square 8', Freq=0.504
2. **Filter:** Cutoff=0.40, Emphasis=0.65, Env Amt=0.68
3. **Filter ENV:** A=0, D=0.60, S=0.22, R=0.50
4. **Amp ENV:** A=0, D=0.55, S=0.88, R=0.55
5. **Controllers:** Glide On, Time=0.14; Voice Mode=Mono

### Tạo pad atmospheric (Solar Wind style)

1. **OSC1:** Sawtooth 8', **OSC2:** Tri 16', Freq=0.49; **OSC3:** Tri LO, LFO Mode=On
2. **Mixer:** OSC1=0.7, OSC2=0.6, Noise=0.2
3. **Controllers:** Filter Mod=On, OSC3 Freq thấp (~0.2)
4. **Filter:** Cutoff=0.42, Emphasis=0.2, Env Amt=0.5
5. **Filter ENV:** A=0.6, D=0.5, S=0.6, R=0.8
6. **Amp ENV:** A=0.6, D=0.3, S=0.8, R=1.0

### Sử dụng Arpeggiator

1. **Music → Arpeggiator:** Enable=On, Mode=Up, Octaves=2, Rate=1/8
2. **Controllers:** BPM=120
3. Giữ 3–4 nốt cùng lúc trên bàn phím QWERTY → nghe Arp tự chạy

### Sử dụng Step Sequencer

1. **Music → Sequencer:** Enable=On, Steps=8, Rate=1/8
2. Click các ô trong lưới để bật các bước
3. Cuộn chuột trên từng ô để chọn nốt
4. Click **Play** → Seq tự chạy, không cần giữ phím

---

## Các Phím Tắt Hữu Ích

| Thao tác | Phím / Chuột |
|----------|-------------|
| Kéo knob chính xác | Giữ **Ctrl** + kéo chuột |
| Nhập giá trị thủ công | **Double-click** vào knob/slider |
| Reset về giá trị mặc định | **Ctrl + Click** vào knob |
| Tăng/giảm Octave | **[** / **]** |
| Ẩn/hiện cửa sổ | Menu **View** |

---

*MiniMoog DSP Simulator v1.0 — Tài liệu hướng dẫn nội bộ*
