#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "config.h"
#include "config_manager.h"
#include "version.h"

enum OTAStatus {
    OTA_IDLE,
    OTA_CHECKING,
    OTA_AVAILABLE,
    OTA_DOWNLOADING,
    OTA_FLASHING,
    OTA_SUCCESS,
    OTA_ERROR
};

class OTAManager {
public:
    void begin(ConfigManager& configMgr);
    
    // GitHub OTA
    bool checkForUpdate();
    bool startGitHubUpdate();
    
    // File upload OTA  
    bool handleUploadChunk(uint8_t* data, size_t len, size_t index, size_t total, bool final);
    
    // State
    OTAStatus getStatus() const { return _status; }
    uint8_t getProgress() const { return _progress; }
    String getStatusMessage() const { return _statusMessage; }
    String getLatestVersion() const { return _latestVersion; }
    String getReleaseNotes() const { return _releaseNotes; }
    bool isUpdateAvailable() const { return _updateAvailable; }
    String getCurrentVersion() const { return FIRMWARE_VERSION; }
    bool hasFsUpdate() const { return _fsDownloadUrl.length() > 0; }
    
    String toJson();
    
private:
    ConfigManager* _configMgr = nullptr;
    OTAStatus _status = OTA_IDLE;
    uint8_t _progress = 0;
    String _statusMessage;
    String _latestVersion;
    String _releaseNotes;
    String _downloadUrl;
    String _fsDownloadUrl;
    bool _updateAvailable = false;
    
    bool compareVersions(const String& current, const String& latest);
    bool downloadAndFlash(const String& url, int command = U_FLASH, bool reboot = true);
    void setStatus(OTAStatus status, const String& message);
};