#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"

struct AppConfig {
    // WiFi
    char wifi_ssid[64];
    char wifi_password[64];
    
    // API
    char api_token[128];
    uint32_t poll_interval;      // мс
    
    // Кольори (RGB uint32_t)
    uint32_t color_ok;           // зелений
    uint32_t color_drones;       // жовтий
    uint32_t color_missiles;     // червоний  
    uint32_t color_partial;      // помаранчевий
    uint32_t color_offline;      // синій
    
    // Яскравість
    uint8_t brightness_day;
    uint8_t brightness_night;
    
    // Нічний режим
    bool night_mode_enabled;
    uint8_t night_start_hour;
    uint8_t night_end_hour;
    
    // Анімації
    bool animation_enabled;
    uint8_t alert_animation_mode; // 0=Smooth, 1=Blink, 2=Strobe, 3=Wave, 4=Breathe
    bool pulse_on_alert;
    bool wave_on_new_alert;
    bool smooth_transitions;
    
    // OTA / GitHub
    char github_owner[64];
    char github_repo[64];
    bool auto_check_updates;
    
    // LED тип
    uint8_t led_type;      // 0=WS2812B, 1=WS2815, 2=SK6812, 3=GS8208
    uint8_t color_order;   // 0=GRB, 1=RGB, 2=BRG
};

class ConfigManager {
public:
    AppConfig config;
    
    bool begin();
    bool load();
    bool save();
    void setDefaults();
    String toJson();
    bool fromJson(const String& json);
    bool updateFromJson(const String& json);
};