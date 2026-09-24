#include "wifi_manager.h"

const byte DNS_PORT = 53;

bool WifiManager::begin(ConfigManager& configMgr) {
    _configMgr = &configMgr;
    
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    _apName = "AlertMap-" + mac.substring(mac.length() - 4);
    
    Serial.println("[WiFi] Початок роботи. MAC: " + mac);
    
    if (strlen(_configMgr->config.wifi_ssid) > 0) {
        if (tryConnect(15000)) {
            return true;
        }
    }
    
    startAP();
    return false;
}

void WifiManager::update() {
    if (_state == WIFI_STATE_AP_MODE && _dnsServer) {
        _dnsServer->processNextRequest();
    }
    
    if (_state == WIFI_STATE_CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] З'єднання втрачено!");
            setState(WIFI_STATE_DISCONNECTED);
        }
    } else if (_state == WIFI_STATE_DISCONNECTED) {
        if (millis() - _lastReconnectAttempt > 10000 && strlen(_configMgr->config.wifi_ssid) > 0) {
            _lastReconnectAttempt = millis();
            tryConnect(15000);
        }
    }
}

void WifiManager::resetWiFi() {
    Serial.println("[WiFi] Скидання налаштувань WiFi...");
    memset(_configMgr->config.wifi_ssid, 0, sizeof(_configMgr->config.wifi_ssid));
    memset(_configMgr->config.wifi_password, 0, sizeof(_configMgr->config.wifi_password));
    _configMgr->save();
    
    WiFi.disconnect();
    startAP();
}

String WifiManager::getIP() {
    if (_state == WIFI_STATE_AP_MODE) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

String WifiManager::getSSID() {
    if (_state == WIFI_STATE_AP_MODE) {
        return _apName;
    }
    return WiFi.SSID();
}

int WifiManager::getRSSI() {
    if (_state == WIFI_STATE_AP_MODE) return 0;
    return WiFi.RSSI();
}

String WifiManager::getAPName() {
    return _apName;
}

void WifiManager::setStateCallback(WiFiStateCallback cb) {
    _stateCallback = cb;
}

void WifiManager::setState(WiFiState newState) {
    if (_state != newState) {
        _state = newState;
        if (_stateCallback) {
            _stateCallback(_state);
        }
    }
}

bool WifiManager::tryConnect(uint32_t timeoutMs) {
    Serial.printf("[WiFi] Підключення до %s...\n", _configMgr->config.wifi_ssid);
    setState(WIFI_STATE_CONNECTING);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(_configMgr->config.wifi_ssid, _configMgr->config.wifi_password);
    
    _connectStartTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - _connectStartTime < timeoutMs) {
        delay(100);
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Підключено!");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
        setState(WIFI_STATE_CONNECTED);
        stopAP();
        return true;
    }
    
    Serial.println("[WiFi] Не вдалося підключитися (таймаут)");
    setState(WIFI_STATE_DISCONNECTED);
    return false;
}

void WifiManager::startAP() {
    Serial.println("[WiFi] Запуск режиму Точки Доступу (AP)...");
    setState(WIFI_STATE_AP_MODE);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_apName.c_str());
    
    Serial.print("[WiFi] AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    if (!_dnsServer) {
        _dnsServer = new DNSServer();
    }
    _dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
    
    setupCaptivePortal();
}

void WifiManager::stopAP() {
    if (_dnsServer) {
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }
    if (_apServer) {
        _apServer->end();
        delete _apServer;
        _apServer = nullptr;
    }
    WiFi.softAPdisconnect(true);
}

