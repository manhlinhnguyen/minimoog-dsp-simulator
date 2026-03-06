// ─────────────────────────────────────────────────────────
// FILE: ui/widgets/knob_widget.cpp
// BRIEF: Convenience wrapper around imgui-knobs
// ─────────────────────────────────────────────────────────
#include "knob_widget.h"
#include "imgui-knobs.h"
#include "shared/params.h"

bool KnobWidget::draw(const char* label, AtomicParamStore& store,
                       int paramId, float speed, float size) {
    float val = store.get(paramId);
    const ParamMeta& m = PARAM_META[paramId];
    if (ImGuiKnobs::Knob(label, &val,
                          m.minVal, m.maxVal, speed,
                          "%.2f", ImGuiKnobVariant_Wiper, size)) {
        store.set(paramId, val);
        return true;
    }
    return false;
}
