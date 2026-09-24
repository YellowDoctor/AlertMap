#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "config.h"
#include "animations.h"

enum AlertLevel : uint8_t {
    ALERT_OK = 0,        // Зелений — все спокійно
    ALERT_DRONES = 1,    // Жовтий — загроза дронів
    ALERT_PARTIAL = 2,   // Помаранчевий — часткова тривога
    ALERT_MISSILES = 3,  // Червоний — ракетна/балістична
    ALERT_OFFLINE = 4    // Синій — немає зв'язку
};

class ConfigManager;

class LEDController {
public:
    void begin(ConfigManager& configMgr);
    void update();
    void setDistrictAlert(uint8_t ledIndex, AlertLevel level);
    void setAllAlerts(const AlertLevel* levels);
    void setBrightness(uint8_t brightness);
    uint8_t getBrightness() const { return _currentBrightness; }
    void showTestPattern();
    void showStartupAnimation();
    void showOffline();
    void showConnecting();
    AlertLevel getDistrictAlert(uint8_t ledIndex) const;
    CRGB alertLevelToColor(AlertLevel level) const;
    
    // Анімації
    void triggerWave() { _animEngine.triggerWave(); }
    void triggerDistrictAlert(uint8_t ledIndex) { _animEngine.triggerDistrictAlert(ledIndex); }
    void setAnimationMode(uint8_t mode) { _animEngine.setMode(mode); }
    void setPulseEnabled(bool enabled) { _animEngine.setPulseEnabled(enabled); }
    void setSmoothEnabled(bool enabled) { _animEngine.setSmoothEnabled(enabled); }
    void setWaveEnabled(bool enabled) { _animEngine.setWaveEnabled(enabled); }
    
private:
    CRGB _leds[NUM_LEDS];
    CRGB _targetColors[NUM_LEDS];
    AlertLevel _alertLevels[NUM_LEDS];
    ConfigManager* _configMgr = nullptr;
    AnimationEngine _animEngine;
    uint8_t _currentBrightness = 128;
    bool _initialized = false;
};