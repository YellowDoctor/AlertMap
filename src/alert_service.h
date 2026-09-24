#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "config.h"
#include "config_manager.h"
#include "led_controller.h"
#include "led_mapping.h"

struct AlertEvent {
    char district_name[48];
    AlertLevel level;
    AlertLevel prev_level;
    time_t timestamp;
};

class AlertService {
public:
    void begin(ConfigManager& configMgr);
    void update();
    bool forceUpdate();
    
    AlertLevel getDistrictAlert(uint8_t ledIndex);
    const AlertLevel* getAllAlerts() const { return _districtAlerts; }
    uint8_t getActiveAlertCount();
    uint8_t getActiveMissileCount();
    uint8_t getActiveDroneCount();
    bool isOnline() const { return _online; }
    time_t getLastUpdateTime() const { return _lastSuccessfulUpdate; }
    String getLastError() const { return _lastError; }
    
    const AlertEvent* getRecentEvents() const { return _recentEvents; }
    uint8_t getRecentEventCount() const { return _eventCount; }
    
    uint32_t getTotalRequests() const { return _totalRequests; }
    uint32_t getTotalErrors() const { return _totalErrors; }
    uint32_t getTotal304() const { return _total304; }
    bool hasNewAlerts() const { return _hasNewAlerts; }
    void clearNewAlertsFlag() { _hasNewAlerts = false; }
    
private:
    ConfigManager* _configMgr = nullptr;
    AlertLevel _districtAlerts[NUM_LEDS];
    AlertLevel _prevDistrictAlerts[NUM_LEDS];
    
    unsigned long _lastPollTime = 0;
    unsigned long _currentInterval = 10000;
    
    bool _online = false;
    time_t _lastSuccessfulUpdate = 0;
    String _lastModified;
    String _lastError;
    bool _hasNewAlerts = false;
    
    uint32_t _totalRequests = 0;
    uint32_t _totalErrors = 0;
    uint32_t _total304 = 0;
    
    static const uint8_t MAX_EVENTS = 20;
    AlertEvent _recentEvents[MAX_EVENTS];
    uint8_t _eventCount = 0;
    
    bool fetchAlerts();
    bool parseAlerts(const String& payload);
    AlertLevel determineThreatLevel(JsonObject& alert);
    void addEvent(const char* name, AlertLevel newLevel, AlertLevel oldLevel);
    void detectChanges();
};