void WifiManager::setupCaptivePortal() {
    if (!_apServer) {
        _apServer = new AsyncWebServer(80);
    }
    
    _apServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        request->send(200, "text/html", getCaptivePortalHTML());
    });
    
    _apServer->on("/scan", HTTP_GET, [this](AsyncWebServerRequest *request){
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n; ++i) {
            if (i > 0) json += ",";
            json += "\"" + WiFi.SSID(i) + "\"";
        }
        json += "]";
        request->send(200, "application/json", json);
    });
    
    _apServer->on("/connect", HTTP_POST, [this](AsyncWebServerRequest *request){
        if(request->hasParam("ssid", true) && request->hasParam("pass", true)){
            String ssid = request->getParam("ssid", true)->value();
            String pass = request->getParam("pass", true)->value();
            
            strlcpy(_configMgr->config.wifi_ssid, ssid.c_str(), sizeof(_configMgr->config.wifi_ssid));
            strlcpy(_configMgr->config.wifi_password, pass.c_str(), sizeof(_configMgr->config.wifi_password));
            _configMgr->save();
            
            request->send(200, "text/html", "<html><head><meta charset='utf-8'></head><body style='background-color:#1e1e1e;color:white;text-align:center;font-family:sans-serif;margin-top:50px;'><h1>Збережено!</h1><p>Спроба підключення... Пристрій перезавантажиться, якщо підключення успішне.</p></body></html>");
            
            delay(500); // Give time to send response
            ESP.restart();
        } else {
            request->send(400, "text/plain", "Bad Request");
        }
    });

    _apServer->onNotFound([](AsyncWebServerRequest *request){
        request->redirect("http://192.168.4.1/");
    });
    
    _apServer->begin();
    Serial.println("[WiFi] Captive Portal запущено.");
}

String WifiManager::getCaptivePortalHTML() {
    return String(R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>AlertMap UA - Налаштування WiFi</title>
    <style>
        body {
            background-color: #121212;
            color: #ffffff;
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
        }
        .container {
            background-color: #1e1e1e;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
            width: 100%;
            max-width: 400px;
            text-align: center;
        }
        h1 {
            color: #ff3b30;
            margin-bottom: 20px;
        }
        .form-group {
            margin-bottom: 15px;
            text-align: left;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #aaaaaa;
        }
        select, input[type="text"], input[type="password"] {
            width: 100%;
            padding: 10px;
            border: 1px solid #333;
            border-radius: 5px;
            background-color: #2c2c2c;
            color: white;
            box-sizing: border-box;
        }
        button {
            width: 100%;
            padding: 12px;
            background-color: #ff3b30;
            color: white;
            border: none;
            border-radius: 5px;
            cursor: pointer;
            font-size: 16px;
            margin-top: 10px;
        }
        button:hover {
            background-color: #d32f2f;
        }
        #scan-btn {
            background-color: #4CAF50;
            margin-bottom: 10px;
        }
        #scan-btn:hover {
            background-color: #388E3C;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>AlertMap UA</h1>
        <p>Налаштування підключення до мережі</p>
        
        <button id="scan-btn" onclick="scanNetworks()">Сканувати мережі</button>
        
        <form action="/connect" method="POST">
            <div class="form-group">
                <label for="ssid">Мережа (SSID)</label>
                <select id="network-select" onchange="document.getElementById('ssid').value = this.value" style="display:none; margin-bottom:10px;"></select>
                <input type="text" id="ssid" name="ssid" required placeholder="Введіть назву мережі">
            </div>
            
            <div class="form-group">
                <label for="pass">Пароль</label>
                <input type="password" id="pass" name="pass" placeholder="Введіть пароль (якщо є)">
            </div>
            
            <button type="submit">Підключитися</button>
        </form>
    </div>

    <script>
        function scanNetworks() {
            const btn = document.getElementById('scan-btn');
            const select = document.getElementById('network-select');
            
            btn.innerText = "Сканування...";
            btn.disabled = true;
            
            fetch('/scan')
                .then(response => response.json())
                .then(networks => {
                    select.innerHTML = '<option value="">-- Оберіть мережу --</option>';
                    networks.forEach(net => {
                        select.innerHTML += `<option value="${net}">${net}</option>`;
                    });
                    select.style.display = 'block';
                    btn.innerText = "Сканувати мережі";
                    btn.disabled = false;
                })
                .catch(err => {
                    console.error(err);
                    btn.innerText = "Помилка сканування";
                    setTimeout(() => {
                        btn.innerText = "Сканувати мережі";
                        btn.disabled = false;
                    }, 2000);
                });
        }
    </script>
</body>
</html>
)rawliteral");
}
