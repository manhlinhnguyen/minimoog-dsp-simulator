// ─────────────────────────────────────────────────────────
// FILE: core/engines/dx7/dx7_algorithms.h
// BRIEF: DX7 operator routing tables (32 algorithms)
// ─────────────────────────────────────────────────────────
#pragma once

// ════════════════════════════════════════════════════════
// DX7 has 6 operators (0-5) and 32 algorithms.
// mod_src[op]: bitmask of operators that modulate op.
//   bit i set → operator i feeds into op as modulator.
// carriers: bitmask of operators that go to audio output.
// ════════════════════════════════════════════════════════

struct DX7Algorithm {
    int mod_src[6];   // which ops modulate each op
    int carriers;     // bitmask of carrier ops
};

// Convenience bit helpers
#define OP(n) (1 << (n))

constexpr DX7Algorithm DX7_ALGORITHMS[32] = {
    // Alg 1:  op1→op0, op3→op2, op5→op4;  carriers: op0,op2,op4
    { {OP(1), 0,      OP(3), 0,      OP(5), 0     }, OP(0)|OP(2)|OP(4) },
    // Alg 2:  op1→op0, op3→op2, op5+op4→op4 (feedback-style); carriers: op0,op2,op4
    { {OP(1), 0,      OP(3), 0,      OP(5)|OP(4),0}, OP(0)|OP(2)|OP(4) },
    // Alg 3:  op2→op1→op0, op4→op3; carriers: op0,op3,op5
    { {OP(1), OP(2),  0,     OP(4),  0,     0     }, OP(0)|OP(3)|OP(5) },
    // Alg 4:  op2→op1→op0, op5→op4→op3; carriers: op0,op3
    { {OP(1), OP(2),  0,     OP(4),  OP(5), 0     }, OP(0)|OP(3)       },
    // Alg 5:  op2→op1→op0, op3 op4 op5 are carriers
    { {OP(1), OP(2),  0,     0,      0,     0     }, OP(0)|OP(3)|OP(4)|OP(5) },
    // Alg 6:  op3+op4+op5→op2→op1→op0; carrier: op0
    { {OP(1), OP(2),  OP(3)|OP(4)|OP(5), 0, 0, 0 }, OP(0)             },
    // Alg 7:  op3→op2→op1→op0; carriers: op0,op4,op5
    { {OP(1), OP(2),  OP(3), 0,      0,     0     }, OP(0)|OP(4)|OP(5) },
    // Alg 8:  op3+op4→op2→op1→op0; carriers: op0,op5
    { {OP(1), OP(2),  OP(3)|OP(4), 0, 0,   0     }, OP(0)|OP(5)       },
    // Alg 9:  op5→op4→op3→op2, op1→op0; carriers: op0,op2
    { {OP(1), 0,      OP(3), OP(4),  OP(5), 0     }, OP(0)|OP(2)       },
    // Alg 10: op2→op1→op0, op5→op4→op3; carriers: op0,op3
    { {OP(1), OP(2),  0,     OP(4),  OP(5), 0     }, OP(0)|OP(3)       },
    // Alg 11: op5→op4→op3→op2→op1→op0; carrier: op0
    { {OP(1), OP(2),  OP(3), OP(4),  OP(5), 0     }, OP(0)             },
    // Alg 12: all 5 mods → op0; carrier: op0
    { {OP(1)|OP(2)|OP(3)|OP(4)|OP(5), 0, 0, 0, 0, 0}, OP(0)           },
    // Alg 13: op1→op0, op5→op4→op3→op2; carriers: op0,op2
    { {OP(1), 0,      OP(3), OP(4),  OP(5), 0     }, OP(0)|OP(2)       },
    // Alg 14: op2→op1, op5→op4→op3; op1+op3→op0; carrier: op0
    { {OP(1)|OP(3), OP(2), 0, OP(4), OP(5), 0     }, OP(0)             },
    // Alg 15: op3→op2→op1→op0, op5→op4; carriers: op0,op4
    { {OP(1), OP(2),  OP(3), 0,      OP(5), 0     }, OP(0)|OP(4)       },
    // Alg 16: op2→op1→op0, op4→op3; carriers: op0,op3,op5
    { {OP(1), OP(2),  0,     OP(4),  0,     0     }, OP(0)|OP(3)|OP(5) },
    // Alg 17: op2→op1, op4→op3, op5→op0; carriers: op0,op1,op3
    { {OP(5), OP(2),  0,     OP(4),  0,     0     }, OP(0)|OP(1)|OP(3) },
    // Alg 18: op2+op3+op4+op5→op1→op0; carrier: op0
    { {OP(1), OP(2)|OP(3)|OP(4)|OP(5), 0, 0, 0, 0 }, OP(0)            },
    // Alg 19: op3→op2→op1→op0; carriers: op0,op4,op5
    { {OP(1), OP(2),  OP(3), 0,      0,     0     }, OP(0)|OP(4)|OP(5) },
    // Alg 20: op4→op3→op2→op1→op0; carriers: op0,op5
    { {OP(1), OP(2),  OP(3), OP(4),  0,     0     }, OP(0)|OP(5)       },
    // Alg 21: op4→op3→op2→op1→op0; carrier: op0 only
    { {OP(1), OP(2),  OP(3), OP(4),  0,     0     }, OP(0)             },
    // Alg 22: op1+op2+op3+op4→op0, op5 is carrier; carriers: op0,op5
    { {OP(1)|OP(2)|OP(3)|OP(4), 0, 0, 0, 0, 0    }, OP(0)|OP(5)       },
    // Alg 23: all carriers (no modulation)
    { {0, 0, 0, 0, 0, 0                           }, 0x3F              },
    // Alg 24: op2→op0; all others carriers
    { {OP(2), 0, 0, 0, 0, 0                       }, OP(0)|OP(1)|OP(3)|OP(4)|OP(5) },
    // Alg 25: op2+op3→op0, op2+op3→op1; carriers: op0,op1,op4,op5
    { {OP(2)|OP(3), OP(2)|OP(3), 0, 0, 0, 0      }, OP(0)|OP(1)|OP(4)|OP(5) },
    // Alg 26: op3→op2→op0, op3→op1; carriers: op0,op1,op4,op5
    { {OP(2), OP(3), OP(3), 0, 0, 0               }, OP(0)|OP(1)|OP(4)|OP(5) },
    // Alg 27: op2→op0, op4→op3; carriers: op0,op1,op3,op5
    { {OP(2), 0, 0, OP(4), 0, 0                   }, OP(0)|OP(1)|OP(3)|OP(5) },
    // Alg 28: op2→op1→op0; carriers: op0,op3,op4,op5
    { {OP(1), OP(2), 0, 0, 0, 0                   }, OP(0)|OP(3)|OP(4)|OP(5) },
    // Alg 29: op2→op1→op0, op4→op3→op0; carriers: op0,op5
    { {OP(1)|OP(3), OP(2), 0, OP(4), 0, 0         }, OP(0)|OP(5)       },
    // Alg 30: op2→op0, op4+op5→op3; carriers: op0,op1,op3
    { {OP(2), 0, 0, OP(4)|OP(5), 0, 0             }, OP(0)|OP(1)|OP(3) },
    // Alg 31: op5→op4→op3→op2; carriers: op0,op1,op2
    { {0, 0, OP(3), OP(4), OP(5), 0               }, OP(0)|OP(1)|OP(2) },
    // Alg 32: all 6 operators are carriers (organ-like, no modulation)
    { {0, 0, 0, 0, 0, 0                           }, 0x3F              },
};

#undef OP
