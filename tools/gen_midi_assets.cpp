// ─────────────────────────────────────────────────────────
// FILE: tools/gen_midi_assets.cpp
// BRIEF: Generates classic MIDI files into assets/midi/
//   Usage: gen_midi_assets <output_dir>
// ─────────────────────────────────────────────────────────
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

// ════════════════════════════════════════════════════════
// SMF builder — format 0, single track
// ════════════════════════════════════════════════════════

static void vlq(std::vector<uint8_t>& out, uint32_t v) {
    if (v < 0x80u)    { out.push_back(static_cast<uint8_t>(v)); return; }
    if (v < 0x4000u)  { out.push_back(0x80u | (v >> 7)); out.push_back(v & 0x7Fu); return; }
    if (v < 0x200000u){ out.push_back(0x80u | (v >> 14)); out.push_back(0x80u | ((v >> 7) & 0x7Fu)); out.push_back(v & 0x7Fu); return; }
    out.push_back(0x80u | (v >> 21));
    out.push_back(0x80u | ((v >> 14) & 0x7Fu));
    out.push_back(0x80u | ((v >>  7) & 0x7Fu));
    out.push_back(v & 0x7Fu);
}
static void be16(std::vector<uint8_t>& o, uint16_t v){ o.push_back(v>>8); o.push_back(v&0xFF); }
static void be32(std::vector<uint8_t>& o, uint32_t v){ o.push_back(v>>24); o.push_back((v>>16)&0xFF); o.push_back((v>>8)&0xFF); o.push_back(v&0xFF); }

struct Ev {
    uint32_t tick;
    bool     isNoteOff;   // sort priority at same tick
    bool     isMeta;
    uint8_t  status;
    uint8_t  d1, d2;
    std::vector<uint8_t> meta;
};

struct MidiFile {
    uint16_t      ppqn;
    std::vector<Ev> events;

    MidiFile(uint16_t p = 480) : ppqn(p) {}

    void tempo(uint32_t tick, uint32_t uspb) {
        Ev e{}; e.tick=tick; e.isMeta=true; e.status=0xFF; e.d1=0x51;
        e.meta = { uint8_t(uspb>>16), uint8_t((uspb>>8)&0xFF), uint8_t(uspb&0xFF) };
        events.push_back(std::move(e));
    }
    void timeSig(uint32_t tick, uint8_t num, uint8_t denom) {
        Ev e{}; e.tick=tick; e.isMeta=true; e.status=0xFF; e.d1=0x58;
        e.meta = { num, denom, 24, 8 };
        events.push_back(std::move(e));
    }
    void trackName(uint32_t tick, const char* name) {
        Ev e{}; e.tick=tick; e.isMeta=true; e.status=0xFF; e.d1=0x03;
        e.meta.assign(name, name+std::strlen(name));
        events.push_back(std::move(e));
    }
    // noteDur = sounding duration (< ticks for articulation gap)
    void note(uint32_t start, uint32_t ticks, uint8_t n, uint8_t vel, uint8_t ch=0) {
        const uint32_t noteDur = (ticks > 20) ? ticks - 10 : ticks;
        const uint32_t end     = start + ticks;  // NoteOff at rhythmic end
        // NoteOn
        Ev on{}; on.tick=start; on.status=uint8_t(0x90|(ch&0x0F)); on.d1=n; on.d2=vel;
        events.push_back(on);
        // NoteOff
        Ev off{}; off.tick=start+noteDur; off.isNoteOff=true;
        off.status=uint8_t(0x80|(ch&0x0F)); off.d1=n; off.d2=0;
        (void)end;
        events.push_back(off);
    }

