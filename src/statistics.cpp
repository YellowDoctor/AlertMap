#include "statistics.h"
#include <time.h>

void Statistics::begin() {
    _startTime = millis();
    for (int i = 0; i < NUM_LEDS; i++) {
        _activeAlerts[i].active = false;
        _activeAlerts[i].startedAt = 0;
        _activeAlerts[i].districtName[0] = '\0';
    }
    // Get initial day
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    if (timeinfo) {
        _lastDay = timeinfo->tm_mday;
    }
}

void Statistics::update() {
    checkDayChange();
}

void Statistics::onAlertChange(uint8_t ledIndex, AlertLevel oldLevel, AlertLevel newLevel) {
    if (ledIndex >= NUM_LEDS) return;
    
    time_t now;
    time(&now);
    
    // Alert started (transition from No Alert to Alert)
    if (oldLevel == ALERT_OK && newLevel != ALERT_OK) {
        _totalAlertsToday++;
        _lastAlertTime = now;
        _activeAlerts[ledIndex].active = true;
        _activeAlerts[ledIndex].startedAt = now;
        // In a full implementation, we could look up the district name here
        // snprintf(_activeAlerts[ledIndex].districtName, sizeof(_activeAlerts[ledIndex].districtName), "РћР±Р»Р°СЃС‚СЊ %d", ledIndex);
    } 
    // Alert ended (transition from Alert to No Alert)
    else if (oldLevel != ALERT_OK && newLevel == ALERT_OK) {
        _totalAllClearToday++;
        
        if (_activeAlerts[ledIndex].active) {
            _activeAlerts[ledIndex].active = false;
            uint32_t duration = now - _activeAlerts[ledIndex].startedAt;
            if (duration > _longestAlertDuration) {
                _longestAlertDuration = duration;
                if (strlen(_activeAlerts[ledIndex].districtName) > 0) {
                    strncpy(_longestAlertDistrict, _activeAlerts[ledIndex].districtName, sizeof(_longestAlertDistrict));
                }
            }
        }
    }
}

void Statistics::onApiRequest(bool success) {
    _apiRequests++;
    if (!success) {
        _apiErrors++;
    }
}

void Statistics::onWifiReconnect() {
    _wifiReconnects++;
}

uint32_t Statistics::getUptimeSeconds() {
    return (millis() - _startTime) / 1000;
}

String Statistics::toJson() {
    JsonDocument doc;
    doc["totalAlertsToday"] = _totalAlertsToday;
    doc["totalAllClearToday"] = _totalAllClearToday;
    doc["lastAlertTime"] = _lastAlertTime;
    doc["longestAlertDuration"] = _longestAlertDuration;
    doc["longestAlertDistrict"] = _longestAlertDistrict;
    doc["uptimeSeconds"] = getUptimeSeconds();
    doc["wifiReconnects"] = _wifiReconnects;
    doc["apiRequests"] = _apiRequests;
    doc["apiErrors"] = _apiErrors;
    doc["freeHeap"] = getFreeHeap();
    
    String output;
    serializeJson(doc, output);
    return output;
}

void Statistics::resetDaily() {
    _totalAlertsToday = 0;
    _totalAllClearToday = 0;
}

void Statistics::checkDayChange() {
    time_t now;
    time(&now);
    struct tm* timeinfo = localtime(&now);
    // Check if time is valid (year > 2000) and day has changed
    if (timeinfo && timeinfo->tm_year > 100) { 
        if (timeinfo->tm_mday != _lastDay) {
            _lastDay = timeinfo->tm_mday;
            resetDaily();
        }
    }
}
