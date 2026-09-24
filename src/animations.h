#pragma once
#include <Arduino.h>
#include <FastLED.h>
#include "config.h"

enum AlertLevel : uint8_t;

class AnimationEngine {
public:
    void begin();
    void update(CRGB* leds, CRGB* targetColors, AlertLevel* alertLevels, uint8_t numLeds);
    void triggerWave();
    void triggerDistrictAlert(uint8_t ledIndex);
    void setMode(uint8_t mode) { _mode = mode; }
    uint8_t getMode() const { return _mode; }
    void setPulseEnabled(bool enabled) { _pulseEnabled = enabled; }
    void setSmoothEnabled(bool enabled) { _smoothEnabled = enabled; }
    void setWaveEnabled(bool enabled) { _waveEnabled = enabled; }
    bool isWaveActive() const { return _waveActive; }
    
private:
    uint8_t _mode = DEFAULT_ALERT_ANIM_MODE;
    bool _pulseEnabled = true;
    bool _smoothEnabled = true;
    bool _waveEnabled = true;
    bool _waveActive = false;
    uint32_t _waveStartTime = 0;
    uint32_t _lastUpdateTime = 0;
    
    uint32_t _districtTriggerTime[NUM_LEDS];
    bool _districtAnimating[NUM_LEDS];
    
    static const uint16_t WAVE_DURATION = 1500;
    static const uint8_t WAVE_WIDTH = 12;
    static const uint16_t PULSE_PERIOD = 1800;
    static const uint8_t BLEND_SPEED = 24;
    
    void applySmoothTransition(CRGB* leds, CRGB* targets, uint8_t numLeds);
    void applyPulse(CRGB* leds, AlertLevel* alertLevels, uint8_t numLeds);
    void applyWave(CRGB* leds, uint8_t numLeds);
    void applyDistrictAlertIntro(CRGB* leds, CRGB* targets, AlertLevel* alertLevels, uint8_t numLeds, uint32_t now);
};