    std::vector<uint8_t> build() {
        std::stable_sort(events.begin(), events.end(), [](const Ev& a, const Ev& b){
            if (a.tick != b.tick) return a.tick < b.tick;
            if (a.isMeta  != b.isMeta)   return a.isMeta;   // meta first
            if (a.isNoteOff != b.isNoteOff) return a.isNoteOff; // noteOff before noteOn
            return false;
        });

        std::vector<uint8_t> trk;
        uint32_t last = 0;
        for (const auto& e : events) {
            vlq(trk, e.tick - last); last = e.tick;
            if (e.isMeta) {
                trk.push_back(0xFF); trk.push_back(e.d1);
                vlq(trk, uint32_t(e.meta.size()));
                for (uint8_t b : e.meta) trk.push_back(b);
            } else {
                trk.push_back(e.status); trk.push_back(e.d1);
                if ((e.status & 0xF0) != 0xC0) trk.push_back(e.d2);
            }
        }
        // End of track
        trk.push_back(0x00); trk.push_back(0xFF); trk.push_back(0x2F); trk.push_back(0x00);

        std::vector<uint8_t> out;
        out.push_back('M'); out.push_back('T'); out.push_back('h'); out.push_back('d');
        be32(out, 6); be16(out, 0); be16(out, 1); be16(out, ppqn);
        out.push_back('M'); out.push_back('T'); out.push_back('r'); out.push_back('k');
        be32(out, uint32_t(trk.size()));
        for (uint8_t b : trk) out.push_back(b);
        return out;
    }
};

static bool writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fwrite(data.data(), 1, data.size(), f);
    std::fclose(f);
    return true;
}

// ════════════════════════════════════════════════════════
// Note helpers
// ════════════════════════════════════════════════════════

struct N { int8_t note; uint16_t ticks; uint8_t vel; };  // note=-1 → rest

// Add a sequence of N[] starting at startTick, returns end tick
static uint32_t addNotes(MidiFile& mf, const N* ns, int cnt,
                          uint32_t startTick, uint8_t ch = 0) {
    uint32_t t = startTick;
    for (int i = 0; i < cnt; ++i) {
        if (ns[i].note >= 0)
            mf.note(t, ns[i].ticks, uint8_t(ns[i].note), ns[i].vel, ch);
        t += ns[i].ticks;
    }
    return t;
}

// Durations (480 PPQN)
static constexpr uint16_t W  = 1920; // whole
static constexpr uint16_t H  = 960;  // half
static constexpr uint16_t DH = 1440; // dotted half
static constexpr uint16_t Q  = 480;  // quarter
static constexpr uint16_t DQ = 720;  // dotted quarter
static constexpr uint16_t E  = 240;  // eighth
static constexpr uint16_t DE = 360;  // dotted eighth
static constexpr uint16_t S  = 120;  // sixteenth

// Note numbers (C4 = 60)
enum {
    C2=36, Cs2=37, D2=38, Ds2=39, E2=40, F2=41, Fs2=42, G2=43, Gs2=44, A2=45, As2=46, B2=47,
    C3=48, Cs3=49, D3=50, Ds3=51, E3=52, F3=53, Fs3=54, G3=55, Gs3=56, A3=57, As3=58, B3=59,
    C4=60, Cs4=61, D4=62, Ds4=63, E4=64, F4=65, Fs4=66, G4=67, Gs4=68, A4=69, As4=70, B4=71,
    C5=72, Cs5=73, D5=74, Ds5=75, E5=76, F5=77, Fs5=78, G5=79, Gs5=80, A5=81, As5=82, B5=83,
    C6=84, REST=-1
};

