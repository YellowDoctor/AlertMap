#include "alert_service.h"
#include <WiFi.h>

void AlertService::begin(ConfigManager& configMgr) {
    _configMgr = &configMgr;
    _currentInterval = _configMgr->config.poll_interval;
    if (_currentInterval < 10000) _currentInterval = 10000;
    
    for (int i = 0; i < NUM_LEDS; i++) {
        _districtAlerts[i] = ALERT_OK;
        _prevDistrictAlerts[i] = ALERT_OK;
    }
    _eventCount = 0;
    Serial.println("[Alert] Сервіс моніторингу тривог запущено");
}

void AlertService::update() {
    if (millis() - _lastPollTime >= _currentInterval) {
        _lastPollTime = millis();
        fetchAlerts();
    }
}

bool AlertService::forceUpdate() {
    _lastPollTime = millis();
    return fetchAlerts();
}

AlertLevel AlertService::getDistrictAlert(uint8_t ledIndex) {
    if (ledIndex < NUM_LEDS) return _districtAlerts[ledIndex];
    return ALERT_OK;
}

uint8_t AlertService::getActiveAlertCount() {
    uint8_t count = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
        if (_districtAlerts[i] != ALERT_OK && _districtAlerts[i] != ALERT_OFFLINE) count++;
    }
    return count;
}

uint8_t AlertService::getActiveMissileCount() {
    uint8_t count = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
        if (_districtAlerts[i] == ALERT_MISSILES) count++;
    }
    return count;
}

uint8_t AlertService::getActiveDroneCount() {
    uint8_t count = 0;
    for (int i = 0; i < NUM_LEDS; i++) {
        if (_districtAlerts[i] == ALERT_DRONES) count++;
    }
    return count;
}

bool AlertService::fetchAlerts() {
    if (WiFi.status() != WL_CONNECTED) {
        _online = false;
        _lastError = "Немає підключення до WiFi";
        return false;
    }
    
    if (!_configMgr || strlen(_configMgr->config.api_token) == 0) {
        _online = false;
        _lastError = "API токен не налаштовано";
        return false;
    }
    
    _totalRequests++;
    WiFiClientSecure client;
    client.setInsecure();
    
    HTTPClient https;
    Serial.println("[Alert] Запит тривог до alerts.in.ua...");
    
    if (https.begin(client, "https://api.alerts.in.ua/v1/alerts/active.json")) {
        String auth = "Bearer ";
        auth += _configMgr->config.api_token;
        https.addHeader("Authorization", auth);
        
        if (_lastModified.length() > 0) {
            https.addHeader("If-Modified-Since", _lastModified);
        }
        
        const char* headerKeys[] = {"Last-Modified"};
        https.collectHeaders(headerKeys, 1);
        
        int httpCode = https.GET();
        
        if (httpCode > 0) {
            if (httpCode == HTTP_CODE_OK) {
                if (https.hasHeader("Last-Modified")) {
                    _lastModified = https.header("Last-Modified");
                }
                String payload = https.getString();
                if (parseAlerts(payload)) {
                    _online = true;
                    _lastSuccessfulUpdate = time(nullptr);
                    _currentInterval = _configMgr->config.poll_interval;
                    if (_currentInterval < 10000) _currentInterval = 10000;
                    detectChanges();
                }
            } else if (httpCode == 304) {
                _total304++;
                _online = true;
                _lastSuccessfulUpdate = time(nullptr);
                Serial.println("[Alert] 304: Дані не змінилися (кеш)");
            } else if (httpCode == 429) {
                _totalErrors++;
                _lastError = "Перевищено ліміт запитів (429)";
                _currentInterval = 20000;
                Serial.println("[Alert] " + _lastError);
            } else if (httpCode == 401) {
                _totalErrors++;
                _lastError = "Помилка токена API (401)";
                Serial.println("[Alert] " + _lastError);
            } else {
                _totalErrors++;
                _lastError = "HTTP помилка: " + String(httpCode);
                Serial.println("[Alert] " + _lastError);
            }
        } else {
            _totalErrors++;
            _lastError = "Помилка з'єднання: " + https.errorToString(httpCode);
            Serial.println("[Alert] " + _lastError);
        }
        https.end();
    } else {
        _totalErrors++;
        _lastError = "Не вдалося відкрити HTTPS з'єднання";
        Serial.println("[Alert] " + _lastError);
    }
    
    return _online;
}

