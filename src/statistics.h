#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "led_controller.h"

class Statistics {
public:
    void begin();
    void update();                              // Call in loop()
    void onAlertChange(uint8_t ledIndex, AlertLevel oldLevel, AlertLevel newLevel);
    void onApiRequest(bool success);
    void onWifiReconnect();
    
    // Getters
    uint32_t getTotalAlertsToday() { return _totalAlertsToday; }
    uint32_t getTotalAllClearToday() { return _totalAllClearToday; }
    time_t getLastAlertTime() { return _lastAlertTime; }
    uint32_t getLongestAlertDuration() { return _longestAlertDuration; }
    const char* getLongestAlertDistrict() { return _longestAlertDistrict; }
    uint32_t getUptimeSeconds();
    uint32_t getWifiReconnects() { return _wifiReconnects; }
    uint32_t getApiRequests() { return _apiRequests; }
    uint32_t getApiErrors() { return _apiErrors; }
    uint32_t getFreeHeap() { return ESP.getFreeHeap(); }
    
    String toJson();  // Serialize all stats to JSON
    void resetDaily(); // Reset daily counters
    
private:
    uint32_t _totalAlertsToday = 0;
    uint32_t _totalAllClearToday = 0;
    time_t _lastAlertTime = 0;
    uint32_t _longestAlertDuration = 0;
    char _longestAlertDistrict[48] = "";
    uint32_t _wifiReconnects = 0;
    uint32_t _apiRequests = 0;
    uint32_t _apiErrors = 0;
    unsigned long _startTime;
    
    // Tracking active alerts for duration calculation
    struct ActiveAlert {
        bool active;
        time_t startedAt;
        char districtName[48];
    };
    ActiveAlert _activeAlerts[NUM_LEDS];
    
    uint8_t _lastDay = 0;  // For daily reset
    void checkDayChange();
};