// ════════════════════════════════════════════════════════
// PIECE 1 — Für Elise (Beethoven, WoO 59, A minor)
// Tempo 72 BPM (adagio)
// ════════════════════════════════════════════════════════
static std::vector<uint8_t> buildFurElise() {
    MidiFile mf;
    const uint32_t uspb = 833333; // 72 BPM
    mf.tempo(0, uspb);
    mf.timeSig(0, 3, 3); // 3/8
    mf.trackName(0, "Fur Elise");

    // Main A section — melody (ch 0)
    static const N mel[] = {
        // Opening motif (×2)
        {E5,E,80},{Ds5,E,80},{E5,E,80},{Ds5,E,80},{E5,E,80},{B4,E,80},{D5,E,80},{C5,E,80},
        {A4,Q,80},{REST,E,0},{C4,E,65},{E4,E,65},{A4,E,65},
        {B4,Q,80},{REST,E,0},{E4,E,65},{Gs4,E,65},{B4,E,65},
        {C5,Q,80},{REST,E,0},{E4,E,65},{E5,E,80},
        // Repeat motif
        {Ds5,E,80},{E5,E,80},{Ds5,E,80},{E5,E,80},{B4,E,80},{D5,E,80},{C5,E,80},
        {A4,Q,80},{REST,E,0},{C4,E,65},{E4,E,65},{A4,E,65},
        {B4,Q,80},{REST,E,0},{E4,E,65},{C5,E,80},{B4,E,80},
        {A4,DQ,80},{REST,E,0},{REST,Q,0},
        // B section (simplified)
        {B4,E,75},{C5,E,75},{D5,E,75},{E5,Q,80},{REST,E,0},{G3,E,65},{F5,E,80},{E5,E,80},
        {D5,Q,80},{REST,E,0},{F3,E,65},{E5,E,80},{D5,E,80},
        {C5,Q,80},{REST,E,0},{E3,E,65},{D5,E,80},{C5,E,80},
        {B4,Q,80},{REST,E,0},{REST,E,0},{REST,Q,0},
        // Return to A
        {E5,E,80},{Ds5,E,80},{E5,E,80},{Ds5,E,80},{E5,E,80},{B4,E,80},{D5,E,80},{C5,E,80},
        {A4,Q,80},{REST,E,0},{C4,E,65},{E4,E,65},{A4,E,65},
        {B4,Q,80},{REST,E,0},{E4,E,65},{Gs4,E,65},{B4,E,65},
        {C5,Q,80},{REST,E,0},{E4,E,65},{E5,E,80},
        {Ds5,E,80},{E5,E,80},{Ds5,E,80},{E5,E,80},{B4,E,80},{D5,E,80},{C5,E,80},
        {A4,Q,80},{REST,E,0},{C4,E,65},{E4,E,65},{A4,E,65},
        {B4,Q,80},{REST,E,0},{E4,E,65},{C5,E,80},{B4,E,80},
        {A4,H,80},{REST,Q,0},
    };
    addNotes(mf, mel, sizeof(mel)/sizeof(mel[0]), 0, 0);
    return mf.build();
}

// ════════════════════════════════════════════════════════
// PIECE 2 — Ode to Joy (Beethoven, Op. 125, simplified)
// Tempo 108 BPM
// ════════════════════════════════════════════════════════
static std::vector<uint8_t> buildOdeToJoy() {
    MidiFile mf;
    mf.tempo(0, 555555); // 108 BPM
    mf.timeSig(0, 4, 2);
    mf.trackName(0, "Ode to Joy");

    static const N mel[] = {
        // Phrase 1
        {E4,Q,80},{E4,Q,80},{F4,Q,80},{G4,Q,80},
        {G4,Q,80},{F4,Q,80},{E4,Q,80},{D4,Q,80},
        {C4,Q,80},{C4,Q,80},{D4,Q,80},{E4,Q,80},
        {E4,DQ,80},{D4,E,80},{D4,H,80},
        // Phrase 2 (same)
        {E4,Q,80},{E4,Q,80},{F4,Q,80},{G4,Q,80},
        {G4,Q,80},{F4,Q,80},{E4,Q,80},{D4,Q,80},
        {C4,Q,80},{C4,Q,80},{D4,Q,80},{E4,Q,80},
        {D4,DQ,80},{C4,E,80},{C4,H,80},
        // Phrase 3
        {D4,Q,80},{D4,Q,80},{E4,Q,80},{C4,Q,80},
        {D4,Q,80},{E4,E,80},{F4,E,80},{E4,Q,80},{C4,Q,80},
        {D4,Q,80},{E4,E,80},{F4,E,80},{E4,Q,80},{D4,Q,80},
        {C4,Q,80},{D4,Q,80},{G3,H,80},
        // Phrase 4
        {E4,Q,80},{E4,Q,80},{F4,Q,80},{G4,Q,80},
        {G4,Q,80},{F4,Q,80},{E4,Q,80},{D4,Q,80},
        {C4,Q,80},{C4,Q,80},{D4,Q,80},{E4,Q,80},
        {D4,DQ,80},{C4,E,80},{C4,H,80},
    };
    addNotes(mf, mel, sizeof(mel)/sizeof(mel[0]), 0, 0);
    return mf.build();
}

