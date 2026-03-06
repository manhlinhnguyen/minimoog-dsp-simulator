#pragma once
#include "imgui.h"

namespace AdsrDisplay {
    // Draws an ADSR shape using ImDrawList.
    // a/d/s/r: normalized 0..1
    void draw(float a, float d, float s, float r, ImVec2 size);
}
