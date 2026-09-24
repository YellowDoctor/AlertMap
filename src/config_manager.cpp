#include "config_manager.h"

void ConfigManager::setDefaults() {
    memset(&config, 0, sizeof(config));
    
    strlcpy(config.wifi_ssid, "", sizeof(config.wifi_ssid));
    strlcpy(config.wifi_password, "", sizeof(config.wifi_password));
    strlcpy(config.api_token, "", sizeof(config.api_token));
    config.poll_interval = API_POLL_INTERVAL;
    
    config.color_ok = DEFAULT_COLOR_OK;
    config.color_drones = DEFAULT_COLOR_DRONES;
    config.color_missiles = DEFAULT_COLOR_MISSILES;
    config.color_partial = DEFAULT_COLOR_PARTIAL;
    config.color_offline = DEFAULT_COLOR_OFFLINE;
    
    config.brightness_day = DEFAULT_BRIGHTNESS_DAY;
    config.brightness_night = DEFAULT_BRIGHTNESS_NIGHT;
    
    config.night_mode_enabled = true;
    config.night_start_hour = DEFAULT_NIGHT_START;
    config.night_end_hour = DEFAULT_NIGHT_END;
    
    config.animation_enabled = true;
    config.alert_animation_mode = DEFAULT_ALERT_ANIM_MODE;
    config.pulse_on_alert = true;
    config.wave_on_new_alert = true;
    config.smooth_transitions = true;
    
    strlcpy(config.github_owner, "", sizeof(config.github_owner));
    strlcpy(config.github_repo, "", sizeof(config.github_repo));
    config.auto_check_updates = false;
    
    config.led_type = 1; // WS2815B
    config.color_order = 0; // GRB
}

bool ConfigManager::begin() {
    Serial.println("[ConfigMgr] Ініціалізація LittleFS...");
    if (!LittleFS.begin(true)) {
        Serial.println("[ConfigMgr] Помилка монтування LittleFS!");
        setDefaults();
        return false;
    }
    
    setDefaults();
    
    if (LittleFS.exists(CONFIG_FILE)) {
        return load();
    } else {
        Serial.println("[ConfigMgr] Файл конфігурації не знайдено, створення нового...");
        return save();
    }
}

bool ConfigManager::load() {
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("[ConfigMgr] Не вдалося відкрити конфігураційний файл");
        return false;
    }
    String json = file.readString();
    file.close();
    return fromJson(json);
}

bool ConfigManager::save() {
    String json = toJson();
    File file = LittleFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("[ConfigMgr] Не вдалося записати конфігураційний файл");
        return false;
    }
    file.print(json);
    file.close();
    Serial.println("[ConfigMgr] Конфігурація успішно збережена");
    return true;
}

String ConfigManager::toJson() {
    JsonDocument doc;
    
    doc["wifi_ssid"] = config.wifi_ssid;
    doc["wifi_password"] = config.wifi_password;
    doc["api_token"] = config.api_token;
    doc["poll_interval"] = config.poll_interval;
    
    doc["color_ok"] = config.color_ok;
    doc["color_drones"] = config.color_drones;
    doc["color_missiles"] = config.color_missiles;
    doc["color_partial"] = config.color_partial;
    doc["color_offline"] = config.color_offline;
    
    doc["brightness_day"] = config.brightness_day;
    doc["brightness_night"] = config.brightness_night;
    
    doc["night_mode_enabled"] = config.night_mode_enabled;
    doc["night_start_hour"] = config.night_start_hour;
    doc["night_end_hour"] = config.night_end_hour;
    
    doc["animation_enabled"] = config.animation_enabled;
    doc["alert_animation_mode"] = config.alert_animation_mode;
    doc["pulse_on_alert"] = config.pulse_on_alert;
    doc["wave_on_new_alert"] = config.wave_on_new_alert;
    doc["smooth_transitions"] = config.smooth_transitions;
    
    doc["github_owner"] = config.github_owner;
    doc["github_repo"] = config.github_repo;
    doc["auto_check_updates"] = config.auto_check_updates;
    
    doc["led_type"] = config.led_type;
    doc["color_order"] = config.color_order;
    
    String output;
    serializeJson(doc, output);
    return output;
}

