//
// Created by taxmachine on 16/02/25.
// Modified by Vixcho on 13/10/26.
//

#include "ak820pro.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <lodepng.h>
#include <thread>

#include "../utils/logger.hpp"
#include "../utils/utils.hpp"

static uint8_t START[PACKET_LENGTH] = {
    COMMAND_PREFIX, START_COMMAND, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t SAVE[PACKET_LENGTH] = {
    COMMAND_PREFIX, SAVE_COMMAND, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static uint8_t FINISH[PACKET_LENGTH] = {
    COMMAND_PREFIX, FINISH_COMMAND, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t START_MODE[PACKET_LENGTH] = {
    COMMAND_PREFIX, MODE_COMMAND, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t START_SLEEP[PACKET_LENGTH] = {
    COMMAND_PREFIX, 0x17, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t START_TIME[PACKET_LENGTH] = {
    COMMAND_PREFIX, TIME_COMMAND, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

AK820Pro::AK820Pro(const uint16_t vid, const uint16_t pid) {
  hid_init();
  this->vid = vid;
  this->pid = pid;
  this->openHandle();
}

AK820Pro::~AK820Pro() {
  this->closeHandle();
  hid_exit();
}

bool AK820Pro::openHandle() {
  if (this->handle) {
    return true;
  }

  Logger::getInstance().info("Enumerando dispositivos HID (VID 0x0c45)...");
  struct hid_device_info *devs = hid_enumerate(this->vid, 0);
  if (!devs) {
    this->lastErrorMessage =
        "No se encontraron dispositivos HID con VID 0x0c45";
    Logger::getInstance().warning(this->lastErrorMessage);
    return false;
  }

  struct hid_device_info *bestCandidate = nullptr;
  int bestScore = -1;

  for (struct hid_device_info *cur = devs; cur != nullptr; cur = cur->next) {
    bool isWired = (cur->product_id == AK820PRO_PID);
    bool isWireless = (cur->product_id == AK820PRO_PID_WIRELESS);

    if (!isWired && !isWireless) {
      continue;
    }

    std::string modeStr =
        isWireless ? "2.4GHz Dongle (0xfdfd)" : "Wired (0x8009)";
    std::string pathStr = cur->path ? cur->path : "null";
    Logger::getInstance().info("Detectado " + modeStr + " en " + pathStr +
                               " (Interfaz " +
                               std::to_string(cur->interface_number) + ")");

    int score = 0;
    if (isWireless) {
      if (cur->interface_number == 4 || cur->usage_page == 0xff59) {
        score = 100;
      } else if (cur->interface_number == 3 || cur->usage_page == 0xff60) {
        score = 80;
      } else {
        score = 10;
      }
    } else {
      if (cur->interface_number == 3 || cur->usage_page == 0xff13) {
        score = 100;
      } else if (cur->interface_number == 2) {
        score = 80;
      } else {
        score = 10;
      }
    }

    if (score > bestScore) {
      bestScore = score;
      bestCandidate = cur;
    }
  }

  if (bestCandidate && bestCandidate->path) {
    this->handle = hid_open_path(bestCandidate->path);
    if (this->handle) {
      this->activePath = bestCandidate->path;
      this->wirelessMode = (bestCandidate->product_id == AK820PRO_PID_WIRELESS);
      this->lastErrorMessage.clear();
      Logger::getInstance().success(
          "Conexión establecida con " +
          std::string(this->wirelessMode ? "2.4GHz Dongle" : "Cable USB") +
          " en " + this->activePath);
      hid_free_enumeration(devs);
      return true;
    } else {
      Logger::getInstance().error(
          "hid_open_path falló en el candidato preferido (" +
          std::string(bestCandidate->path) +
          "). Posible falta de permisos udev.");
    }
  }

  // Fallback: try opening each matching device until one succeeds
  for (struct hid_device_info *cur = devs; cur != nullptr; cur = cur->next) {
    if ((cur->product_id == AK820PRO_PID ||
         cur->product_id == AK820PRO_PID_WIRELESS) &&
        cur->path) {
      this->handle = hid_open_path(cur->path);
      if (this->handle) {
        this->activePath = cur->path;
        this->wirelessMode = (cur->product_id == AK820PRO_PID_WIRELESS);
        this->lastErrorMessage.clear();
        Logger::getInstance().success("Conexión alternativa exitosa en " +
                                      this->activePath);
        hid_free_enumeration(devs);
        return true;
      }
    }
  }

  hid_free_enumeration(devs);
  this->lastErrorMessage = "No se pudo abrir el dispositivo. Comprueba los "
                           "permisos de lectura/escritura (reglas udev).";
  Logger::getInstance().error(this->lastErrorMessage);
  return false;
}

void AK820Pro::closeHandle() {
  if (this->handle) {
    hid_close(this->handle);
    this->handle = nullptr;
    Logger::getInstance().info("Conexión con el teclado cerrada.");
  }
}

bool AK820Pro::isConnected() const { return (this->handle != nullptr); }

bool AK820Pro::isWireless() const { return this->wirelessMode; }

std::string AK820Pro::getConnectionStatus() const {
  if (!this->handle) {
    return "Desconectado";
  }
  return this->wirelessMode ? "Conectado (2.4GHz Dongle)"
                            : "Conectado (Cable USB)";
}

std::string AK820Pro::getLastError() const { return this->lastErrorMessage; }

bool AK820Pro::sendFeatureReport(const uint8_t *data, size_t len) const {
  if (!this->handle)
    return false;
  int res = hid_send_feature_report(this->handle, data, len);
  std::this_thread::sleep_for(std::chrono::milliseconds(35));
  if (res < 0) {
    Logger::getInstance().error("hid_send_feature_report falló (código " +
                                std::to_string(res) + ")");
  }
  return (res >= 0);
}

bool AK820Pro::getFeatureReport(uint8_t *data, size_t len) const {
  if (!this->handle)
    return false;
  int res = hid_get_feature_report(this->handle, data, len);
  std::this_thread::sleep_for(std::chrono::milliseconds(35));
  return (res >= 0);
}

void AK820Pro::executeHIDCommand(const std::vector<uint8_t *> &commands) const {
  if (!this->handle) {
    throw std::runtime_error("Dispositivo HID no conectado");
  }

  hid_set_nonblocking(this->handle, 0);
  uint8_t received[PACKET_LENGTH] = {0};

  for (const auto *cmd : commands) {
    hid_send_feature_report(this->handle, cmd, PACKET_LENGTH);
    std::this_thread::sleep_for(std::chrono::milliseconds(35));
    hid_get_feature_report(this->handle, received, PACKET_LENGTH);
    std::this_thread::sleep_for(std::chrono::milliseconds(35));
  }
}

void AK820Pro::setColor(const uint8_t r, const uint8_t g,
                        const uint8_t b) const {
  this->setMode(STATIC, r, g, b, false, MAX_BRIGHTNESS, MAX_SPEED, LEFT);
}

void AK820Pro::setMode(const LightingMode mode, const uint8_t r,
                       const uint8_t g, const uint8_t b, const bool rainbow,
                       const int brightness, const int speed,
                       const Direction direction) const {
  if (!this->handle && !const_cast<AK820Pro *>(this)->openHandle()) {
    std::string err =
        "Dispositivo HID no disponible: " + this->lastErrorMessage;
    Logger::getInstance().error(err);
    throw std::runtime_error(err);
  }

  if (brightness < 0 || brightness > MAX_BRIGHTNESS) {
    throw std::runtime_error("Brightness level cannot be over 5 or under 0");
  }

  if (speed < 0 || speed > MAX_SPEED) {
    throw std::runtime_error("Speed cannot be over 5 or under 0");
  }

  Logger::getInstance().info("Enviando paquete de modo " +
                             std::to_string(mode) +
                             " (Brillo: " + std::to_string(brightness) +
                             ", Vel: " + std::to_string(speed) + ")");

  ModePacket packet{};
  packet.mode = static_cast<uint8_t>(mode);
  packet.r = r;
  packet.g = g;
  packet.b = b;
  packet.options.rainbow = rainbow;
  packet.options.brightness = static_cast<uint8_t>(brightness);
  packet.options.speed = static_cast<uint8_t>(speed);
  packet.options.direction = static_cast<uint8_t>(direction);
  packet.delimiter = 0xaa55;

  uint8_t data[PACKET_LENGTH];
  memcpy(data, &packet, PACKET_LENGTH);

  uint8_t received[PACKET_LENGTH] = {0};
  this->sendFeatureReport(START, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(START_MODE, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(data, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(FINISH, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  Logger::getInstance().success("Modo de iluminación aplicado con éxito.");
}

std::future<void> AK820Pro::setModeAsync(const LightingMode mode,
                                         const uint8_t r, const uint8_t g,
                                         const uint8_t b, const bool rainbow,
                                         const int brightness, const int speed,
                                         const Direction direction) const {
  return std::async(std::launch::async, [this, mode, r, g, b, rainbow,
                                         brightness, speed, direction] {
    try {
      this->setMode(mode, r, g, b, rainbow, brightness, speed, direction);
    } catch (const std::exception &e) {
      Logger::getInstance().error("Error en setModeAsync: " +
                                  std::string(e.what()));
    }
  });
}

void AK820Pro::setPerKeyRGB(const std::vector<KeyColor> &keyColors) const {
  if (!this->handle && !const_cast<AK820Pro *>(this)->openHandle()) {
    std::string err =
        "Dispositivo HID no disponible: " + this->lastErrorMessage;
    Logger::getInstance().error(err);
    throw std::runtime_error(err);
  }

  Logger::getInstance().info(
      "Preparando datos de 144 posiciones para LEDs individuales...");

  uint8_t buffer[576];
  memset(buffer, 0, sizeof(buffer));

  for (size_t i = 0; i < 144; i++) {
    buffer[i * 4] = static_cast<uint8_t>(i);
    buffer[i * 4 + 1] = 0;
    buffer[i * 4 + 2] = 0;
    buffer[i * 4 + 3] = 0;
  }

  for (const auto &kc : keyColors) {
    if (kc.keyIndex < 144) {
      buffer[kc.keyIndex * 4] = kc.keyIndex;
      buffer[kc.keyIndex * 4 + 1] = kc.r;
      buffer[kc.keyIndex * 4 + 2] = kc.g;
      buffer[kc.keyIndex * 4 + 3] = kc.b;
    }
  }

  uint8_t cmdCustom[PACKET_LENGTH] = {COMMAND_PREFIX,
                                      CUSTOM_LIGHT_COMMAND,
                                      0x00,
                                      0x00,
                                      0x00,
                                      0x00,
                                      0x00,
                                      0x00,
                                      0x09};
  uint8_t received[PACKET_LENGTH] = {0};

  this->sendFeatureReport(START, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(cmdCustom, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  for (int p = 0; p < 9; p++) {
    uint8_t packet[PACKET_LENGTH] = {0};
    memcpy(packet, buffer + (p * 64), 64);
    this->sendFeatureReport(packet, PACKET_LENGTH);
    this->getFeatureReport(received, PACKET_LENGTH);
  }

  this->sendFeatureReport(SAVE, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(FINISH, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  // Switch lighting mode to CUSTOM (0x80)
  this->setMode(CUSTOM, 255, 255, 255, false, MAX_BRIGHTNESS, MAX_SPEED, LEFT);
  Logger::getInstance().success(
      "LEDs individuales enviados y modo CUSTOM activado.");
}

std::future<void>
AK820Pro::setPerKeyRGBAsync(const std::vector<KeyColor> &keyColors) const {
  return std::async(std::launch::async, [this, keyColors] {
    try {
      this->setPerKeyRGB(keyColors);
    } catch (const std::exception &e) {
      Logger::getInstance().error("Error en setPerKeyRGBAsync: " +
                                  std::string(e.what()));
    }
  });
}

void AK820Pro::setSleepTime(const LightSleepTime sleep_time) const {
  if (!this->handle && !const_cast<AK820Pro *>(this)->openHandle()) {
    throw std::runtime_error("Dispositivo HID no disponible");
  }

  uint8_t data[PACKET_LENGTH] = {0};
  data[8] = static_cast<uint8_t>(sleep_time);
  data[PACKET_LENGTH - 1] = 0x55;
  data[PACKET_LENGTH - 2] = 0xaa;

  uint8_t received[PACKET_LENGTH] = {0};
  this->sendFeatureReport(START, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(START_SLEEP, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(data, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);
}

std::future<void>
AK820Pro::setSleepTimeAsync(const LightSleepTime sleep_time) const {
  return std::async(std::launch::async, [this, sleep_time] {
    try {
      this->setSleepTime(sleep_time);
    } catch (const std::exception &e) {
      Logger::getInstance().error("Error en setSleepTimeAsync: " +
                                  std::string(e.what()));
    }
  });
}

void AK820Pro::syncClock(int slot) const {
  if (!this->handle && !const_cast<AK820Pro *>(this)->openHandle()) {
    std::string err =
        "Dispositivo HID no disponible: " + this->lastErrorMessage;
    Logger::getInstance().error(err);
    throw std::runtime_error(err);
  }

  Logger::getInstance().info("Sincronizando reloj RTC con la pantalla...");
  auto now = std::chrono::system_clock::now();
  std::time_t tt = std::chrono::system_clock::to_time_t(now);
  std::tm local_tm{};
  localtime_r(&tt, &local_tm);

  uint8_t timeData[PACKET_LENGTH] = {0};
  timeData[0] = 0x00;
  timeData[1] = static_cast<uint8_t>(slot);
  timeData[2] = 0x5a;
  timeData[3] = static_cast<uint8_t>(local_tm.tm_year % 100);
  timeData[4] = static_cast<uint8_t>(local_tm.tm_mon + 1);
  timeData[5] = static_cast<uint8_t>(local_tm.tm_mday);
  timeData[6] = static_cast<uint8_t>(local_tm.tm_hour);
  timeData[7] = static_cast<uint8_t>(local_tm.tm_min);
  timeData[8] = static_cast<uint8_t>(local_tm.tm_sec);
  timeData[9] = 0x00;
  timeData[10] = 0x04;
  timeData[62] = 0xaa;
  timeData[63] = 0x55;

  uint8_t received[PACKET_LENGTH] = {0};
  this->sendFeatureReport(START, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(START_TIME, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(timeData, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(SAVE, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  Logger::getInstance().success("Reloj RTC sincronizado exitosamente.");
}

std::future<void> AK820Pro::syncClockAsync(int slot) const {
  return std::async(std::launch::async, [this, slot] {
    try {
      this->syncClock(slot);
    } catch (const std::exception &e) {
      Logger::getInstance().error("Error en syncClockAsync: " +
                                  std::string(e.what()));
    }
  });
}

void AK820Pro::uploadImageToLCD(const std::string &path, int slot) const {
  if (!std::filesystem::exists(path)) {
    std::string err = "El archivo de imagen no existe: " + path;
    Logger::getInstance().error(err);
    throw std::runtime_error(err);
  }

  if (!this->handle && !const_cast<AK820Pro *>(this)->openHandle()) {
    std::string err =
        "Dispositivo HID no disponible: " + this->lastErrorMessage;
    Logger::getInstance().error(err);
    throw std::runtime_error(err);
  }

  Logger::getInstance().info("Cargando archivo de imagen: " + path);
  std::vector<unsigned char> png;
  std::vector<unsigned char> rawImage;
  unsigned width = 0, height = 0;

  unsigned error = lodepng::load_file(png, path);
  if (error) {
    std::string err =
        "Error leyendo imagen: " + std::string(lodepng_error_text(error));
    Logger::getInstance().error(err);
    throw std::runtime_error(err);
  }
  error = lodepng::decode(rawImage, width, height, png);
  if (error) {
    std::string err =
        "Error decodificando imagen: " + std::string(lodepng_error_text(error));
    Logger::getInstance().error(err);
    throw std::runtime_error(err);
  }

  Logger::getInstance().info("Imagen decodificada (" + std::to_string(width) +
                             "x" + std::to_string(height) +
                             "). Ajustando a 240x135 RGB565...");

  std::vector<uint8_t> frameRGB565(TFT_FRAME_BYTES, 0);

  for (uint32_t y = 0; y < TFT_HEIGHT; y++) {
    for (uint32_t x = 0; x < TFT_WIDTH; x++) {
      uint32_t srcX = (x * width) / TFT_WIDTH;
      uint32_t srcY = (y * height) / TFT_HEIGHT;
      uint32_t srcIdx = (srcY * width + srcX) * 4;

      uint8_t r = rawImage[srcIdx];
      uint8_t g = rawImage[srcIdx + 1];
      uint8_t b = rawImage[srcIdx + 2];

      uint16_t pixel = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
      uint32_t dstIdx = (y * TFT_WIDTH + x) * 2;
      frameRGB565[dstIdx] = static_cast<uint8_t>(pixel & 0xFF);
      frameRGB565[dstIdx + 1] = static_cast<uint8_t>((pixel >> 8) & 0xFF);
    }
  }

  std::vector<uint8_t> payload(TFT_HEADER_BYTES + TFT_FRAME_BYTES, 0xFF);
  payload[0] = 1;
  payload[1] = 50;
  memcpy(payload.data() + TFT_HEADER_BYTES, frameRGB565.data(),
         TFT_FRAME_BYTES);

  uint16_t totalChunks = static_cast<uint16_t>(
      (payload.size() + IMAGE_CHUNK_SIZE - 1) / IMAGE_CHUNK_SIZE);
  Logger::getInstance().info("Enviando " + std::to_string(totalChunks) +
                             " bloques (4096 bytes) a la pantalla TFT...");

  uint8_t cmdImage[PACKET_LENGTH] = {0};
  cmdImage[0] = COMMAND_PREFIX;
  cmdImage[1] = IMAGE_COMMAND;
  cmdImage[2] = static_cast<uint8_t>(slot);
  cmdImage[8] = static_cast<uint8_t>(totalChunks & 0xFF);
  cmdImage[9] = static_cast<uint8_t>((totalChunks >> 8) & 0xFF);

  uint8_t received[PACKET_LENGTH] = {0};
  this->sendFeatureReport(START, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  this->sendFeatureReport(cmdImage, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  for (uint16_t i = 0; i < totalChunks; i++) {
    size_t offset = i * IMAGE_CHUNK_SIZE;
    size_t len = std::min<size_t>(IMAGE_CHUNK_SIZE, payload.size() - offset);
    std::vector<uint8_t> chunk(IMAGE_CHUNK_SIZE, 0);
    memcpy(chunk.data(), payload.data() + offset, len);

    hid_write(this->handle, chunk.data(), IMAGE_CHUNK_SIZE);
    std::this_thread::sleep_for(std::chrono::milliseconds(35));
  }

  this->sendFeatureReport(SAVE, PACKET_LENGTH);
  this->getFeatureReport(received, PACKET_LENGTH);

  Logger::getInstance().success("Imagen cargada exitosamente en la ranura " +
                                std::to_string(slot) + " de la pantalla.");
}

std::future<void> AK820Pro::uploadImageToLCDAsync(const std::string &path,
                                                  int slot) const {
  return std::async(std::launch::async, [this, path, slot] {
    try {
      this->uploadImageToLCD(path, slot);
    } catch (const std::exception &e) {
      Logger::getInstance().error("Error en uploadImageToLCDAsync: " +
                                  std::string(e.what()));
    }
  });
}

void AK820Pro::uploadGIF(const std::string &path) const {
  this->uploadImageToLCD(path, 1);
}
