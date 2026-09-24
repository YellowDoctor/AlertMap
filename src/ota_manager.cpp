#include "ota_manager.h"

void OTAManager::begin(ConfigManager& configMgr) {
    _configMgr = &configMgr;
    setStatus(OTA_IDLE, "Готовий");
}

void OTAManager::setStatus(OTAStatus status, const String& message) {
    _status = status;
    _statusMessage = message;
    Serial.printf("[OTA] Статус: %s\n", message.c_str());
}

bool OTAManager::checkForUpdate() {
    setStatus(OTA_CHECKING, "Перевірка оновлень на GitHub...");
    
    if (!_configMgr || strlen(_configMgr->config.github_owner) == 0 || strlen(_configMgr->config.github_repo) == 0) {
        setStatus(OTA_ERROR, "GitHub репозиторій не налаштовано");
        return false;
    }
    
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    
    String url = "https://api.github.com/repos/";
    url += _configMgr->config.github_owner;
    url += "/";
    url += _configMgr->config.github_repo;
    url += "/releases/latest";
    
    http.begin(client, url);
    http.addHeader("User-Agent", "ESP32-AlertMap-OTA");
    
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        setStatus(OTA_ERROR, "Помилка GitHub API: HTTP " + String(httpCode));
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        setStatus(OTA_ERROR, "Помилка парсингу відповіді GitHub");
        return false;
    }
    
    String latestVer = doc["tag_name"] | "";
    if (latestVer.startsWith("v") || latestVer.startsWith("V")) {
        latestVer = latestVer.substring(1);
    }
    
    _latestVersion = latestVer;
    _releaseNotes = doc["body"] | "Немає опису";
    
    String currentVer = FIRMWARE_VERSION;
    if (currentVer.startsWith("v") || currentVer.startsWith("V")) {
        currentVer = currentVer.substring(1);
    }
    
    _downloadUrl = "";
    _fsDownloadUrl = "";
    
    JsonArray assets = doc["assets"];
    for (JsonObject asset : assets) {
        const char* name = asset["name"];
        const char* dlUrl = asset["browser_download_url"];
        if (!name || !dlUrl) continue;
        String nameStr = String(name);
        
        if (nameStr.indexOf("firmware") >= 0 || (nameStr.endsWith(".bin") && nameStr.indexOf("littlefs") < 0 && nameStr.indexOf("bootloader") < 0 && nameStr.indexOf("partition") < 0)) {
            _downloadUrl = String(dlUrl);
        } else if (nameStr.indexOf("littlefs") >= 0 || nameStr.indexOf("spiffs") >= 0) {
            _fsDownloadUrl = String(dlUrl);
        }
    }
    
    if (_downloadUrl.isEmpty()) {
        setStatus(OTA_ERROR, "У релізі не знайдено firmware.bin");
        return false;
    }
    
    if (compareVersions(currentVer, latestVer)) {
        _updateAvailable = true;
        String msg = "Доступне оновлення: v" + latestVer;
        if (_fsDownloadUrl.length() > 0) {
            msg += " (+ Web UI)";
        }
        setStatus(OTA_AVAILABLE, msg);
        return true;
    } else {
        _updateAvailable = false;
        setStatus(OTA_IDLE, "Встановлено актуальну версію (v" + currentVer + ")");
        return false;
    }
}

bool OTAManager::startGitHubUpdate() {
    if (!_updateAvailable || _downloadUrl.isEmpty()) {
        setStatus(OTA_ERROR, "Оновлення недоступне");
        return false;
    }
    
    // Якщо в релізі є і LittleFS, і Firmware:
    if (_fsDownloadUrl.length() > 0) {
        setStatus(OTA_DOWNLOADING, "Оновлення Web UI (LittleFS)...");
        if (!downloadAndFlash(_fsDownloadUrl, U_SPIFFS, false)) {
            setStatus(OTA_ERROR, "Помилка оновлення LittleFS");
            return false;
        }
    }
    
    // Оновлення прошивки
    setStatus(OTA_DOWNLOADING, "Оновлення прошивки...");
    return downloadAndFlash(_downloadUrl, U_FLASH, true);
}

