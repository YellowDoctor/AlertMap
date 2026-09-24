#include <Arduino.h>
#include "config.h"
#include "version.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "alert_service.h"
#include "led_controller.h"
#include "night_mode.h"
#include "ota_manager.h"
#include "statistics.h"
#include "web_server.h"

#ifndef FIRMWARE_NAME
#define FIRMWARE_NAME "AlertMap UA"
#endif

#ifndef FIRMWARE_BUILD_DATE
#define FIRMWARE_BUILD_DATE __DATE__
#endif

#ifndef FIRMWARE_BUILD_TIME
#define FIRMWARE_BUILD_TIME __TIME__
#endif

// Глобальні екземпляри класів
ConfigManager configManager;
WifiManager wifiManager;
AlertService alertService;
LEDController ledController; // AnimationEngine інтегрований всередині
NightMode nightMode;
OTAManager otaManager;
Statistics stats;
AppWebServer webServer;

// Колбек стану WiFi - оновлює LED індикацію
void onWiFiStateChange(WiFiState state) {
    switch (state) {
        case WIFI_STATE_CONNECTING:
            ledController.showConnecting();
            break;
        case WIFI_STATE_AP_MODE:
            // Відображаємо сині світлодіоди для режиму точки доступу
            ledController.showOffline();
            break;
        case WIFI_STATE_CONNECTED:
            Serial.println("[Main] WiFi підключено, запуск сервісів...");
            break;
        case WIFI_STATE_DISCONNECTED:
            ledController.showOffline();
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("================================");
    Serial.printf("%s v%s\n", FIRMWARE_NAME, FIRMWARE_VERSION);
    Serial.printf("Build: %s %s\n", FIRMWARE_BUILD_DATE, FIRMWARE_BUILD_TIME);
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.println("================================");
    
    // 1. Ініціалізація конфігурації
    configManager.begin();
    
    // 2. Ініціалізація LED контролера + стартова анімація
    ledController.begin(configManager);
    ledController.showStartupAnimation();
    
    // 3. Ініціалізація статистики
    stats.begin();
    
    // 4. Підключення до WiFi (або запуск AP)
    wifiManager.setStateCallback(onWiFiStateChange);
    wifiManager.begin(configManager);
    
    // 5. Ініціалізація NTP / нічного режиму
    nightMode.begin(configManager);
    
    // 6. Ініціалізація менеджера OTA
    otaManager.begin(configManager);
    
    // 7. Ініціалізація сервісу тривог
    alertService.begin(configManager);
    
    // 8. Запуск веб-сервера
    webServer.begin(configManager, alertService, ledController, stats, otaManager, nightMode, wifiManager);
    
    Serial.println("[Main] Налаштування завершено!");
    Serial.printf("[Main] Вільна пам'ять після налаштування: %d байт\n", ESP.getFreeHeap());
}

void loop() {
    // Управління WiFi
    wifiManager.update();
    
    // Отримуємо тривоги лише при підключеному WiFi
    if (wifiManager.getState() == WIFI_STATE_CONNECTED) {
        alertService.update();
        
        // Перевіряємо наявність нових тривог та запускаємо анімацію хвилі
        if (alertService.hasNewAlerts()) {
            ledController.triggerWave();
            alertService.clearNewAlertsFlag();
        }
        
        // Застосовуємо рівні тривог до світлодіодів
        ledController.setAllAlerts(alertService.getAllAlerts());
    }
    
    // Яскравість нічного режиму
    nightMode.update();
    ledController.setBrightness(nightMode.getCurrentBrightness());
    
    // Оновлення світлодіодів (відображення) - також обробляє внутрішні анімації
    ledController.update();
    
    // Статистика
    stats.update();
    
    // Невелика затримка для запобігання проблем з watchdog на ESP32-C3
    delay(10);
}