bool AlertService::parseAlerts(const String& payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        _lastError = "Помилка JSON: " + String(error.c_str());
        Serial.println("[Alert] " + _lastError);
        return false;
    }
    
    for (int i = 0; i < NUM_LEDS; i++) {
        _prevDistrictAlerts[i] = _districtAlerts[i];
        _districtAlerts[i] = ALERT_OK; // За замовчуванням спокійно
    }
    
    JsonArray alerts = doc["alerts"];
    if (alerts.isNull()) return true;
    
    for (JsonObject alert : alerts) {
        const char* locType = alert["location_type"];
        const char* locTitle = alert["location_title"];
        const char* locRaion = alert["location_raion"];
        const char* locOblast = alert["location_oblast"];
        
        if (!locType) continue;
        String locTypeStr = String(locType);
        
        AlertLevel level = determineThreatLevel(alert);
        
        if (locTypeStr == "oblast") {
            const char* searchOblast = locOblast ? locOblast : locTitle;
            uint8_t leds[NUM_LEDS];
            uint8_t count = getLedsForOblast(searchOblast, leds, NUM_LEDS);
            for (uint8_t i = 0; i < count; i++) {
                uint8_t led = leds[i];
                if (led < NUM_LEDS && level > _districtAlerts[led]) {
                    _districtAlerts[led] = level;
                }
            }
        } else {
            int16_t led = findLedByLocationName(locTitle, locRaion, locOblast);
            if (led >= 0 && led < NUM_LEDS) {
                if (level > _districtAlerts[led]) {
                    _districtAlerts[led] = level;
                }
            }
        }
    }
    
    return true;
}

AlertLevel AlertService::determineThreatLevel(JsonObject& alert) {
    AlertLevel finalLevel = ALERT_PARTIAL;
    JsonArray threats = alert["threats"];
    
    if (!threats.isNull() && threats.size() > 0) {
        for (JsonObject threat : threats) {
            const char* type = threat["threat_type"];
            const char* levelStr = threat["level"];
            if (!type) continue;
            
            String tType = String(type);
            String lStr = levelStr ? String(levelStr) : "";
            
            if (tType == "missiles" || tType == "ballistic" || tType == "artillery") {
                return ALERT_MISSILES;
            } else if (tType == "drones") {
                if (finalLevel < ALERT_DRONES) finalLevel = ALERT_DRONES;
            } else if (tType == "air_raid") {
                if (lStr == "red") {
                    return ALERT_MISSILES;
                } else if (lStr == "yellow") {
                    if (finalLevel < ALERT_DRONES) finalLevel = ALERT_DRONES;
                }
            }
        }
    } else {
        const char* aLevel = alert["alert_level"];
        if (aLevel) {
            String lvl = String(aLevel);
            if (lvl == "red") return ALERT_MISSILES;
            if (lvl == "yellow") return ALERT_DRONES;
        }
    }
    
    return finalLevel;
}

void AlertService::detectChanges() {
    _hasNewAlerts = false;
    for (int i = 0; i < NUM_LEDS; i++) {
        if (_districtAlerts[i] != _prevDistrictAlerts[i]) {
            if (_districtAlerts[i] > _prevDistrictAlerts[i] && _districtAlerts[i] != ALERT_OK) {
                _hasNewAlerts = true;
            }
            addEvent(LED_DISTRICT_MAP[i].name, _districtAlerts[i], _prevDistrictAlerts[i]);
        }
    }
}

void AlertService::addEvent(const char* name, AlertLevel newLevel, AlertLevel oldLevel) {
    if (_eventCount < MAX_EVENTS) {
        _eventCount++;
    }
    
    for (int i = MAX_EVENTS - 1; i > 0; i--) {
        _recentEvents[i] = _recentEvents[i - 1];
    }
    
    strlcpy(_recentEvents[0].district_name, name, sizeof(_recentEvents[0].district_name));
    _recentEvents[0].level = newLevel;
    _recentEvents[0].prev_level = oldLevel;
    _recentEvents[0].timestamp = time(nullptr);
}