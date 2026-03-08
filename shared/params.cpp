// ─────────────────────────────────────────────────────────
// FILE: shared/params.cpp
// BRIEF: PARAM_META table — global + music layer params only
// ─────────────────────────────────────────────────────────
#include "params.h"

// Order MUST match ParamID enum exactly.
const ParamMeta PARAM_META[PARAM_COUNT] = {
    // name                  default   min      max
    // ── GLOBAL ──────────────────────────────────────
    { "Master Volume",        0.8f,    0.0f,    1.0f  },
    { "BPM",                120.0f,   60.0f,  200.0f  },

    // ── ARPEGGIATOR ─────────────────────────────────
    { "Arp On",               0.0f,    0.0f,    1.0f  },
    { "Arp Mode",             0.0f,    0.0f,    5.0f  },
    { "Arp Octaves",          1.0f,    1.0f,    4.0f  },
    { "Arp Rate",             3.0f,    0.0f,    7.0f  },
    { "Arp Gate",             0.5f,    0.0f,    1.0f  },
    { "Arp Swing",            0.0f,    0.0f,    0.5f  },

    // ── SEQUENCER ───────────────────────────────────
    { "Seq On",               0.0f,    0.0f,    1.0f  },
    { "Seq Playing",          0.0f,    0.0f,    1.0f  },
    { "Seq Steps",           16.0f,    1.0f,   16.0f  },
    { "Seq Rate",             3.0f,    0.0f,    7.0f  },
    { "Seq Gate",             0.5f,    0.0f,    1.0f  },
    { "Seq Swing",            0.0f,    0.0f,    0.5f  },

    // ── CHORD ───────────────────────────────────────
    { "Chord On",             0.0f,    0.0f,    1.0f  },
    { "Chord Type",           0.0f,    0.0f,   15.0f  },
    { "Chord Inversion",      0.0f,    0.0f,    3.0f  },

    // ── SCALE ───────────────────────────────────────
    { "Scale On",             0.0f,    0.0f,    1.0f  },
    { "Scale Root",           0.0f,    0.0f,   11.0f  },
    { "Scale Type",           0.0f,    0.0f,   15.0f  },
};