// ════════════════════════════════════════════════════════
// PIECE 3 — Minuet in G (Petzold/Bach, BWV Anh. 114)
// Tempo 126 BPM, 3/4
// ════════════════════════════════════════════════════════
static std::vector<uint8_t> buildMinuetG() {
    MidiFile mf;
    mf.tempo(0, 476190); // 126 BPM
    mf.timeSig(0, 3, 2);
    mf.trackName(0, "Minuet in G");

    static const N mel[] = {
        // Part 1 A (bars 1-8)
        {D5,Q,80},{G4,Q,75},{A4,Q,75},
        {B4,Q,80},{C5,Q,80},{D5,Q,80},
        {G4,H,80},{REST,Q,0},
        {E5,Q,80},{C5,Q,75},{D5,Q,75},
        {E5,Q,80},{Fs5,Q,80},{G5,Q,80},
        {A5,H,80},{REST,Q,0},
        {A5,Q,78},{B4,E,70},{C5,E,70},{D5,E,70},{E5,E,70},
        {Fs5,Q,80},{G5,Q,78},{G4,Q,75},
        {Fs5,H,78},{REST,Q,0},
        // Part 1 B (bars 9-16)
        {B5,Q,80},{G5,E,75},{A5,E,75},{B5,E,75},{G5,E,75},
        {A5,Q,78},{D5,E,72},{E5,E,72},{Fs5,E,72},{D5,E,72},
        {G5,Q,78},{E5,E,72},{Fs5,E,72},{G5,E,72},{E5,E,72},
        {Fs5,Q,80},{D5,E,72},{C5,E,72},{B4,E,72},{A4,E,72},
        {B4,Q,75},{C5,Q,75},{D5,Q,78},
        {G4,Q,80},{A4,Q,75},{B4,Q,75},
        {C5,Q,78},{B4,Q,75},{A4,Q,72},
        {G4,H,80},{REST,Q,0},
    };

    static const N bass[] = {
        // Part 1 A bass (simplified root notes)
        {G3,Q,65},{D3,Q,60},{D3,Q,60},
        {G3,Q,65},{G3,Q,60},{B3,Q,60},
        {G3,H,65},{REST,Q,0},
        {C4,Q,65},{G3,Q,60},{G3,Q,60},
        {C4,Q,65},{C4,Q,60},{E4,Q,60},
        {D4,H,65},{REST,Q,0},
        {D4,Q,65},{G3,E,58},{A3,E,58},{B3,E,58},{C4,E,58},
        {D4,Q,65},{G3,Q,60},{G3,Q,58},
        {D4,H,65},{REST,Q,0},
        // Part 1 B bass
        {G3,Q,65},{G3,Q,60},{G3,Q,60},
        {D3,Q,65},{Fs3,Q,60},{D3,Q,60},
        {G3,Q,65},{C4,Q,60},{G3,Q,60},
        {D3,Q,65},{A3,Q,60},{Fs3,Q,58},
        {G3,Q,65},{G3,Q,60},{G3,Q,60},
        {G3,Q,65},{Fs3,Q,60},{G3,Q,60},
        {C4,Q,65},{G3,Q,60},{D3,Q,60},
        {G3,H,65},{REST,Q,0},
    };

    addNotes(mf, mel,  sizeof(mel) /sizeof(mel[0]),  0, 0);
    addNotes(mf, bass, sizeof(bass)/sizeof(bass[0]), 0, 1);
    return mf.build();
}

