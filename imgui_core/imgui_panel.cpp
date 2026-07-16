// CTF|认证
#include "imgui_panel.hpp"
#include "imgui.h"

void draw_delta_panel(const FrameSnapshot& f, bool* open) {
    if (!ImGui::Begin("Delta Kernel", open)) { ImGui::End(); return; }
    ImGui::Text("PID: %d", f.pid);
    ImGui::Text("libUE4: 0x%llX", static_cast<unsigned long long>(f.libue4));
    ImGui::Text("Snapshot: %llu", static_cast<unsigned long long>(f.sequence));
    ImGui::Separator();
    ImGui::Text("Entities: %d", static_cast<int>(f.entities.size()));
    if (ImGui::BeginTable("entities", 6, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        for (const char* h : {"Actor", "Team", "Camp", "HP", "State", "Pos"}) { ImGui::TableNextColumn(); ImGui::TextUnformatted(h); }
        for (const auto& e : f.entities) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%llX", static_cast<unsigned long long>(e.actor));
            ImGui::TableNextColumn(); ImGui::Text("%d", e.team);
            ImGui::TableNextColumn(); ImGui::Text("%d", e.camp);
            ImGui::TableNextColumn(); ImGui::Text("%.0f/%.0f", e.health, e.max_health);
            ImGui::TableNextColumn(); ImGui::Text("%d%s", e.live_status, e.dead ? " dead" : "");
            ImGui::TableNextColumn(); ImGui::Text("%.1f %.1f %.1f", e.location.x, e.location.y, e.location.z);
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

