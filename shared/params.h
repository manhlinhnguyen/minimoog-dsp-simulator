// ─────────────────────────────────────────────────────────
// FILE: shared/params.h
// BRIEF: Global + music-layer ParamID enum and metadata.
// Moog-specific params live in core/engines/moog/moog_params.h
// ─────────────────────────────────────────────────────────
#pragma once
#include <cstddef>

// ════════════════════════════════════════════════════════
// PARAM ID — global + music layer parameters only.
// Used by: AtomicParamStore, EngineManager, UI music panel,
//          Output panel (P_MASTER_VOL).
// ════════════════════════════════════════════════════════

enum ParamID : int {

    // ── GLOBAL ──────────────────────────────────────────
    P_MASTER_VOL = 0,   // 0.0..1.0
    P_BPM,              // 60.0..200.0

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

    PARAM_COUNT         // = 20
};

// ════════════════════════════════════════════════════════
// PARAM METADATA — name, default, min, max
// ════════════════════════════════════════════════════════

struct ParamMeta {
    const char* name;
    float       defaultVal;
    float       minVal;
    float       maxVal;
};

// Defined in shared/params.cpp
extern const ParamMeta PARAM_META[PARAM_COUNT];