// ════════════════════════════════════════════════════════
// PIECE 4 — Greensleeves (Traditional, A minor)
// Tempo 90 BPM, 3/4
// ════════════════════════════════════════════════════════
static std::vector<uint8_t> buildGreensleeves() {
    MidiFile mf;
    mf.tempo(0, 666667); // 90 BPM
    mf.timeSig(0, 3, 2);
    mf.trackName(0, "Greensleeves");

    static const N mel[] = {
        // Verse A (bars 1-8)
        {A3,Q,78},                              // pickup
        {C4,H,80},{D4,Q,78},
        {E4,DQ,80},{D4,E,75},{C4,Q,75},
        {A3,H,80},{A3,Q,72},
        {A3,H,72},{B3,Q,70},
        {C4,H,75},{D4,Q,78},
        {E4,DQ,80},{D4,E,75},{C4,Q,75},
        {A3,H,80},{REST,Q,0},
        // Verse B
        {Gs3,Q,70},
        {B3,H,75},{Gs3,Q,70},
        {E3,H,72},{Fs3,Q,68},
        {Gs3,DQ,75},{A3,E,72},{B3,Q,75},
        {A3,H,78},{A3,Q,72},
        {C4,H,80},{D4,Q,78},
        {E4,DQ,80},{D4,E,75},{C4,Q,75},
        {A3,H,80},{REST,Q,0},
        // Refrain A
        {C5,Q,82},
        {C5,H,82},{B4,Q,78},
        {A4,DQ,82},{G4,E,75},{Fs4,Q,72},
        {E4,H,80},{E4,Q,78},
        {G4,H,78},{Fs4,Q,72},
        {E4,DQ,80},{D4,E,75},{C4,Q,75},
        {D4,H,78},{E4,Q,80},
        {A3,H,82},{REST,Q,0},
        // Refrain B
        {C5,Q,82},
        {C5,H,82},{B4,Q,78},
        {A4,DQ,82},{G4,E,75},{Fs4,Q,72},
        {E4,DQ,80},{Fs4,E,75},{G4,Q,78},
        {A4,H,82},{A4,Q,80},
        {C5,H,82},{B4,Q,78},
        {A4,DQ,82},{G4,E,75},{Fs4,Q,72},
        {E4,H,80},{REST,Q,0},
    };

    static const N bass[] = {
        {REST,Q,0}, // pickup rest
        {A2,Q,60},{E3,Q,55},{A3,Q,55},
        {A2,Q,60},{E3,Q,55},{A3,Q,55},
        {A2,Q,60},{E3,Q,55},{A3,Q,55},
        {A2,Q,60},{E3,Q,55},{A3,Q,55},
        {G2,Q,60},{D3,Q,55},{G3,Q,55},
        {C3,Q,62},{G3,Q,55},{C4,Q,55},
        {E3,Q,60},{A2,Q,55},{E3,Q,55},
        {A2,H,62},{REST,Q,0},
        {E2,Q,58},{B2,Q,55},{E3,Q,55},
        {E2,Q,58},{B2,Q,55},{E3,Q,55},
        {E2,Q,58},{B2,Q,55},{E3,Q,55},
        {E2,Q,58},{B2,Q,55},{E3,Q,55},
        {A2,Q,60},{E3,Q,55},{A3,Q,55},
        {C3,Q,62},{G3,Q,55},{C4,Q,55},
        {E3,Q,60},{A2,Q,55},{E3,Q,55},
        {A2,H,62},{REST,Q,0},
        // Refrain bass (simplified)
        {REST,Q,0},
        {A2,Q,60},{E3,Q,55},{A3,Q,55},
        {Fs2,Q,58},{Cs3,Q,52},{Fs3,Q,52},
        {A2,Q,60},{E3,Q,55},{A3,Q,55},
        {G2,Q,60},{D3,Q,55},{G3,Q,55},
        {C3,Q,62},{G3,Q,55},{C4,Q,55},
        {D3,Q,62},{A3,Q,55},{D4,Q,55},
        {A2,H,62},{REST,Q,0},
        {REST,Q,0},
        {A2,Q,60},{E3,Q,55},{A3,Q,55},
        {Fs2,Q,58},{Cs3,Q,52},{Fs3,Q,52},
        {A2,Q,60},{E3,Q,55},{A3,Q,55},
        {A2,Q,60},{E3,Q,55},{A3,Q,55},
        {C3,Q,62},{G3,Q,55},{C4,Q,55},
        {E3,Q,60},{A2,Q,55},{E3,Q,55},
        {A2,H,62},{REST,Q,0},
    };

    addNotes(mf, mel,  sizeof(mel) /sizeof(mel[0]),  0, 0);
    addNotes(mf, bass, sizeof(bass)/sizeof(bass[0]), 0, 1);
    return mf.build();
}