bool ConfigManager::fromJson(const String& json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        Serial.println("[ConfigMgr] Помилка десеріалізації JSON");
        return false;
    }
    
    strlcpy(config.wifi_ssid, doc["wifi_ssid"] | "", sizeof(config.wifi_ssid));
    strlcpy(config.wifi_password, doc["wifi_password"] | "", sizeof(config.wifi_password));
    strlcpy(config.api_token, doc["api_token"] | "", sizeof(config.api_token));
    config.poll_interval = doc["poll_interval"] | API_POLL_INTERVAL;
    
    config.color_ok = doc["color_ok"] | DEFAULT_COLOR_OK;
    config.color_drones = doc["color_drones"] | DEFAULT_COLOR_DRONES;
    config.color_missiles = doc["color_missiles"] | DEFAULT_COLOR_MISSILES;
    config.color_partial = doc["color_partial"] | DEFAULT_COLOR_PARTIAL;
    config.color_offline = doc["color_offline"] | DEFAULT_COLOR_OFFLINE;
    
    config.brightness_day = doc["brightness_day"] | DEFAULT_BRIGHTNESS_DAY;
    config.brightness_night = doc["brightness_night"] | DEFAULT_BRIGHTNESS_NIGHT;
    
    config.night_mode_enabled = doc["night_mode_enabled"] | true;
    config.night_start_hour = doc["night_start_hour"] | DEFAULT_NIGHT_START;
    config.night_end_hour = doc["night_end_hour"] | DEFAULT_NIGHT_END;
    
    config.animation_enabled = doc["animation_enabled"] | true;
    config.alert_animation_mode = doc["alert_animation_mode"] | (uint8_t)DEFAULT_ALERT_ANIM_MODE;
    config.pulse_on_alert = doc["pulse_on_alert"] | true;
    config.wave_on_new_alert = doc["wave_on_new_alert"] | true;
    config.smooth_transitions = doc["smooth_transitions"] | true;
    
    strlcpy(config.github_owner, doc["github_owner"] | "", sizeof(config.github_owner));
    strlcpy(config.github_repo, doc["github_repo"] | "", sizeof(config.github_repo));
    config.auto_check_updates = doc["auto_check_updates"] | false;
    
    config.led_type = doc["led_type"] | 1;
    config.color_order = doc["color_order"] | 0;
    
    return true;
}

bool ConfigManager::updateFromJson(const String& json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) return false;
    
    if (doc["wifi_ssid"].is<const char*>()) strlcpy(config.wifi_ssid, doc["wifi_ssid"], sizeof(config.wifi_ssid));
    if (doc["wifi_password"].is<const char*>()) strlcpy(config.wifi_password, doc["wifi_password"], sizeof(config.wifi_password));
    if (doc["api_token"].is<const char*>()) strlcpy(config.api_token, doc["api_token"], sizeof(config.api_token));
    if (doc["poll_interval"].is<uint32_t>()) config.poll_interval = doc["poll_interval"];
    
    if (doc["color_ok"].is<uint32_t>()) config.color_ok = doc["color_ok"];
    if (doc["color_drones"].is<uint32_t>()) config.color_drones = doc["color_drones"];
    if (doc["color_missiles"].is<uint32_t>()) config.color_missiles = doc["color_missiles"];
    if (doc["color_partial"].is<uint32_t>()) config.color_partial = doc["color_partial"];
    if (doc["color_offline"].is<uint32_t>()) config.color_offline = doc["color_offline"];
    
    if (doc["brightness_day"].is<uint8_t>()) config.brightness_day = doc["brightness_day"];
    if (doc["brightness_night"].is<uint8_t>()) config.brightness_night = doc["brightness_night"];
    
    if (doc["night_mode_enabled"].is<bool>()) config.night_mode_enabled = doc["night_mode_enabled"];
    if (doc["night_start_hour"].is<uint8_t>()) config.night_start_hour = doc["night_start_hour"];
    if (doc["night_end_hour"].is<uint8_t>()) config.night_end_hour = doc["night_end_hour"];
    
    if (doc["animation_enabled"].is<bool>()) config.animation_enabled = doc["animation_enabled"];
    if (doc["alert_animation_mode"].is<uint8_t>()) config.alert_animation_mode = doc["alert_animation_mode"];
    if (doc["pulse_on_alert"].is<bool>()) config.pulse_on_alert = doc["pulse_on_alert"];
    if (doc["wave_on_new_alert"].is<bool>()) config.wave_on_new_alert = doc["wave_on_new_alert"];
    if (doc["smooth_transitions"].is<bool>()) config.smooth_transitions = doc["smooth_transitions"];
    
    if (doc["github_owner"].is<const char*>()) strlcpy(config.github_owner, doc["github_owner"], sizeof(config.github_owner));
    if (doc["github_repo"].is<const char*>()) strlcpy(config.github_repo, doc["github_repo"], sizeof(config.github_repo));
    if (doc["auto_check_updates"].is<bool>()) config.auto_check_updates = doc["auto_check_updates"];
    
    if (doc["led_type"].is<uint8_t>()) config.led_type = doc["led_type"];
    if (doc["color_order"].is<uint8_t>()) config.color_order = doc["color_order"];
    
    return true;
}