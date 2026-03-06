// ─────────────────────────────────────────────────────────
// FILE: shared/params.cpp
// BRIEF: PARAM_META table — names, ranges, defaults
// ─────────────────────────────────────────────────────────
#include "params.h"

// Order MUST match ParamID enum exactly.
const ParamMeta PARAM_META[PARAM_COUNT] = {
    // name                 jsonKey              default  min     max     discrete  unit
    // ── GLOBAL ──────────────────────────────────────────────────────────────────
    { "Master Tune",        "master_tune",        0.0f,  -1.0f,   1.0f,  false,  "st"  },
    { "Master Volume",      "master_vol",         0.8f,   0.0f,   1.0f,  false,  ""    },
    { "BPM",                "bpm",              120.0f,  60.0f, 200.0f,  false,  "BPM" },

    // ── CONTROLLERS ─────────────────────────────────────────────────────────────
    { "Glide On",           "glide_on",           0.0f,   0.0f,   1.0f,  true,   ""    },
    { "Glide Time",         "glide_time",         0.1f,   0.0f,   1.0f,  false,  ""    },
    { "Mod Mix",            "mod_mix",            0.0f,   0.0f,   1.0f,  false,  ""    },
    { "OSC Mod On",         "osc_mod_on",         0.0f,   0.0f,   1.0f,  true,   ""    },
    { "Filter Mod On",      "filter_mod_on",      0.0f,   0.0f,   1.0f,  true,   ""    },
    { "OSC3 LFO Mode",      "osc3_lfo_on",        0.0f,   0.0f,   1.0f,  true,   ""    },

    // ── OSCILLATOR 1 ────────────────────────────────────────────────────────────
    { "OSC1 On",            "osc1_on",            1.0f,   0.0f,   1.0f,  true,   ""    },
    { "OSC1 Range",         "osc1_range",         3.0f,   0.0f,   5.0f,  true,   ""    },
    { "OSC1 Freq",          "osc1_freq",          0.5f,   0.0f,   1.0f,  false,  ""    },
    { "OSC1 Wave",          "osc1_wave",          3.0f,   0.0f,   5.0f,  true,   ""    },

    // ── OSCILLATOR 2 ────────────────────────────────────────────────────────────
    { "OSC2 On",            "osc2_on",            1.0f,   0.0f,   1.0f,  true,   ""    },
    { "OSC2 Range",         "osc2_range",         3.0f,   0.0f,   5.0f,  true,   ""    },
    { "OSC2 Freq",          "osc2_freq",          0.5f,   0.0f,   1.0f,  false,  ""    },
    { "OSC2 Wave",          "osc2_wave",          3.0f,   0.0f,   5.0f,  true,   ""    },

    // ── OSCILLATOR 3 ────────────────────────────────────────────────────────────
    { "OSC3 On",            "osc3_on",            1.0f,   0.0f,   1.0f,  true,   ""    },
    { "OSC3 Range",         "osc3_range",         3.0f,   0.0f,   5.0f,  true,   ""    },
    { "OSC3 Freq",          "osc3_freq",          0.5f,   0.0f,   1.0f,  false,  ""    },
    { "OSC3 Wave",          "osc3_wave",          3.0f,   0.0f,   5.0f,  true,   ""    },

    // ── MIXER ───────────────────────────────────────────────────────────────────
    { "Mix OSC1",           "mix_osc1",           1.0f,   0.0f,   1.0f,  false,  ""    },
    { "Mix OSC2",           "mix_osc2",           0.0f,   0.0f,   1.0f,  false,  ""    },
    { "Mix OSC3",           "mix_osc3",           0.0f,   0.0f,   1.0f,  false,  ""    },
    { "Mix Noise",          "mix_noise",          0.0f,   0.0f,   1.0f,  false,  ""    },
    { "Noise Color",        "noise_color",        0.0f,   0.0f,   1.0f,  true,   ""    },

    // ── FILTER ──────────────────────────────────────────────────────────────────
    { "Filter Cutoff",      "filter_cutoff",      0.7f,   0.0f,   1.0f,  false,  ""    },
    { "Filter Emphasis",    "filter_emphasis",    0.0f,   0.0f,   1.0f,  false,  ""    },
    { "Filter Env Amount",  "filter_amount",      0.5f,   0.0f,   1.0f,  false,  ""    },
    { "Filter Kbd Track",   "filter_kbd_track",   0.0f,   0.0f,   2.0f,  true,   ""    },

    // ── FILTER ENVELOPE ──────────────────────────────────────────────────────────
    { "F.Env Attack",       "fenv_attack",        0.1f,   0.0f,   1.0f,  false,  ""    },
    { "F.Env Decay",        "fenv_decay",         0.3f,   0.0f,   1.0f,  false,  ""    },
    { "F.Env Sustain",      "fenv_sustain",       0.5f,   0.0f,   1.0f,  false,  ""    },
    { "F.Env Release",      "fenv_release",       0.3f,   0.0f,   1.0f,  false,  ""    },

    // ── AMP ENVELOPE ─────────────────────────────────────────────────────────────
    { "A.Env Attack",       "aenv_attack",        0.05f,  0.0f,   1.0f,  false,  ""    },
    { "A.Env Decay",        "aenv_decay",         0.3f,   0.0f,   1.0f,  false,  ""    },
    { "A.Env Sustain",      "aenv_sustain",       0.7f,   0.0f,   1.0f,  false,  ""    },
    { "A.Env Release",      "aenv_release",       0.3f,   0.0f,   1.0f,  false,  ""    },

    // ── POLYPHONY ────────────────────────────────────────────────────────────────
    { "Voice Mode",         "voice_mode",         1.0f,   0.0f,   2.0f,  true,   ""    },
    { "Voice Count",        "voice_count",        4.0f,   1.0f,   8.0f,  true,   ""    },
    { "Voice Steal",        "voice_steal",        0.0f,   0.0f,   2.0f,  true,   ""    },
    { "Unison Detune",      "unison_detune",      0.2f,   0.0f,   1.0f,  false,  ""    },

    // ── ARPEGGIATOR ──────────────────────────────────────────────────────────────
    { "Arp On",             "arp_on",             0.0f,   0.0f,   1.0f,  true,   ""    },
    { "Arp Mode",           "arp_mode",           0.0f,   0.0f,   5.0f,  true,   ""    },
    { "Arp Octaves",        "arp_octaves",        1.0f,   1.0f,   4.0f,  true,   ""    },
    { "Arp Rate",           "arp_rate",           3.0f,   0.0f,   7.0f,  true,   ""    },
    { "Arp Gate",           "arp_gate",           0.5f,   0.0f,   1.0f,  false,  ""    },
    { "Arp Swing",          "arp_swing",          0.0f,   0.0f,   0.5f,  false,  ""    },

    // ── SEQUENCER ────────────────────────────────────────────────────────────────
    { "Seq On",             "seq_on",             0.0f,   0.0f,   1.0f,  true,   ""    },
    { "Seq Playing",        "seq_playing",        0.0f,   0.0f,   1.0f,  true,   ""    },
    { "Seq Steps",          "seq_steps",         16.0f,   1.0f,  16.0f,  true,   ""    },
    { "Seq Rate",           "seq_rate",           3.0f,   0.0f,   7.0f,  true,   ""    },
    { "Seq Gate",           "seq_gate",           0.5f,   0.0f,   1.0f,  false,  ""    },
    { "Seq Swing",          "seq_swing",          0.0f,   0.0f,   0.5f,  false,  ""    },

    // ── CHORD ────────────────────────────────────────────────────────────────────
    { "Chord On",           "chord_on",           0.0f,   0.0f,   1.0f,  true,   ""    },
    { "Chord Type",         "chord_type",         0.0f,   0.0f,  15.0f,  true,   ""    },
    { "Chord Inversion",    "chord_inversion",    0.0f,   0.0f,   3.0f,  true,   ""    },

    // ── SCALE ────────────────────────────────────────────────────────────────────
    { "Scale On",           "scale_on",           0.0f,   0.0f,   1.0f,  true,   ""    },
    { "Scale Root",         "scale_root",         0.0f,   0.0f,  11.0f,  true,   ""    },
    { "Scale Type",         "scale_type",         0.0f,   0.0f,  15.0f,  true,   ""    },
};
