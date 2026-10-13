//
// Created by taxmachine on 23/02/25.
// Modified by Vixcho on 13/10/26.
//

#include "../app.hpp"
#include <future>

#include <imgui.h>
#include <iostream>

#include "../../constants.hpp"
#include "../../utils/rgb.hpp"

static int selectedMode = 1;
static int selectedTime = 0;
static int selectedDirection = 0;

void AjazzGUI::changeMode() {
  uint8_t r, g, b;
  const auto keyboard_color = RGB(GetConfig()->keyboard_rgb);
  keyboard_color.toRGB(&r, &g, &b);

  try {
    if (!GetKeyboard()->isConnected()) {
      GetKeyboard()->openHandle();
    }

    GetKeyboard()->setModeAsync(GetConfig()->mode, r, g, b,
                                GetConfig()->rainbow, GetConfig()->brightness,
                                GetConfig()->speed, GetConfig()->direction);

    GetKeyboard()->setSleepTimeAsync(GetConfig()->sleep_delay);
  } catch (const std::exception &e) {
    std::cerr << "Error aplicando modo: " << e.what() << std::endl;
  }
}

void AjazzGUI::ModeWindow() {
  ImGui::TextColored(ImVec4(0.35f, 0.70f, 1.00f, 1.00f),
                     "Configuración de Iluminación RGB");
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::Text("Modo de Iluminación:");
  int col = 0;
  for (const auto &[mode, name] : LIGHT_MODES) {
    if (ImGui::RadioButton(name, &selectedMode, mode)) {
      GetConfig()->mode = mode;
    }
    col++;
    if (col % 4 != 0) {
      ImGui::SameLine(0, 16);
    }
  }

  ImGui::Dummy(ImVec2(0, 12));
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::Text("Color y Opciones:");
  ImGui::Checkbox("Modo Arcoíris (Multicolor)", &GetConfig()->rainbow);

  if (!GetConfig()->rainbow) {
    ImGui::SameLine(0, 20);
    ImGui::ColorEdit3("Color Estático##Picker", GetConfig()->keyboard_rgb,
                      ImGuiColorEditFlags_NoAlpha);
  }

  ImGui::Dummy(ImVec2(0, 12));
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::Text("Ajustes de Velocidad y Brillo:");
  ImGui::SliderInt("Brillo", &GetConfig()->brightness, 0, MAX_BRIGHTNESS);
  ImGui::SliderInt("Velocidad", &GetConfig()->speed, 0, MAX_SPEED);

  ImGui::Dummy(ImVec2(0, 12));
  ImGui::Separator();
  ImGui::Spacing();

  ImGui::Text("Tiempo de Suspensión (Auto-Sleep):");
  int sCol = 0;
  for (const auto &[mode, mode_name] : SLEEP_DELAY) {
    if (ImGui::RadioButton(mode_name, &selectedTime, mode)) {
      GetConfig()->sleep_delay = mode;
    }
    sCol++;
    if (sCol < static_cast<int>(SLEEP_DELAY.size())) {
      ImGui::SameLine(0, 16);
    }
  }

  if (DIRECTIONS_PER_MODE.contains(GetConfig()->mode)) {
    ImGui::Dummy(ImVec2(0, 12));
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text("Dirección de la Animación:");
    const auto &directions = DIRECTIONS_PER_MODE.at(GetConfig()->mode);
    for (size_t i = 0; i < directions.size(); i++) {
      auto dir = directions[i];
      if (ImGui::RadioButton(DIRECTIONS.at(dir), &selectedDirection, dir)) {
        GetConfig()->direction = dir;
      }
      if (i + 1 < directions.size()) {
        ImGui::SameLine(0, 16);
      }
    }
  }

  ImGui::Dummy(ImVec2(0, 16));
  if (ImGui::Button("Aplicar Iluminación", ImVec2(180, 40))) {
    this->changeMode();
  }
}
