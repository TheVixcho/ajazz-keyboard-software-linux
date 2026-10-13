//
// Created by taxmachine on 26/02/25.
// Modified by Vixcho on 13/10/26.
//

#include "../app.hpp"
#include <imgui.h>
#include "../../constants.hpp"

void AjazzGUI::ThemeEditorWindow() {
    ImGui::TextColored(ImVec4(0.35f, 0.70f, 1.00f, 1.00f), "Personalización del Tema Visual");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::BeginGroup();
    if (ImGui::SliderFloat("Redondeo de Ventana", &GetConfig()->window_roundness, 0, 15)) {
        ImGui::GetStyle().WindowRounding = GetConfig()->window_roundness;
    }

    if (ImGui::SliderFloat("Redondeo de Marcos", &GetConfig()->frame_roundness, 0, 15)) {
        ImGui::GetStyle().FrameRounding = GetConfig()->frame_roundness;
    }

    if (ImGui::SliderFloat("Espaciado de Marcos (X)", &GetConfig()->frame_padding_x, 0, 15)) {
        ImGui::GetStyle().FramePadding.x = GetConfig()->frame_padding_x;
    }

    if (ImGui::SliderFloat("Espaciado de Marcos (Y)", &GetConfig()->frame_padding_y, 0, 15)) {
        ImGui::GetStyle().FramePadding.y = GetConfig()->frame_padding_y;
    }
    ImGui::EndGroup();
}
