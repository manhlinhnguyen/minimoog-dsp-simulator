// ─────────────────────────────────────────────────────────
// FILE: ui/widgets/knob_widget.h
// BRIEF: Convenience wrapper around imgui-knobs for AtomicParamStore
// ─────────────────────────────────────────────────────────
#pragma once
#include "imgui.h"
#include "shared/interfaces.h"

namespace KnobWidget {
    bool draw(const char* label, AtomicParamStore& store,
              int paramId, float speed = 0.005f, float size = 55.f);
}
