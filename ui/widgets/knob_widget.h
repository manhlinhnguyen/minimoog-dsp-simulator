#pragma once
#include "imgui.h"
#include "shared/interfaces.h"

// Helper: draws a labeled knob and writes back to AtomicParamStore on change
namespace KnobWidget {
    bool draw(const char* label, AtomicParamStore& store,
              int paramId, float speed = 0.005f, float size = 55.f);
}
