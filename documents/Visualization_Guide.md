# Hướng Dẫn Đọc Visualization

**Phiên bản:** MiniMoog DSP Simulator v2.2  
**Cập nhật:** 2026-03-08

Menu **Visualization** chứa 6 công cụ hiển thị tín hiệu âm thanh theo thời gian thực. Tất cả đều hoạt động với mọi engine (Moog, Hammond, Rhodes, DX7, Mellotron, Drums).

---

## 1. Oscilloscope

**Mở:** Visualization → Oscilloscope

### Giao diện

```
dB  ┌────────────────────────────────────────────────┐
    │                    ▲ trigger                   │
    │         ╭──╮       │                           │
    │────────╯    ╰──────┴────────────────────────── │  ← 0 (zero crossing)
    │                    ╰───╮           ╭───        │
    │                         ╰─────────╯             │
    └────────────────────────────────────────────────┘
      [✓ Trigger]  [✓ Auto]  Scale: [──●──]
```

### Các điều khiển

| Điều khiển | Mô tả |
|------------|-------|
| **Trigger** | Khi bật: tìm điểm zero-crossing tăng (âm → dương) để cố định waveform, tránh hình ảnh nhảy loạn. Khi tắt: hiển thị raw stream liên tục |
| **Auto** | Tự động điều chỉnh scale Y để waveform luôn chiếm ~80% chiều cao. Khi tắt: dùng slider Scale để zoom thủ công |
| **Scale** | Chỉ khả dụng khi tắt Auto. Kéo phải = phóng to biên độ, kéo trái = thu nhỏ |

### Cách đọc

- **Waveform phẳng (đường thẳng):** Không có audio, engine chưa được trigger, hoặc Master Volume = 0
- **Waveform đối xứng qua trục 0:** Tín hiệu sạch, không DC offset
- **Waveform lệch hẳn lên / xuống:** Có DC offset — thường do nonlinearity quá mạnh (Drive cao)
- **Waveform clipping (đầu bị cắt phẳng):** Overload — giảm Master Volume hoặc Drive
- **Waveform nhảy / không ổn định:** Bật Trigger để cố định

### Dùng để làm gì

- Kiểm tra waveform cơ bản của oscillator (sine, sawtooth, square)
- Quan sát tác động của envelope (attack/release rõ ràng theo thời gian)
- Phát hiện clipping và DC offset

---

## 2. Spectrum Analyzer

**Mở:** Visualization → Spectrum Analyzer

### Giao diện

```
dB
 0 ┤     ▌█▌
-20┤   ▌▌███▌▌
-40┤  ▌████████▌
-60┤ ▌████████████▌▌
-80┤▌████████████████████▌▌▌
   └──────────────────────────→ Hz (log)
      50  200  1k   5k  20k
```

### Trục và thang đo

| Trục | Mô tả |
|------|-------|
| **Trục X (ngang)** | Tần số, thang **logarithm** từ 20 Hz → 20 kHz. Log scale giống cách tai nghe — mỗi octave chiếm cùng khoảng cách |
| **Trục Y (dọc)** | Biên độ theo **dB** từ 0 dB (trên) đến −80 dB (dưới) |

### Màu sắc thanh

| Màu | Ý nghĩa |
|-----|---------|
| 🟢 **Xanh lá** | Vùng bình thường (−80 dB → −16 dB) |
| 🟡 **Vàng** | Vùng cao (−16 dB → −6 dB) |
| 🔴 **Đỏ/cam** | Gần hoặc đạt 0 dB — cảnh báo clipping |

### Cách đọc

