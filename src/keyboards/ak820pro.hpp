//
// Created by taxmachine on 16/02/25.
// Modified by Vixcho on 13/10/26.
//

#ifndef AJAZZ_HPP
#define AJAZZ_HPP

#include <future>
#include <iomanip>
#include <string>
#include <vector>
#include <array>
#include <hidapi/hidapi.h>

#include "keyboarddefs.hpp"

class AK820Pro {
    struct ModePacket {
        uint8_t mode;
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint32_t padding;
        struct {
            bool rainbow;
            uint8_t brightness;
            uint8_t speed;
            uint8_t direction;
        } options;
        uint16_t padding1;
        uint16_t delimiter;
        uint8_t additional_data[48];
    };

public:
    struct KeyColor {
        uint8_t keyIndex;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    enum LightingMode {
        LED_OFF = 0,
        STATIC = 1,
        SINGLE_ON = 2,
        SINGLE_OFF = 3,
        GLITTERING = 4,
        FALLING = 5,
        COLOURFUL = 6,
        BREATH = 7,
        SPECTRUM = 8,
        OUTWARD = 9,
        SCROLLING = 10,
        ROLLING = 11,
        ROTATING = 12,
        EXPLODE = 13,
        LAUNCH = 14,
        RIPPLES = 15,
        FLOWING = 16,
        PULSATING = 17,
        TILT = 18,
        SHUTTLE = 19,
        CUSTOM = 0x80
    };

    enum Direction {
        LEFT = 0,
        UP = 1,
        DOWN = 2,
        RIGHT = 3
    };

    enum LightSleepTime {
        NONE = 0,
        ONE_MIN = 1,
        FIVE_MIN = 2,
        THIRTY_MIN = 3
    };

    explicit AK820Pro(uint16_t vid = AJAZZ_VID, uint16_t pid = 0);
    ~AK820Pro();

    bool openHandle();
    void closeHandle();

    bool isConnected() const;
    bool isWireless() const;
    std::string getConnectionStatus() const;
    std::string getLastError() const;

    void setColor(uint8_t r, uint8_t g, uint8_t b) const;
    void setMode(LightingMode mode, uint8_t r = 255, uint8_t g = 0 , uint8_t b = 0,
                bool rainbow = true, int brightness = MAX_BRIGHTNESS, int speed = MAX_SPEED, Direction direction = LEFT) const;
    std::future<void> setModeAsync(LightingMode mode, uint8_t r, uint8_t g, uint8_t b,
                                   bool rainbow, int brightness, int speed, Direction direction) const;

    void setPerKeyRGB(const std::vector<KeyColor>& keyColors) const;
    std::future<void> setPerKeyRGBAsync(const std::vector<KeyColor>& keyColors) const;

    void setSleepTime(LightSleepTime sleep_time) const;
    std::future<void> setSleepTimeAsync(LightSleepTime sleep_time) const;

    void syncClock(int slot = 1) const;
    std::future<void> syncClockAsync(int slot = 1) const;

    void uploadGIF(const std::string& path) const;
    void uploadImageToLCD(const std::string& path, int slot = 1) const;
    std::future<void> uploadImageToLCDAsync(const std::string& path, int slot = 1) const;

private:
    uint16_t vid;
    uint16_t pid;
    mutable hid_device* handle = nullptr;
    mutable std::string activePath;
    mutable bool wirelessMode = false;
    mutable std::string lastErrorMessage;

    void executeHIDCommand(const std::vector<uint8_t*>& commands) const;
    bool sendFeatureReport(const uint8_t* data, size_t len) const;
    bool getFeatureReport(uint8_t* data, size_t len) const;
};

#endif //AJAZZ_HPP