bool OTAManager::downloadAndFlash(const String& url, int command, bool reboot) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    
    http.begin(client, url);
    http.addHeader("User-Agent", "ESP32-AlertMap-OTA");
    
    const char* headerKeys[] = {"Location"};
    http.collectHeaders(headerKeys, 1);
    
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_FOUND || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String newUrl = http.header("Location");
        http.end();
        http.begin(client, newUrl);
        httpCode = http.GET();
    }
    
    if (httpCode != HTTP_CODE_OK) {
        setStatus(OTA_ERROR, "Помилка завантаження: HTTP " + String(httpCode));
        http.end();
        return false;
    }
    
    int contentLength = http.getSize();
    if (contentLength <= 0) {
        setStatus(OTA_ERROR, "Невірний розмір файлу оновлення");
        http.end();
        return false;
    }
    
    if (!Update.begin(contentLength, command)) {
        setStatus(OTA_ERROR, "Недостатньо пам'яті для запису розділу");
        http.end();
        return false;
    }
    
    setStatus(OTA_FLASHING, "Запис у пам'ять Flash...");
    
    WiFiClient* tcp = http.getStreamPtr();
    size_t written = 0;
    uint8_t buff[1024];
    
    while (http.connected() && (written < contentLength)) {
        size_t available = tcp->available();
        if (available) {
            int c = tcp->readBytes(buff, ((available > sizeof(buff)) ? sizeof(buff) : available));
            Update.write(buff, c);
            written += c;
            _progress = (written * 100) / contentLength;
        }
        delay(1);
    }
    
    if (written == contentLength) {
        if (Update.end(true)) {
            http.end();
            if (reboot) {
                setStatus(OTA_SUCCESS, "Оновлення завершено! Перезавантаження...");
                delay(1500);
                ESP.restart();
            }
            return true;
        }
    }
    
    setStatus(OTA_ERROR, "Помилка верифікації прошивки");
    http.end();
    return false;
}

bool OTAManager::handleUploadChunk(uint8_t* data, size_t len, size_t index, size_t total, bool final) {
    if (index == 0) {
        setStatus(OTA_FLASHING, "Завантаження прошивки через браузер...");
        _progress = 0;
        if (!Update.begin(total, U_FLASH)) {
            setStatus(OTA_ERROR, "Недостатньо пам'яті для файлу");
            return false;
        }
    }
    
    if (Update.write(data, len) != len) {
        setStatus(OTA_ERROR, "Помилка запису блоку даних");
        return false;
    }
    
    if (total > 0) {
        _progress = ((index + len) * 100) / total;
    }
    
    if (final) {
        if (Update.end(true)) {
            setStatus(OTA_SUCCESS, "Прошивку успішно оновлено! Перезавантаження...");
            delay(1500);
            ESP.restart();
            return true;
        } else {
            setStatus(OTA_ERROR, "Помилка завершення прошивки");
            return false;
        }
    }
    return true;
}

bool OTAManager::compareVersions(const String& current, const String& latest) {
    if (current == latest) return false;
    
    int c1 = 0, c2 = 0, c3 = 0;
    int l1 = 0, l2 = 0, l3 = 0;
    
    sscanf(current.c_str(), "%d.%d.%d", &c1, &c2, &c3);
    sscanf(latest.c_str(), "%d.%d.%d", &l1, &l2, &l3);
    
    if (l1 > c1) return true;
    if (l1 < c1) return false;
    if (l2 > c2) return true;
    if (l2 < c2) return false;
    return (l3 > c3);
}

String OTAManager::toJson() {
    JsonDocument doc;
    doc["status"] = (int)_status;
    doc["progress"] = _progress;
    doc["message"] = _statusMessage;
    doc["available"] = _updateAvailable;
    doc["latest_version"] = _latestVersion;
    doc["current_version"] = FIRMWARE_VERSION;
    doc["has_fs"] = (_fsDownloadUrl.length() > 0);
    String res;
    serializeJson(doc, res);
    return res;
}