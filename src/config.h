#pragma once

#include <Arduino.h>

// Піни та налаштування LED
constexpr uint8_t LED_PIN = 4;
constexpr uint16_t NUM_LEDS = 128;
constexpr uint16_t NUM_DISTRICTS = 128;

// Режими анімації при появі нової тривоги
enum AlertAnimMode : uint8_t {
    ANIM_SMOOTH = 0,   // Плавний перехід кольору
    ANIM_BLINK = 1,    // Миготіння району (5 разів)
    ANIM_STROBE = 2,   // Стробоскоп (швидкі яскраві спалахи)
    ANIM_WAVE = 3,     // Хвиля по всій карті
    ANIM_BREATHE = 4   // Постійна пульсація (дихання)
};

constexpr uint8_t DEFAULT_ALERT_ANIM_MODE = ANIM_BLINK;

// API налаштування
constexpr const char* API_BASE_URL = "https://api.alerts.in.ua";
constexpr const char* API_ENDPOINT = "/v1/alerts/active.json";
constexpr uint32_t API_POLL_INTERVAL = 10000; // 10 секунд (мс)

// Налаштування Точки Доступу (AP)
constexpr const char* AP_SSID_PREFIX = "AlertMap";
constexpr const char* AP_PASSWORD = "alertmap1";

// Кольори за замовчуванням (у форматі HEX)
constexpr uint32_t DEFAULT_COLOR_OK = 0x00FF00;       // Зелений
constexpr uint32_t DEFAULT_COLOR_DRONES = 0xFFFF00;   // Жовтий
constexpr uint32_t DEFAULT_COLOR_MISSILES = 0xFF0000; // Червоний
constexpr uint32_t DEFAULT_COLOR_PARTIAL = 0xFF8000;  // Помаранчевий
constexpr uint32_t DEFAULT_COLOR_OFFLINE = 0x0000FF;  // Синій

// Нічний режим за замовчуванням
constexpr uint8_t DEFAULT_BRIGHTNESS_DAY = 128;
constexpr uint8_t DEFAULT_BRIGHTNESS_NIGHT = 20;
constexpr uint8_t DEFAULT_NIGHT_START = 23; // 23:00
constexpr uint8_t DEFAULT_NIGHT_END = 7;    // 07:00

// NTP сервер для синхронізації часу
constexpr const char* NTP_SERVER = "pool.ntp.org";
constexpr long NTP_OFFSET_SEC = 7200; // Київ (UTC+2)

// Інші налаштування
constexpr const char* CONFIG_FILE = "/config.json";
constexpr uint8_t MAX_EVENTS_LOG = 20;