#include "web_server.h"

void AppWebServer::begin(
    ConfigManager& configMgr,
    AlertService& alertService,
    LEDController& ledController,
    Statistics& stats,
    OTAManager& otaManager,
    NightMode& nightMode,
    WifiManager& wifiManager
) {
    _configMgr = &configMgr;
    _alertService = &alertService;
    _ledController = &ledController;
    _stats = &stats;
    _otaManager = &otaManager;
    _nightMode = &nightMode;
    _wifiManager = &wifiManager;
    
    _server = new AsyncWebServer(80);
    setupRoutes();
    _server->begin();
    Serial.println("[WebServer] Веб-сервер запущено на порту 80");
}

void AppWebServer::addCorsHeaders(AsyncWebServerResponse* response) {
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
}

void AppWebServer::setupRoutes() {
    _server->onNotFound([this](AsyncWebServerRequest *request) {
        if (request->method() == HTTP_OPTIONS) {
            AsyncWebServerResponse *response = request->beginResponse(204);
            addCorsHeaders(response);
            request->send(response);
        } else {
            request->send(404, "text/plain", "Not found");
        }
    });

    _server->serveStatic("/", LittleFS, "/www/").setDefaultFile("index.html");
    
    _server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetStatus(request);
    });
    
    _server->on("/api/alerts", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetAlerts(request);
    });
    
    _server->on("/api/stats", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetStats(request);
    });
    
    _server->on("/api/config", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleGetConfig(request);
    });
    
    _server->on("/api/config", HTTP_POST, 
        [](AsyncWebServerRequest *request) { request->send(200); },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handlePostConfig(request, data, len, index, total);
        }
    );
    
    _server->on("/api/wifi", HTTP_POST,
        [](AsyncWebServerRequest *request) { request->send(200); },
        NULL,
        [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            handlePostWifi(request, data, len, index, total);
        }
    );
    
    _server->on("/api/ota/check", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleOTACheck(request);
    });
    
    _server->on("/api/ota/github", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleOTAGithub(request);
    });
    
    _server->on("/api/ota/upload", HTTP_POST, 
        [](AsyncWebServerRequest *request) { 
            AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "{\"status\":\"ok\"}");
            response->addHeader("Access-Control-Allow-Origin", "*");
            request->send(response);
        },
        [this](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
            handleOTAUpload(request, filename, index, data, len, final);
        }
    );
    
    _server->on("/api/ota/progress", HTTP_GET, [this](AsyncWebServerRequest* request) {
        handleOTAProgress(request);
    });
    
    _server->on("/api/test/pattern", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleTestPattern(request);
    });
    
    _server->on("/api/restart", HTTP_POST, [this](AsyncWebServerRequest* request) {
        handleRestart(request);
    });
}

void AppWebServer::handleGetStatus(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["version"] = FIRMWARE_VERSION;
    doc["uptime"] = millis() / 1000;
    doc["wifi_ssid"] = _wifiManager->getSSID();
    doc["wifi_rssi"] = _wifiManager->getRSSI();
    doc["wifi_ip"] = _wifiManager->getIP();
    doc["wifi_state"] = (int)_wifiManager->getState();
    doc["is_night"] = _nightMode->isNight();
    doc["current_time"] = _nightMode->getCurrentTime();
    
    doc["active_alerts"] = _alertService->getActiveAlertCount();
    doc["active_missiles"] = _alertService->getActiveMissileCount();
    doc["active_drones"] = _alertService->getActiveDroneCount();
    
    doc["online"] = _alertService->isOnline();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["last_update"] = (uint32_t)_alertService->getLastUpdateTime();
    
    String responseStr;
    serializeJson(doc, responseStr);
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", responseStr);
    addCorsHeaders(response);
    request->send(response);
}

void AppWebServer::handleGetAlerts(AsyncWebServerRequest* request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    addCorsHeaders(response);
    
    response->print("{\"alerts\":[");
    for (int i = 0; i < NUM_LEDS; i++) {
        if (i > 0) response->print(",");
        response->print("{\"led\":");
        response->print(i);
        response->print(",\"name\":\"");
        response->print(LED_DISTRICT_MAP[i].name);
        response->print("\",\"district\":\"");
        response->print(LED_DISTRICT_MAP[i].district);
        response->print("\",\"oblast\":\"");
        response->print(LED_DISTRICT_MAP[i].oblast);
        response->print("\",\"level\":");
        uint8_t lvl = _alertService->getDistrictAlert(i);
        response->print(lvl);
        response->print(",\"level_name\":\"");
        switch(lvl) {
            case ALERT_OK: response->print("OK"); break;
            case ALERT_DRONES: response->print("DRONES"); break;
            case ALERT_PARTIAL: response->print("PARTIAL"); break;
            case ALERT_MISSILES: response->print("MISSILES"); break;
            default: response->print("OFFLINE"); break;
        }
        response->print("\"}");
    }
    response->print("]}");
    request->send(response);
}

