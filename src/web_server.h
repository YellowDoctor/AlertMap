#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config_manager.h"
#include "alert_service.h"
#include "led_controller.h"
#include "statistics.h"
#include "ota_manager.h"
#include "night_mode.h"
#include "wifi_manager.h"
#include "version.h"

class AppWebServer {
public:
    void begin(
        ConfigManager& configMgr,
        AlertService& alertService,
        LEDController& ledController,
        Statistics& stats,
        OTAManager& otaManager,
        NightMode& nightMode,
        WifiManager& wifiManager
    );
    
private:
    AsyncWebServer* _server = nullptr;
    ConfigManager* _configMgr = nullptr;
    AlertService* _alertService = nullptr;
    LEDController* _ledController = nullptr;
    Statistics* _stats = nullptr;
    OTAManager* _otaManager = nullptr;
    NightMode* _nightMode = nullptr;
    WifiManager* _wifiManager = nullptr;
    
    void setupRoutes();
    
    void handleGetStatus(AsyncWebServerRequest* request);
    void handleGetAlerts(AsyncWebServerRequest* request);
    void handleGetStats(AsyncWebServerRequest* request);
    void handleGetConfig(AsyncWebServerRequest* request);
    void handlePostConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
    void handlePostWifi(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
    void handleOTACheck(AsyncWebServerRequest* request);
    void handleOTAGithub(AsyncWebServerRequest* request);
    void handleOTAUpload(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final);
    void handleOTAProgress(AsyncWebServerRequest* request);
    void handleTestPattern(AsyncWebServerRequest* request);
    void handleRestart(AsyncWebServerRequest* request);
    
    void addCorsHeaders(AsyncWebServerResponse* response);
};