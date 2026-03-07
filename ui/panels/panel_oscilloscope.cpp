// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_oscilloscope.cpp
// BRIEF: Software oscilloscope — ring-buffer triggered display
// ─────────────────────────────────────────────────────────
#include "panel_oscilloscope.h"
#include "imgui.h"
#include <cstring>
#include <cmath>

namespace PanelOscilloscope {

static constexpr int BUF  = SynthEngine::OSC_BUF_SIZE;  // 2048
static constexpr int DISP = 512;   // samples to display

// Find rising zero-crossing trigger in the ring buffer
// Returns offset into ring buffer (relative to readStart) where trigger is
static int findTrigger(const float* ring, int writePos, int searchLen) {
    for (int i = 4; i < searchLen - DISP; ++i) {
        const int prev = (writePos + i - 1) & (BUF - 1);
        const int curr = (writePos + i)     & (BUF - 1);
        if (ring[prev] < 0.0f && ring[curr] >= 0.0f)
            return i;
    }
    return 0;  // fallback: no trigger found
}

void render(SynthEngine& engine) {
    ImGui::SetNextWindowSize(ImVec2(580, 220), ImGuiCond_FirstUseEver);
    ImGui::Begin("Oscilloscope");

    static float s_ring[BUF];
    static float s_display[DISP];
    static float s_scale     = 1.0f;
    static bool  s_triggered = true;
    static bool  s_autoScale = true;

    // Snapshot ring buffer from audio thread
    int writePos = 0;
    engine.getOscBuffer(s_ring, writePos);

    // Trigger: search in the oldest half of the ring buffer
    const int trigOff = s_triggered
        ? findTrigger(s_ring, writePos, BUF / 2)
        : 0;

    // Extract DISP samples starting at trigger
    float peak = 1e-6f;
    for (int i = 0; i < DISP; ++i) {
        const float v = s_ring[(writePos + trigOff + i) & (BUF - 1)];
        s_display[i] = v;
        if (v > peak) peak = v;
        if (-v > peak) peak = -v;
    }

    // Auto-scale: keep peak at ~80% of display height (only when enabled)
    if (s_autoScale) {
        if (peak > 0.01f)
            s_scale = 0.8f / peak;
        else
            s_scale = 1.0f;
    }

    // Scale for display
    float scaled[DISP];
    for (int i = 0; i < DISP; ++i)
        scaled[i] = s_display[i] * s_scale;

    // Draw background + grid
    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const float  pw    = avail.x;
    const float  ph    = 150.f;
    const ImVec2 p0    = ImGui::GetCursorScreenPos();
    ImDrawList*  dl    = ImGui::GetWindowDrawList();

    dl->AddRectFilled(p0, ImVec2(p0.x + pw, p0.y + ph),
                      IM_COL32(10, 12, 16, 255));

    // Horizontal grid lines (0%, ±50%, ±100%)
    for (int g = -2; g <= 2; ++g) {
        const float gy = p0.y + ph * 0.5f - g * ph * 0.25f;
        const ImU32 gc = (g == 0) ? IM_COL32(60, 80, 60, 255)
                                  : IM_COL32(35, 45, 35, 255);
        dl->AddLine(ImVec2(p0.x, gy), ImVec2(p0.x + pw, gy), gc);
    }
    // Vertical grid lines (quarters)
    for (int g = 1; g <= 3; ++g) {
        const float gx = p0.x + pw * g * 0.25f;
        dl->AddLine(ImVec2(gx, p0.y), ImVec2(gx, p0.y + ph),
                    IM_COL32(35, 45, 35, 255));
    }

    // Waveform — drawn as connected line segments
    const float xStep = pw / static_cast<float>(DISP - 1);
    for (int i = 1; i < DISP; ++i) {
        const float x0 = p0.x + (i - 1) * xStep;
        const float x1 = p0.x +  i      * xStep;
        const float y0 = p0.y + ph * 0.5f - scaled[i - 1] * ph * 0.5f;
        const float y1 = p0.y + ph * 0.5f - scaled[i]     * ph * 0.5f;
        dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1),
                    IM_COL32(60, 220, 80, 220), 1.5f);
    }

    // Trigger marker
    if (s_triggered) {
        dl->AddLine(ImVec2(p0.x + 2, p0.y),
                    ImVec2(p0.x + 2, p0.y + ph),
                    IM_COL32(255, 200, 0, 100));
    }

    // Reserve space for the drawn area
    ImGui::Dummy(ImVec2(pw, ph));

    // Controls
    ImGui::Checkbox("Trigger", &s_triggered);
    ImGui::SameLine(0.f, 20.f);
    ImGui::Checkbox("Auto", &s_autoScale);
    ImGui::SameLine(0.f, 20.f);
    ImGui::BeginDisabled(s_autoScale);
    ImGui::SetNextItemWidth(80.f);
    ImGui::SliderFloat("Scale", &s_scale, 0.1f, 10.f, "%.1fx");
    ImGui::EndDisabled();
    ImGui::SameLine(0.f, 20.f);
    ImGui::TextDisabled("Peak: %.3f", peak);

    ImGui::End();
}

} // namespace PanelOscilloscope
