#include "night_mode.h"
#include <WiFi.h>

void NightMode::begin(ConfigManager& configMgr) {
    _configMgr = &configMgr;
    _currentBrightness = _configMgr ? _configMgr->config.brightness_day : 128;
    _targetBrightness = _currentBrightness;
    
    // Київський час: UTC+2 з переходом на літній час (Daylight Saving Time 3600s)
    configTime(7200, 3600, "pool.ntp.org", "time.google.com");
    Serial.println("[Night] Ініціалізація нічного режиму та NTP синхронізації");
    syncNTP();
}

void NightMode::update() {
    unsigned long now = millis();
    
    if (now - _lastNtpSync >= _ntpSyncInterval || (!_ntpSynced && now > 10000 && _lastNtpSync == 0)) {
        syncNTP();
        _lastNtpSync = now;
    }
    
    if (_configMgr) {
        if (_configMgr->config.night_mode_enabled && isNight()) {
            _targetBrightness = _configMgr->config.brightness_night;
        } else {
            _targetBrightness = _configMgr->config.brightness_day;
        }
    }
    
    updateBrightness();
}

void NightMode::syncNTP() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 1500)) {
        _ntpSynced = true;
        Serial.printf("[Night] NTP синхронізовано: %s\n", getCurrentTime().c_str());
    }
}

void NightMode::updateBrightness() {
    unsigned long now = millis();
    // Плавний перехід яскравості
    if (now - _lastBrightnessUpdate > 50) {
        if (_currentBrightness < _targetBrightness) {
            _currentBrightness++;
        } else if (_currentBrightness > _targetBrightness) {
            _currentBrightness--;
        }
        _lastBrightnessUpdate = now;
    }
}

uint8_t NightMode::getCurrentBrightness() {
    return _currentBrightness;
}

bool NightMode::isNight() {
    if (!_ntpSynced || !_configMgr) return false;
    int hour = getCurrentHour();
    uint8_t startH = _configMgr->config.night_start_hour;
    uint8_t endH = _configMgr->config.night_end_hour;
    
    if (startH > endH) {
        // Перехід через північ (наприклад 23:00 - 07:00)
        return (hour >= startH || hour < endH);
    } else {
        return (hour >= startH && hour < endH);
    }
}

int NightMode::getCurrentHour() {
    if (!_ntpSynced) return 12;
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        return timeinfo.tm_hour;
    }
    return 12;
}

String NightMode::getCurrentTime() {
    if (!_ntpSynced) return "00:00:00";
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char buff[16];
        strftime(buff, sizeof(buff), "%H:%M:%S", &timeinfo);
        return String(buff);
    }
    return "00:00:00";
}