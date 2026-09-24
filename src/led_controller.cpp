#include "led_controller.h"
#include "config_manager.h"

void LEDController::begin(ConfigManager& configMgr) {
    _configMgr = &configMgr;
    
    // WS2815B 12V адресна стрічка на GPIO 4
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(_leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    _currentBrightness = _configMgr ? _configMgr->config.brightness_day : DEFAULT_BRIGHTNESS_DAY;
    FastLED.setBrightness(_currentBrightness);
    
    _animEngine.begin();
    if (_configMgr) {
        _animEngine.setMode(_configMgr->config.alert_animation_mode);
        _animEngine.setPulseEnabled(_configMgr->config.pulse_on_alert);
        _animEngine.setSmoothEnabled(_configMgr->config.smooth_transitions);
        _animEngine.setWaveEnabled(_configMgr->config.wave_on_new_alert);
    }
    
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        _alertLevels[i] = ALERT_OK;
        _targetColors[i] = alertLevelToColor(ALERT_OK);
        _leds[i] = _targetColors[i];
    }
    FastLED.show();
    _initialized = true;
    Serial.println("[LED] WS2815B стрічка успішно ініціалізована на GPIO 4");
}

void LEDController::update() {
    if (!_initialized) return;
    _animEngine.update(_leds, _targetColors, _alertLevels, NUM_LEDS);
    FastLED.show();
}

void LEDController::setDistrictAlert(uint8_t ledIndex, AlertLevel level) {
    if (ledIndex >= NUM_LEDS) return;
    
    // Тригер анімації при появі нової тривоги
    if (level > _alertLevels[ledIndex] && level != ALERT_OK && level != ALERT_OFFLINE) {
        _animEngine.triggerDistrictAlert(ledIndex);
    }
    
    _alertLevels[ledIndex] = level;
    _targetColors[ledIndex] = alertLevelToColor(level);
}

void LEDController::setAllAlerts(const AlertLevel* levels) {
    if (!levels) return;
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        setDistrictAlert(i, levels[i]);
    }
}

void LEDController::setBrightness(uint8_t brightness) {
    _currentBrightness = brightness;
    FastLED.setBrightness(_currentBrightness);
}

void LEDController::showTestPattern() {
    Serial.println("[LED] Тестовий режим: Веселка");
    fill_rainbow(_leds, NUM_LEDS, 0, 255 / NUM_LEDS);
    FastLED.show();
}

void LEDController::showStartupAnimation() {
    Serial.println("[LED] Стартова анімація карти");
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        _leds[i] = CRGB::Yellow;
        if (i > 0) _leds[i-1] = CRGB::Blue;
        FastLED.show();
        delay(15);
    }
    delay(200);
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        _targetColors[i] = alertLevelToColor(ALERT_OK);
        _leds[i] = _targetColors[i];
    }
    FastLED.show();
}

void LEDController::showOffline() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        _targetColors[i] = alertLevelToColor(ALERT_OFFLINE);
        _leds[i] = _targetColors[i];
    }
    FastLED.show();
}

void LEDController::showConnecting() {
    uint32_t now = millis();
    bool blink = ((now / 400) % 2) == 0;
    CRGB color = blink ? alertLevelToColor(ALERT_DRONES) : CRGB::Black;
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        _leds[i] = color;
    }
    FastLED.show();
}

AlertLevel LEDController::getDistrictAlert(uint8_t ledIndex) const {
    if (ledIndex >= NUM_LEDS) return ALERT_OK;
    return _alertLevels[ledIndex];
}

CRGB LEDController::alertLevelToColor(AlertLevel level) const {
    if (!_configMgr) {
        switch (level) {
            case ALERT_OK:       return CRGB(0, 255, 0);
            case ALERT_DRONES:   return CRGB(255, 255, 0);
            case ALERT_PARTIAL:  return CRGB(255, 128, 0);
            case ALERT_MISSILES: return CRGB(255, 0, 0);
            case ALERT_OFFLINE:  return CRGB(0, 0, 255);
            default:             return CRGB::Black;
        }
    }
    uint32_t c = 0;
    switch (level) {
        case ALERT_OK:       c = _configMgr->config.color_ok; break;
        case ALERT_DRONES:   c = _configMgr->config.color_drones; break;
        case ALERT_PARTIAL:  c = _configMgr->config.color_partial; break;
        case ALERT_MISSILES: c = _configMgr->config.color_missiles; break;
        case ALERT_OFFLINE:  c = _configMgr->config.color_offline; break;
        default:             return CRGB::Black;
    }
    return CRGB((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
}