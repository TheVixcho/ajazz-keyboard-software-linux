//
// Created by taxmachine on 26/02/25.
// Modified by Vixcho on 13/10/26.
//

#include "../app.hpp"
#include <imgui.h>
#include <string>
#include <iostream>
#include <ctime>
#include "../../constants.hpp"

static char imagePathBuf[512] = "";
static std::string tftStatusMessage = "";
static int selectedSlot = 1;

void AjazzGUI::TFTScreenWindow() {
    ImGui::TextColored(ImVec4(0.35f, 0.70f, 1.00f, 1.00f), "Control de Pantalla TFT y Reloj");
    ImGui::Separator();
    ImGui::Spacing();

    // Section 1: RTC Clock Sync
    ImGui::Text("Sincronización de Hora y Fecha:");
    ImGui::TextDisabled("Sincroniza el reloj del sistema de tu PC con la pantalla TFT del teclado.");

    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::string timeStr = std::ctime(&tt);
    if (!timeStr.empty() && timeStr.back() == '\n') timeStr.pop_back();

    ImGui::Text("Hora local del sistema: %s", timeStr.c_str());

    if (ImGui::Button("Sincronizar Reloj con el Teclado", ImVec2(260, 38))) {
        try {
            if (!GetKeyboard()->isConnected()) {
                GetKeyboard()->openHandle();
            }
            GetKeyboard()->syncClockAsync(1);
            tftStatusMessage = "Reloj sincronizado exitosamente con el teclado.";
        } catch (const std::exception& e) {
            tftStatusMessage = "Error sincronizando reloj: " + std::string(e.what());
        }
    }

    ImGui::Dummy(ImVec2(0, 16));
    ImGui::Separator();
    ImGui::Spacing();

    // Section 2: LCD Image & GIF Upload
    ImGui::Text("Carga de Imágenes y GIF (240x135 px):");
    ImGui::TextDisabled("Soporta imágenes PNG y animaciones. Las imágenes se ajustan automáticamente a 240x135.");

    ImGui::InputText("Ruta del archivo (PNG / GIF)", imagePathBuf, sizeof(imagePathBuf));
    ImGui::Combo("Ranura (Slot)", &selectedSlot, "Ranura 1\0Ranura 2\0Ranura 3\0");

    ImGui::Spacing();
    if (ImGui::Button("Subir a Pantalla TFT", ImVec2(220, 40))) {
        if (strlen(imagePathBuf) == 0) {
            tftStatusMessage = "Por favor introduce la ruta de un archivo de imagen existente.";
        } else {
            try {
                if (!GetKeyboard()->isConnected()) {
                    GetKeyboard()->openHandle();
                }
                GetKeyboard()->uploadImageToLCDAsync(imagePathBuf, selectedSlot + 1);
                tftStatusMessage = "Carga iniciada hacia la pantalla TFT...";
            } catch (const std::exception& e) {
                tftStatusMessage = "Error al subir imagen: " + std::string(e.what());
            }
        }
    }

    if (!tftStatusMessage.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.40f, 0.90f, 0.50f, 1.00f), "%s", tftStatusMessage.c_str());
    }

    ImGui::Dummy(ImVec2(0, 16));
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "Nota técnica:");
    ImGui::TextWrapped("El microcontrolador de la pantalla TFT está conectado directamente al bus USB. Para transferir imágenes o sincronizar hora, se recomienda tener el interruptor de hardware en modo USB (cable) o el receptor 2.4GHz enlazado.");
}
