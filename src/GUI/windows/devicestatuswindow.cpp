//
// Created by Vixcho on 13/10/26.
//

#include "../app.hpp"
#include <imgui.h>
#include <string>
#include <filesystem>
#include "../../constants.hpp"
#include "../../utils/logger.hpp"

static bool commandCopied = false;

void AjazzGUI::DeviceStatusWindow() {
    ImGui::TextColored(ImVec4(0.35f, 0.70f, 1.00f, 1.00f), "Diagnóstico y Estado de Conexión USB / 2.4GHz");
    ImGui::Separator();
    ImGui::Spacing();

    bool connected = GetKeyboard() && GetKeyboard()->isConnected();
    bool wireless = GetKeyboard() && GetKeyboard()->isWireless();

    if (connected) {
        ImGui::TextColored(ImVec4(0.25f, 0.85f, 0.45f, 1.00f), "● Teclado Conectado y Listo para Comunicación");
        ImGui::BulletText("Tipo de conexión: %s", wireless ? "2.4GHz Dongle (Inalámbrico)" : "Cable USB (Directo)");
        ImGui::BulletText("Vendor ID (VID): 0x0c45 (SONiX / Microdia)");
        ImGui::BulletText("Product ID (PID): %s", wireless ? "0xfdfd (Wireless 2.4G Receiver)" : "0x8009 (AK820 Pro Wired)");
    } else {
        ImGui::TextColored(ImVec4(0.90f, 0.35f, 0.35f, 1.00f), "○ No se ha podido comunicar con el teclado");
        if (GetKeyboard()) {
            std::string err = GetKeyboard()->getLastError();
            if (!err.empty()) {
                ImGui::TextColored(ImVec4(0.95f, 0.65f, 0.25f, 1.00f), "Detalle: %s", err.c_str());
            }
        }
    }

    ImGui::Spacing();
    if (ImGui::Button("Intentar Conexión Nuevamente", ImVec2(240, 38))) {
        if (GetKeyboard()) {
            Logger::getInstance().info("Reintentando conexión con el teclado...");
            bool res = GetKeyboard()->openHandle();
            if (res) {
                Logger::getInstance().success("Teclado conectado exitosamente: " + GetKeyboard()->getConnectionStatus());
            } else {
                Logger::getInstance().error("Fallo al conectar con el teclado: " + GetKeyboard()->getLastError());
            }
        }
    }

    ImGui::Dummy(ImVec2(0, 16));
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.90f, 0.80f, 0.30f, 1.00f), "Permisos de Dispositivo en Linux (Reglas udev):");
    ImGui::TextWrapped("En sistemas Linux, acceder directamente a los dispositivos HID (/dev/hidraw*) requiere permisos. Si el programa no detecta el teclado o muestra error de permisos, instala la regla udev ejecutando el siguiente comando:");

    ImGui::Spacing();
    std::filesystem::path fullPath = std::filesystem::current_path() / "99-ajazz-ak820pro.rules";
    std::string udevCommand = "sudo cp \"" + fullPath.string() + "\" /etc/udev/rules.d/ && sudo udevadm control --reload-rules && sudo udevadm trigger";

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.07f, 0.08f, 1.00f));
    ImGui::BeginChild("##CodeBlock", ImVec2(0, 52), true);
    ImGui::TextUnformatted(udevCommand.c_str());
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();
    if (ImGui::Button("Copiar Comando al Portapapeles", ImVec2(260, 36))) {
        ImGui::SetClipboardText(udevCommand.c_str());
        commandCopied = true;
        Logger::getInstance().info("Comando con ruta absoluta copiado al portapapeles.");
    }

    if (commandCopied) {
        ImGui::SameLine(0, 15);
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "¡Copiado con éxito al portapapeles!");
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Una vez instalada la regla udev, desconecta y vuelve a conectar el receptor 2.4GHz o el cable USB.");
}
