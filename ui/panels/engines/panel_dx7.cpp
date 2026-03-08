// ─────────────────────────────────────────────────────────
// FILE: ui/panels/panel_dx7.cpp
// BRIEF: DX7 panel — algorithm + 6 operator columns
// ─────────────────────────────────────────────────────────
#include "panel_dx7.h"
#include "imgui.h"

namespace PanelDX7 {

void renderContent(DX7Engine& engine) {
    // ── Algorithm ─────────────────────────────────────────
    {
        int algo = static_cast<int>(engine.getParam(DX7P_ALGORITHM));
        ImGui::SetNextItemWidth(100);
        if (ImGui::SliderInt("Algorithm##dx7", &algo, 1, 32))
            engine.setParam(DX7P_ALGORITHM, static_cast<float>(algo));
        ImGui::SameLine();
        float fb = engine.getParam(DX7P_FEEDBACK);
        ImGui::SetNextItemWidth(120);
        if (ImGui::SliderFloat("Feedback##dx7", &fb, 0.0f, 1.0f))
            engine.setParam(DX7P_FEEDBACK, fb);
        ImGui::SameLine();
        float vol = engine.getParam(DX7P_MASTER_VOL);
        ImGui::SetNextItemWidth(120);
        if (ImGui::SliderFloat("Volume##dx7", &vol, 0.0f, 1.0f))
            engine.setParam(DX7P_MASTER_VOL, vol);
    }

    ImGui::Separator();
    ImGui::Text("OPERATORS  (Op1=Carrier, higher ops=Modulators in most algorithms)");
    ImGui::Separator();

    // ── 6 operator columns ────────────────────────────────
    if (ImGui::BeginTable("ops_table", 6,
        ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {

        for (int op = 0; op < 6; ++op) {
            char hdr[16]; snprintf(hdr, sizeof(hdr), "Op %d", op + 1);
            ImGui::TableSetupColumn(hdr);
        }
        ImGui::TableHeadersRow();

        // Ratio row
        ImGui::TableNextRow();
        for (int op = 0; op < 6; ++op) {
            ImGui::TableSetColumnIndex(op);
            float v = engine.getParam(DX7P_OP0_RATIO + op);
            char id[16]; snprintf(id, sizeof(id), "##r%d", op);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat(id, &v, 0.5f, 18.0f, "R:%.2f"))
                engine.setParam(DX7P_OP0_RATIO + op, v);
        }

        // Fixed mode row
        ImGui::TableNextRow();
        for (int op = 0; op < 6; ++op) {
            ImGui::TableSetColumnIndex(op);
            bool fixed = engine.getParam(DX7P_OP0_FIXED_MODE + op) > 0.5f;
            char id[20]; snprintf(id, sizeof(id), "Fixed##m%d", op);
            if (ImGui::Checkbox(id, &fixed))
                engine.setParam(DX7P_OP0_FIXED_MODE + op, fixed ? 1.0f : 0.0f);
        }

        // Fixed Hz row
        ImGui::TableNextRow();
        for (int op = 0; op < 6; ++op) {
            ImGui::TableSetColumnIndex(op);
            float v = engine.getParam(DX7P_OP0_FIXED_HZ + op);
            char id[16]; snprintf(id, sizeof(id), "##fh%d", op);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat(id, &v, 20.0f, 8000.0f, "Hz:%.0f"))
                engine.setParam(DX7P_OP0_FIXED_HZ + op, v);
        }

        // Level row
        ImGui::TableNextRow();
        for (int op = 0; op < 6; ++op) {
            ImGui::TableSetColumnIndex(op);
            float v = engine.getParam(DX7P_OP0_LEVEL + op);
            char id[16]; snprintf(id, sizeof(id), "##l%d", op);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat(id, &v, 0.0f, 1.0f, "L:%.2f"))
                engine.setParam(DX7P_OP0_LEVEL + op, v);
        }

        // ADSR per row
        static const char* envNames[] = {"A","D","S","R"};
        static const int   envBase[]  = {
            DX7P_OP0_ATK, DX7P_OP0_DEC, DX7P_OP0_SUS, DX7P_OP0_REL
        };
        for (int e = 0; e < 4; ++e) {
            ImGui::TableNextRow();
            for (int op = 0; op < 6; ++op) {
                ImGui::TableSetColumnIndex(op);
                float v = engine.getParam(envBase[e] + op);
                char id[16]; snprintf(id, sizeof(id), "##%s%d", envNames[e], op);
                ImGui::SetNextItemWidth(-1);
                char fmt[16]; snprintf(fmt, sizeof(fmt), "%s:%%.2f", envNames[e]);
                if (ImGui::SliderFloat(id, &v, 0.0f, 1.0f, fmt))
                    engine.setParam(envBase[e] + op, v);
            }
        }

        // VelSens row
        ImGui::TableNextRow();
        for (int op = 0; op < 6; ++op) {
            ImGui::TableSetColumnIndex(op);
            float v = engine.getParam(DX7P_OP0_VEL + op);
            char id[16]; snprintf(id, sizeof(id), "##v%d", op);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat(id, &v, 0.0f, 1.0f, "V:%.2f"))
                engine.setParam(DX7P_OP0_VEL + op, v);
        }

        // KbdRate row
        ImGui::TableNextRow();
        for (int op = 0; op < 6; ++op) {
            ImGui::TableSetColumnIndex(op);
            float v = engine.getParam(DX7P_OP0_KBD_RATE + op);
            char id[16]; snprintf(id, sizeof(id), "##kr%d", op);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::SliderFloat(id, &v, 0.0f, 1.0f, "KR:%.2f"))
                engine.setParam(DX7P_OP0_KBD_RATE + op, v);
        }

        ImGui::EndTable();
    }

    ImGui::Text("Active voices: %d", engine.getActiveVoices());
}

void render(DX7Engine& engine) {
    if (!ImGui::Begin("Yamaha DX7")) { ImGui::End(); return; }
    renderContent(engine);
    ImGui::End();
}

} // namespace PanelDX7
