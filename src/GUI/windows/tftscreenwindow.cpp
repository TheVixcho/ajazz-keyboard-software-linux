//
// Created by taxmachine on 26/02/25.
// Modified by Vixcho on 13/10/26.
//

#include "../../constants.hpp"
#include "../app.hpp"
#include <cstring>
#include <ctime>
#include <gtk/gtk.h>
#include <imgui.h>
#include <iostream>
#include <string>

static char imagePathBuf[512] = "";
static std::string tftStatusMessage = "";
static int selectedSlot = 0;

void AjazzGUI::TFTScreenWindow() {
  ImGui::TextColored(ImVec4(0.35f, 0.70f, 1.00f, 1.00f),
                     "Control de Pantalla TFT y Reloj");
  ImGui::Separator();
  ImGui::Spacing();

  // Section 1: RTC Clock Sync
  ImGui::Text("Sincronización de Hora y Fecha:");
  ImGui::TextDisabled("Sincroniza el reloj del sistema de tu PC con la "
                      "pantalla TFT del teclado.");

  auto now = std::chrono::system_clock::now();
  std::time_t tt = std::chrono::system_clock::to_time_t(now);
  std::string timeStr = std::ctime(&tt);
  if (!timeStr.empty() && timeStr.back() == '\n')
    timeStr.pop_back();

  ImGui::Text("Hora local del sistema: %s", timeStr.c_str());

  if (ImGui::Button("Sincronizar Reloj con el Teclado", ImVec2(260, 38))) {
    try {
      if (!GetKeyboard()->isConnected()) {
        GetKeyboard()->openHandle();
      }
      GetKeyboard()->syncClockAsync(1);
      tftStatusMessage = "Reloj sincronizado exitosamente con el teclado.";
    } catch (const std::exception &e) {
      tftStatusMessage = "Error sincronizando reloj: " + std::string(e.what());
    }
  }

  ImGui::Dummy(ImVec2(0, 16));
  ImGui::Separator();
  ImGui::Spacing();

  // Section 2: LCD Image & GIF Upload
  ImGui::Text("Carga de Imágenes y GIF (240x135 px):");
  ImGui::TextDisabled("Soporta imágenes PNG, JPG y animaciones GIF. Las "
                      "imágenes se ajustan automáticamente a 240x135.");

  bool isWired = GetKeyboard() && GetKeyboard()->isConnected() &&
                 !GetKeyboard()->isWireless();
  bool dispReady = GetKeyboard() && GetKeyboard()->isDisplayAvailable();

  if (!isWired) {
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.95f, 0.55f, 0.20f, 1.0f));
    ImGui::BeginChild("##TFTNotice", ImVec2(0, 48), true);
    ImGui::TextColored(ImVec4(0.95f, 0.70f, 0.25f, 1.0f),
                       "Aviso de Hardware: Para transferir imágenes o GIFs a "
                       "la pantalla TFT, conecta el teclado");
    ImGui::TextColored(ImVec4(0.95f, 0.70f, 0.25f, 1.0f),
                       "mediante el cable USB directo y pon el interruptor "
                       "trasero en modo USB (no disponible en 2.4G).");
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::Spacing();
  } else if (dispReady) {
    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.0f),
                       "[OK] Canal de pantalla TFT preparado para "
                       "transferencia por cable USB.");
    ImGui::Spacing();
  }

  ImGui::SetNextItemWidth(450);
  ImGui::InputText("Ruta del archivo##TFTPath", imagePathBuf,
                   sizeof(imagePathBuf));
  ImGui::SameLine();
  if (ImGui::Button("Examinar...##TFTBrowse", ImVec2(100, 0))) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Seleccionar Imagen o Animación GIF", nullptr,
        GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancelar", GTK_RESPONSE_CANCEL,
        "_Abrir", GTK_RESPONSE_ACCEPT, nullptr);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter,
                             "Imágenes y Animaciones (*.gif, *.png, *.jpg)");
    gtk_file_filter_add_pattern(filter, "*.gif");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
      char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      if (filename) {
        strncpy(imagePathBuf, filename, sizeof(imagePathBuf) - 1);
        imagePathBuf[sizeof(imagePathBuf) - 1] = '\0';
        g_free(filename);
      }
    }
    gtk_widget_destroy(dialog);
  }

  ImGui::SetNextItemWidth(200);
  ImGui::Combo("Ranura de Almacenamiento (Slot)", &selectedSlot,
               "Ranura 1\0Ranura 2\0Ranura 3\0");

  ImGui::Spacing();
  if (ImGui::Button("Subir a Pantalla TFT", ImVec2(220, 40))) {
    if (strlen(imagePathBuf) == 0) {
      tftStatusMessage =
          "Por favor selecciona o introduce la ruta de un archivo existente.";
    } else {
      try {
        if (!GetKeyboard()->isConnected()) {
          GetKeyboard()->openHandle();
        }
        GetKeyboard()->uploadImageToLCDAsync(imagePathBuf, selectedSlot + 1);
        tftStatusMessage = "Transferencia iniciada. Consulta la pestaña Logs "
                           "para ver el progreso.";
      } catch (const std::exception &e) {
        tftStatusMessage = "Error al subir imagen: " + std::string(e.what());
      }
    }
  }

  if (!tftStatusMessage.empty()) {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.40f, 0.90f, 0.50f, 1.00f), "%s",
                       tftStatusMessage.c_str());
  }

  ImGui::Dummy(ImVec2(0, 16));
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "Nota técnica:");
  ImGui::TextWrapped(
      "El microcontrolador de la pantalla TFT está conectado directamente al "
      "bus USB. Para transferir imágenes o sincronizar hora, se requiere tener "
      "el interruptor de hardware en modo USB (cable directo).");
}