- **Moog Sawtooth:** Nhiều harmonics đều đặn cách nhau theo bội số fundamental (1×, 2×, 3×...), giảm dần theo −6 dB/octave
- **Moog Square:** Chỉ có harmonics lẻ (1×, 3×, 5×...) — các bin chẵn gần như trống
- **Moog với Filter Cutoff thấp:** Spectrum bị "cắt" ở một tần số — phần trên cutoff biến mất rõ ràng
- **Rhodes:** Peak mạnh ở fundamental, peak nhỏ ở 2×, 6× (bell chime), 9× (attack)
- **Hammond:** 9 đỉnh cách nhau đúng bội số của tonewheel (tỉ lệ với drawbar 16', 8', 4', ...)
- **DX7:** Phổ phức tạp, nhiều sideband FM xung quanh mỗi operator

### Dùng để làm gì

- **Phân tích âm sắc:** Biết waveform đang chứa những harmonic nào
- **Kiểm tra filter:** Quan sát cutoff và resonance tác động lên phổ
- **Tune EQ:** Nhìn thấy tần số dư thừa cần cắt

---

## 3. Spectrogram (Waterfall)

**Mở:** Visualization → Spectrogram

### Giao diện

```
Tần số → (20 Hz ─────────────────── ~11 kHz)
  Thời          ██░░░░░░░░░░░░░░░░  ← cũ nhất (trên)
  gian          ████▓▓░░░░░░░░░░░░
  |             ██████████▓▓░░░░░░
  ↓             ████████████████▓░
  (mới nhất)    ██████████████████  ← mới nhất (dưới)
```

### Bảng màu

| Màu | Mức âm lượng |
|-----|-------------|
| ⬛ **Đen** | Không có tín hiệu (< −80 dB) |
| 🟢 **Xanh đậm** | Yếu (−80 → −40 dB) |
| 🟡 **Vàng xanh** | Trung bình (−40 → −20 dB) |
| 🟠 **Cam** | Mạnh (−20 → −6 dB) |
| ⬜ **Trắng** | Rất mạnh (≥ −6 dB) |

### Cách đọc

- **Trục ngang:** Tần số (trái = thấp, phải = cao) — **tuyến tính**, không log
- **Trục dọc:** Thời gian — **hàng trên = cũ hơn, hàng dưới = mới nhất**
- **Vệt ngang sáng ổn định:** Một partial đang sustain liên tục — đặc trưng của Rhodes/Hammond
- **Vệt sáng rồi tắt nhanh:** Partial có decay ngắn — như bell chime của Rhodes (mode 6×)
- **Nhiều vệt ngang đều đặn:** Âm thanh giàu harmonics — Moog sawtooth điển hình
- **Chỉ 1 vệt ở dưới:** Sine wave thuần túy

### So sánh engine trên Spectrogram

| Engine | Hình ảnh điển hình |
|--------|-------------------|
| **Moog Sawtooth** | Nhiều vệt đều, sáng dần từ trái qua phải, filter cutoff thấy rõ |
| **Moog với Filter Envelope** | Vệt trên cao xuất hiện → mờ dần khi envelope release |
| **Rhodes** | 4 vệt chính (1×, 2×, 6×, 9×) — 2 vệt trên tắt nhanh, 2 dưới tắt chậm |
| **Hammond** | 9 vệt đều nhau, độ sáng theo drawbar — không bao giờ tắt (sustain hoàn toàn) |
| **DX7** | Cụm vệt dày phức tạp quanh mỗi partial |

### Dùng để làm gì

- **Phân tích decay của từng partial:** Thấy harmonic nào sustain lâu, cái nào chết nhanh
- **Quan sát envelope filter:** Tần số cao xuất hiện rồi mờ dần theo filter envelope
- **So sánh preset:** Chạy 2 preset liên tiếp, scroll history cho thấy sự khác biệt

---

## 4. Lissajous / Vectorscope

**Mở:** Visualization → Lissajous

### Giao diện

```
        + (R dương)
        │
   L ───┼─── R        Trục X = kênh Left
        │              Trục Y = kênh Right
        - (R âm)
        
   /  ← đường chéo = Mono
```

### Hình dạng và ý nghĩa

| Hình dạng | Ý nghĩa |
|-----------|---------|
| **Đường thẳng chéo 45° từ trái-dưới → phải-trên** | **Mono hoàn toàn** — L = R. Thấy khi Stereo Spread = 0 |
| **Đường thẳng chéo 135°** | **Anti-phase** — L = −R. Nguy hiểm: signal triệt tiêu khi fold to mono |
| **Hình elip nghiêng theo / (thuận chiều)** | **Stereo bình thường** — hai kênh tương quan thuận, elip càng rộng = stereo càng rộng |
| **Hình elip nghiêng theo \\ (ngược chiều)** | **Partial anti-phase** — cẩn thận khi mono |
| **Hình tròn** | **Quadrature stereo** — L và R lệch pha 90°, thường thấy ở tremolo Suitcase tốc độ cao |
| **Hình bướm / hoa** | **Stereo phức tạp** — nhiều nguồn âm với pan khác nhau, cộng hưởng |
| **Đám mây kín** | Noise hoặc reverb dày đặc |
| **Điểm ở tâm** | Không có tín hiệu |

### Trail (vệt mờ dần)

Lissajous vẽ 512 sample gần nhất. Sample cũ hơn = mờ hơn (alpha thấp), sample mới = sáng hơn. Điều này giúp thấy sự vận động và tốc độ thay đổi của tín hiệu.

### Ví dụ thực tế

- **Rhodes + Tremolo 5 Hz, Depth 0.6:** Hình elip xoay từ nghiêng phải → nghiêng trái theo nhịp tremolo. Tốc độ xoay = Tremolo Rate
- **Hammond full drawbars, no tremolo:** Elip hơi dày, nghiêng thuận chiều — stereo nhẹ từ Stereo Spread
- **Moog mono:** Gần như đường thẳng đúng 45°
- **DX7 EP với reverb:** Đám mây rộng, không đối xứng hoàn toàn

### Dùng để làm gì

- **Kiểm tra stereo width:** Elip rộng = stereo tốt
- **Phát hiện anti-phase:** Elip nghiêng ngược chiều = vấn đề khi fold to mono
- **Quan sát tremolo:** Thấy tốc độ và độ sâu của autopan
- **Mixing:** Đảm bảo signal mono-compatible (không có anti-phase)

---

## 5. VU Meter / Peak Meter

**Mở:** Visualization → VU Meter

### Giao diện

```
  +0 dB  ─ ─ ─ ─ ─  ← Peak hold indicator (vạch vàng/đỏ)
  -3 dB  ░░░░░░░░░░  ← Vùng đỏ
  -6 dB  ▓▓▓▓▓▓▓▓▓▓  ← Vùng vàng
 -10 dB  ██████████
 -20 dB  ████████
 -30 dB  ██████
 -40 dB  ████
 -60 dB  ██
         L     R      ← Số dB hiển thị trên thanh
```

### Các thành phần

| Thành phần | Mô tả |
|------------|-------|
| **Thanh xanh lá** | RMS level (−60 → −6 dB) — mức âm trung bình theo thời gian, có ballistic (rise nhanh, fall chậm) |
| **Thanh vàng** | RMS level vùng (−6 → −3 dB) — bắt đầu nóng |
| **Thanh đỏ** | RMS level vùng (−3 → 0 dB) — cảnh báo |
| **Vạch ngang vàng/đỏ** | **Peak hold** — giá trị peak lớn nhất trong 2 giây qua. Vàng = peak bình thường, Đỏ = peak trên −3 dB |
| **Số dB nhỏ** | Reading dB tức thời của VU (RMS) cho mỗi kênh |

### Ballistic (quán tính kim)

VU meter không phản ứng tức thời — giống đồng hồ cơ:
- **Attack nhanh:** RMS tăng lên nhanh khi có tín hiệu mới
- **Release chậm:** RMS giảm chậm sau khi tín hiệu dừng (~80 frame ≈ 1.3 giây từ −6 dB về −60 dB)

Điều này cho thấy mức "cảm nhận" của âm thanh, không chỉ peak thô.

### Hướng dẫn mức chuẩn

| Mức VU | Ý nghĩa | Hành động |
|--------|---------|-----------|
| Dưới −20 dB thường xuyên | Quá nhỏ | Tăng Master Volume |
| −18 dB → −6 dB | **Vùng lý tưởng** | Để nguyên |
| −6 dB → −3 dB | Hơi nóng | Theo dõi peak |
| Trên −3 dB thường xuyên | **Cảnh báo** | Giảm Drive hoặc Volume |
| Peak hold đỏ liên tục | **Clipping** | Giảm ngay |

### Dùng để làm gì

- **Kiểm tra balance L/R:** Hai thanh nên gần bằng nhau. Nếu L >> R hoặc ngược lại = vấn đề stereo pan
- **Set gain staging:** Điều chỉnh Master Volume để VU dao động trong vùng −18 đến −6 dB
- **Phát hiện clipping:** Peak hold đỏ = clipping đã xảy ra trong 2 giây vừa rồi

---

## 6. Correlation Meter

**Mở:** Visualization → Correlation Meter

### Giao diện

```
 -1          0          +1
  ├──────────┼──────────┤
  │[RED BAR] │[GRN BAR] │  ← thanh màu
  └──────────┴──────────┘
  ρ = +0.847   Narrow stereo
```

### Thang đo Pearson ρ

Correlation meter đo hệ số tương quan Pearson giữa kênh L và R trên 512 sample vừa qua:

$$\rho = \frac{\sum L_i \cdot R_i}{\sqrt{\sum L_i^2 \cdot \sum R_i^2}}$$

| Giá trị ρ | Màu | Ý nghĩa |
|-----------|-----|---------|
| **+0.8 → +1.0** | 🟢 Xanh | **MONO** — L và R gần như giống nhau. An toàn fold to mono |
| **+0.3 → +0.8** | 🟢 Xanh | **Narrow stereo** — stereo nhẹ, mono-safe |
| **−0.1 → +0.3** | 🟡 Vàng | **Wide stereo** — stereo rộng. Kiểm tra khi mono |
| **−0.5 → −0.1** | 🔴 Đỏ | **Partial anti-phase** — một phần tín hiệu triệt tiêu khi mono |
| **−1.0 → −0.5** | 🔴 Đỏ | **ANTI-PHASE [!]** — cực kỳ nguy hiểm, signal biến mất hoàn toàn khi mono |

### Ví dụ thực tế

| Cài đặt | ρ điển hình |
|---------|------------|
| Moog bất kỳ (mono engine) | ρ ≈ +1.0 — thanh dài sang phải, màu xanh, "MONO" |
| Rhodes, Spread = 0 | ρ ≈ +1.0 — mono |
| Rhodes, Spread = 0.6 (mặc định) | ρ ≈ +0.6..+0.8 — "Narrow stereo" |
| Rhodes, Spread = 1.0 | ρ ≈ +0.2..+0.5 — "Wide stereo" |
| Rhodes + Tremolo Depth 0.6 | Biến động theo tremolo — dao động 0 → +0.5 theo nhịp |
| Hammond với phasing | ρ có thể xuống âm tạm thời |
| Effect Chain: Phaser mạnh | ρ dao động âm dương liên tục |

### Cảnh báo anti-phase

Nếu ρ < −0.1 thường xuyên:
1. Kiểm tra Effect Chain — Phaser/Flanger có thể tạo anti-phase
2. Kiểm tra Stereo Spread quá rộng kết hợp với panning bất đối xứng
3. Kiểm tra preset có DI nghịch cực

### Dùng để làm gì

- **Mono compatibility check:** Trước khi export/mixdown, đảm bảo ρ > 0 khi không có nhạc cụ khác
- **Phát hiện vấn đề phasing:** ρ âm = tín hiệu sẽ bị hủy khi phát mono (TV, điện thoại, radio)
- **Calibrate stereo width:** Điều chỉnh Stereo Spread / pan cho đến khi ρ ở mức mong muốn

---

## Bảng so sánh — Dùng visualization nào cho mục đích gì

| Mục đích | Tool được dùng |
|----------|----------------|
| Kiểm tra waveform cơ bản | Oscilloscope |
| Phân tích âm sắc / harmonics | Spectrum Analyzer |
| Xem decay của từng partial theo thời gian | Spectrogram |
| Kiểm tra stereo width & tremolo | Lissajous |
| Kiểm tra gain / phát hiện clipping | VU Meter |
| Kiểm tra mono compatibility | Correlation Meter |
| Debug filter cutoff | Spectrum Analyzer + Oscilloscope |
| So sánh hai preset | Spectrogram (scroll history) |
| Phát hiện anti-phase | Lissajous + Correlation Meter |
| Set master volume chuẩn | VU Meter |

---

## Phím tắt gợi ý khi sử dụng

Tất cả panel visualization có thể:
- **Kéo tiêu đề** để di chuyển tự do trên màn hình
- **Kéo góc** để resize
- **Click X** để đóng (hoặc bỏ tick trong menu Visualization)
- Sắp xếp song song: đặt Spectrum + Lissajous + VU Meter cạnh nhau để quan sát đồng thời

---

*File này được tạo tự động bởi GitHub Copilot (Claude Sonnet 4.6) — 2026-03-08*
