#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include "config_manager.h"

enum WiFiState {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_AP_MODE
};

typedef void (*WiFiStateCallback)(WiFiState state);

class WifiManager {
public:
    bool begin(ConfigManager& configMgr);  // Takes reference to config manager
    void update();                          // Call in loop()
    void resetWiFi();                       // Clear credentials, restart AP
    WiFiState getState() { return _state; }
    String getIP();
    String getSSID();
    int getRSSI();
    String getAPName();
    void setStateCallback(WiFiStateCallback cb);
    
private:
    ConfigManager* _configMgr;
    WiFiState _state = WIFI_STATE_DISCONNECTED;
    WiFiStateCallback _stateCallback = nullptr;
    DNSServer* _dnsServer = nullptr;
    AsyncWebServer* _apServer = nullptr;
    
    unsigned long _connectStartTime;
    unsigned long _lastReconnectAttempt = 0;
    String _apName;
    
    void startAP();
    void stopAP();
    bool tryConnect(uint32_t timeoutMs = 15000);
    void setupCaptivePortal();
    String getCaptivePortalHTML();
    void setState(WiFiState newState);
};
