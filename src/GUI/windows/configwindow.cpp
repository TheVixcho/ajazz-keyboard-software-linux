//
// configwindow.cpp
// Application Settings & Configuration Management
// Created by Vixcho on 13/10/26.
//

#include "../../constants.hpp"
#include "../../utils/i18n.hpp"
#include "../../utils/logger.hpp"
#include "../app.hpp"
#include <filesystem>
#include <imgui.h>
#include <string>

static bool settingsSaved = false;

void AjazzGUI::ConfigWindow() {
  ImGui::TextColored(ImVec4(0.35f, 0.70f, 1.00f, 1.00f), "%s",
                     tr("settings_title"));
  ImGui::Separator();
  ImGui::Spacing();

  // Section 1: Language
  ImGui::Text("%s", tr("settings_lang_label"));
  int currentLangIdx =
      (I18n::getInstance().getLanguage() == Language::ES) ? 1 : 0;
  const char *langOptions[] = {tr("settings_lang_en"), tr("settings_lang_es")};

  ImGui::SetNextItemWidth(260);
  if (ImGui::Combo("##LanguageSelector", &currentLangIdx, langOptions, 2)) {
    if (currentLangIdx == 1) {
      I18n::getInstance().setLanguage(Language::ES);
      if (GetConfig())
        GetConfig()->language = "es";
    } else {
      I18n::getInstance().setLanguage(Language::EN);
      if (GetConfig())
        GetConfig()->language = "en";
    }
    Logger::getInstance().info("Language set to: " +
                               I18n::getInstance().getLanguageCode());
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Section 2: Startup & Connection
  ImGui::Text("%s", tr("settings_startup"));
  if (GetConfig()) {
    ImGui::Checkbox(tr("settings_autoconnect"), &GetConfig()->auto_connect);
  }

  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Spacing();

  // Section 3: Configuration File
  ImGui::Text("%s", tr("settings_config_path"));
  std::string configPath = GetConfig() ? GetConfig()->getConfigFilePath()
                                       : "~/.config/ak820proLED/config.json";
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.09f, 0.11f, 1.00f));
  ImGui::BeginChild("##ConfigPathBox", ImVec2(0, 42), true);
  ImGui::TextUnformatted(configPath.c_str());
  ImGui::EndChild();
  ImGui::PopStyleColor();

  ImGui::Spacing();
  if (ImGui::Button(tr("btn_copy_path"), ImVec2(160, 32))) {
    ImGui::SetClipboardText(configPath.c_str());
  }

  ImGui::Dummy(ImVec2(0, 16));
  ImGui::Separator();
  ImGui::Spacing();

  // Section 4: Save Configuration
  if (ImGui::Button(tr("btn_save_settings"), ImVec2(240, 40))) {
    if (GetConfig()) {
      GetConfig()->saveConfig();
      settingsSaved = true;
      Logger::getInstance().success("Configuration saved to: " + configPath);
    }
  }

  if (settingsSaved) {
    ImGui::SameLine(0, 15);
    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.45f, 1.00f), "%s",
                       tr("settings_saved_msg"));
  }
}