// ════════════════════════════════════════════════════════
// PIECE 5 — Canon in D (Pachelbel, D major)
// Tempo 72 BPM
// ════════════════════════════════════════════════════════
static std::vector<uint8_t> buildCanonD() {
    MidiFile mf;
    mf.tempo(0, 833333); // 72 BPM
    mf.timeSig(0, 4, 2);
    mf.trackName(0, "Canon in D");

    // Famous 8-note bass ostinato: D A B Fs G D G A
    static const uint8_t ostinato[] = { D3, A3, B3, Fs3, G3, D3, G3, A3 };

    // Repeat ostinato 8 times (ch 1) = 32 bars
    for (int rep = 0; rep < 8; ++rep) {
        for (int i = 0; i < 8; ++i) {
            const uint32_t t = uint32_t(rep * 8 + i) * H;
            mf.note(t, H, ostinato[i], 68, 1);
        }
    }

    // Melody (ch 0) — 32 bars starting after 1 ostinato repeat (bar 9)
    static const N mel[] = {
        // Bars 1-4: long notes
        {Fs5,H,80},{E5,H,78},
        {D5,H,80},{Cs5,H,78},
        {B4,H,80},{A4,H,78},
        {B4,H,80},{Cs5,H,78},
        // Bars 5-8: quarters
        {D5,Q,82},{Cs5,Q,80},{B4,Q,80},{A4,Q,78},
        {G4,Q,80},{Fs4,Q,78},{E4,Q,78},{Fs4,Q,76},
        {G4,Q,80},{A4,Q,80},{G4,Q,78},{Fs4,Q,78},
        {G4,Q,80},{A4,Q,80},{B4,Q,82},{Cs5,Q,80},
        // Bars 9-12: eighths
        {D5,E,82},{E5,E,82},{Fs5,E,82},{G5,E,82},{Fs5,E,80},{E5,E,80},{D5,E,80},{Cs5,E,80},
        {B4,E,80},{A4,E,78},{B4,E,80},{Cs5,E,80},{D5,E,82},{E5,E,82},{Fs5,E,82},{G5,E,82},
        {A5,E,82},{G5,E,80},{Fs5,E,80},{E5,E,80},{D5,E,82},{E5,E,82},{Fs5,E,82},{G5,E,82},
        {Fs5,E,82},{G5,E,82},{A5,E,82},{G5,E,80},{Fs5,E,80},{E5,E,80},{D5,E,82},{Cs5,E,80},
        // Final
        {D5,H,82},{D4,H,78},
        {D5,W,82},
    };

    addNotes(mf, mel, sizeof(mel)/sizeof(mel[0]), uint32_t(8 * H), 0);
    return mf.build();
}

// ════════════════════════════════════════════════════════
// main
// ════════════════════════════════════════════════════════

int main(int argc, char* argv[]) {
    const std::string outDir = (argc > 1) ? argv[1] : "./assets/midi";

    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);

    struct Piece { const char* name; std::vector<uint8_t>(*fn)(); };
    const Piece pieces[] = {
        { "fur_elise.mid",   buildFurElise   },
        { "ode_to_joy.mid",  buildOdeToJoy   },
        { "minuet_g.mid",    buildMinuetG    },
        { "greensleeves.mid",buildGreensleeves},
        { "canon_d.mid",     buildCanonD     },
    };

    int ok = 0;
    for (const auto& p : pieces) {
        const std::string path = outDir + "/" + p.name;
        const auto data = p.fn();
        if (writeFile(path, data)) {
            std::printf("  wrote %s (%zu bytes)\n", path.c_str(), data.size());
            ++ok;
        } else {
            std::fprintf(stderr, "  FAILED: %s\n", path.c_str());
        }
    }
    std::printf("Generated %d/%d MIDI files.\n", ok, (int)(sizeof(pieces)/sizeof(pieces[0])));
    return (ok == (int)(sizeof(pieces)/sizeof(pieces[0]))) ? 0 : 1;
}