void AppWebServer::handleGetStats(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", _stats->toJson());
    addCorsHeaders(response);
    request->send(response);
}

void AppWebServer::handleGetConfig(AsyncWebServerRequest* request) {
    JsonDocument doc;
    deserializeJson(doc, _configMgr->toJson());
    
    if (doc.containsKey("api_token")) {
        String token = doc["api_token"].as<String>();
        if (token.length() > 4) {
            doc["api_token"] = String("...") + token.substring(token.length() - 4);
        }
    }
    
    String responseStr;
    serializeJson(doc, responseStr);
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", responseStr);
    addCorsHeaders(response);
    request->send(response);
}

void AppWebServer::handlePostConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        request->_tempObject = new String();
    }
    String* str = (String*)request->_tempObject;
    str->concat((char*)data, len);
    
    if (index + len == total) {
        _configMgr->updateFromJson(*str);
        _configMgr->save();
        _ledController->setAnimationMode(_configMgr->config.alert_animation_mode);
        _ledController->setPulseEnabled(_configMgr->config.pulse_on_alert);
        _ledController->setSmoothEnabled(_configMgr->config.smooth_transitions);
        _ledController->setWaveEnabled(_configMgr->config.wave_on_new_alert);
        _ledController->setBrightness(_nightMode->getCurrentBrightness());
        
        delete str;
        request->_tempObject = NULL;
        
        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "{\"status\":\"ok\"}");
        addCorsHeaders(response);
        request->send(response);
    }
}

void AppWebServer::handlePostWifi(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    if (index == 0) {
        request->_tempObject = new String();
    }
    String* str = (String*)request->_tempObject;
    str->concat((char*)data, len);
    
    if (index + len == total) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, *str);
        if (!err) {
            const char* ssid = doc["wifi_ssid"] | doc["ssid"];
            const char* pass = doc["wifi_password"] | doc["password"];
            if (ssid) {
                strlcpy(_configMgr->config.wifi_ssid, ssid, sizeof(_configMgr->config.wifi_ssid));
                if (pass) strlcpy(_configMgr->config.wifi_password, pass, sizeof(_configMgr->config.wifi_password));
                _configMgr->save();
                _wifiManager->resetWiFi();
            }
        }
        
        delete str;
        request->_tempObject = NULL;
        
        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "{\"status\":\"ok\"}");
        addCorsHeaders(response);
        request->send(response);
    }
}

void AppWebServer::handleOTACheck(AsyncWebServerRequest* request) {
    bool hasUpdate = _otaManager->checkForUpdate();
    JsonDocument doc;
    doc["available"] = hasUpdate;
    doc["current_version"] = _otaManager->getCurrentVersion();
    doc["latest_version"] = _otaManager->getLatestVersion();
    doc["release_notes"] = _otaManager->getReleaseNotes();
    doc["status"] = _otaManager->getStatusMessage();
    
    String res;
    serializeJson(doc, res);
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", res);
    addCorsHeaders(response);
    request->send(response);
}

void AppWebServer::handleOTAGithub(AsyncWebServerRequest* request) {
    _otaManager->startGitHubUpdate();
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "{\"status\":\"started\"}");
    addCorsHeaders(response);
    request->send(response);
}

void AppWebServer::handleOTAUpload(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
    size_t total = request->contentLength();
    _otaManager->handleUploadChunk(data, len, index, total, final);
}

void AppWebServer::handleOTAProgress(AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["status"] = (int)_otaManager->getStatus();
    doc["progress"] = _otaManager->getProgress();
    doc["message"] = _otaManager->getStatusMessage();
    doc["available"] = _otaManager->isUpdateAvailable();
    doc["latest_version"] = _otaManager->getLatestVersion();
    
    String res;
    serializeJson(doc, res);
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", res);
    addCorsHeaders(response);
    request->send(response);
}

void AppWebServer::handleTestPattern(AsyncWebServerRequest* request) {
    _ledController->showTestPattern();
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "{\"ok\":true}");
    addCorsHeaders(response);
    request->send(response);
}

void AppWebServer::handleRestart(AsyncWebServerRequest* request) {
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "{\"ok\":true}");
    addCorsHeaders(response);
    request->send(response);
    
    delay(1000); 
    ESP.restart();
}