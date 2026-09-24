#pragma once
#include <Arduino.h>
#include <time.h>
#include "config_manager.h"

class NightMode {
public:
    void begin(ConfigManager& configMgr);
    void update();
    uint8_t getCurrentBrightness();
    bool isNight();
    bool isNTPSynced() const { return _ntpSynced; }
    String getCurrentTime();
    int getCurrentHour();
    
private:
    ConfigManager* _configMgr = nullptr;
    bool _ntpSynced = false;
    unsigned long _lastNtpSync = 0;
    unsigned long _ntpSyncInterval = 3600000;
    uint8_t _currentBrightness = 128;
    uint8_t _targetBrightness = 128;
    unsigned long _lastBrightnessUpdate = 0;
    
    void syncNTP();
    void updateBrightness();
};