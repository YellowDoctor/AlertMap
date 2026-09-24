#include "animations.h"
#include "led_controller.h"

void AnimationEngine::begin() {
    Serial.println("[Anim] Анімаційний рушій запущено");
    _lastUpdateTime = millis();
    _waveActive = false;
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        _districtTriggerTime[i] = 0;
        _districtAnimating[i] = false;
    }
}

void AnimationEngine::triggerWave() {
    _waveActive = true;
    _waveStartTime = millis();
}

void AnimationEngine::triggerDistrictAlert(uint8_t ledIndex) {
    if (ledIndex < NUM_LEDS) {
        _districtAnimating[ledIndex] = true;
        _districtTriggerTime[ledIndex] = millis();
        
        // Якщо обрано режим хвилі — також запускаємо хвилю по всій карті
        if (_mode == ANIM_WAVE) {
            triggerWave();
        }
    }
}

void AnimationEngine::update(CRGB* leds, CRGB* targetColors, AlertLevel* alertLevels, uint8_t numLeds) {
    uint32_t now = millis();
    if (now - _lastUpdateTime < 20) return;
    _lastUpdateTime = now;

    // 1. Базовий плавний перехід кольору
    if (_smoothEnabled) {
        applySmoothTransition(leds, targetColors, numLeds);
    } else {
        for (uint8_t i = 0; i < numLeds; i++) leds[i] = targetColors[i];
    }

    // 2. Спеціальний режим інтро-анімації для нових тривог
    applyDistrictAlertIntro(leds, targetColors, alertLevels, numLeds, now);

    // 3. Пульсація / дихання
    if (_mode == ANIM_BREATHE || _pulseEnabled) {
        applyPulse(leds, alertLevels, numLeds);
    }

    // 4. Хвиля по карті
    if (_waveEnabled && _waveActive) {
        applyWave(leds, numLeds);
    }
}

void AnimationEngine::applyDistrictAlertIntro(CRGB* leds, CRGB* targets, AlertLevel* alertLevels, uint8_t numLeds, uint32_t now) {
    for (uint8_t i = 0; i < numLeds; i++) {
        if (!_districtAnimating[i]) continue;
        
        uint32_t elapsed = now - _districtTriggerTime[i];

        switch (_mode) {
            case ANIM_BLINK: {
                // Миготіння: 5 спалахів по 400 мс (загалом 2000 мс)
                if (elapsed < 2000) {
                    bool on = ((elapsed / 200) % 2) == 0;
                    leds[i] = on ? targets[i] : CRGB::Black;
                } else {
                    _districtAnimating[i] = false;
                }
                break;
            }

            case ANIM_STROBE: {
                // Стробоскоп: 8 швидких яскраво-білих спалахів по 75 мс (загалом 1200 мс)
                if (elapsed < 1200) {
                    bool flash = ((elapsed / 75) % 2) == 0;
                    leds[i] = flash ? CRGB(255, 255, 255) : CRGB::Black;
                } else {
                    _districtAnimating[i] = false;
                }
                break;
            }

            case ANIM_WAVE: {
                // Хвиля триває 1500 мс
                if (elapsed >= WAVE_DURATION) {
                    _districtAnimating[i] = false;
                }
                break;
            }

            case ANIM_SMOOTH:
            case ANIM_BREATHE:
            default: {
                // У плавному режимі або диханні додаткова інтро-анімація не потрібна
                if (elapsed > 1000) {
                    _districtAnimating[i] = false;
                }
                break;
            }
        }
    }
}

void AnimationEngine::applySmoothTransition(CRGB* leds, CRGB* targets, uint8_t numLeds) {
    for (uint8_t i = 0; i < numLeds; i++) {
        // Якщо світлодіод зараз активно миготить чи стробує, плавний blend не заважає
        if (!_districtAnimating[i] || _mode == ANIM_SMOOTH || _mode == ANIM_WAVE || _mode == ANIM_BREATHE) {
            leds[i] = blend(leds[i], targets[i], BLEND_SPEED);
        }
    }
}

void AnimationEngine::applyPulse(CRGB* leds, AlertLevel* alertLevels, uint8_t numLeds) {
    uint32_t now = millis();
    uint8_t phase = (now * 255) / PULSE_PERIOD;
    uint8_t pulseIntensity = sin8(phase);
    
    // Більш виражене дихання для режиму ANIM_BREATHE (від 40% до 100%)
    uint8_t minScale = (_mode == ANIM_BREATHE) ? 96 : 140;
    uint8_t scale = map(pulseIntensity, 0, 255, minScale, 255);

    for (uint8_t i = 0; i < numLeds; i++) {
        if (alertLevels[i] != ALERT_OK && alertLevels[i] != ALERT_OFFLINE) {
            if (!_districtAnimating[i] || _mode == ANIM_BREATHE) {
                leds[i].nscale8(scale);
            }
        }
    }
}

void AnimationEngine::applyWave(CRGB* leds, uint8_t numLeds) {
    uint32_t elapsed = millis() - _waveStartTime;
    if (elapsed > WAVE_DURATION) {
        _waveActive = false;
        return;
    }

    float progress = (float)elapsed / WAVE_DURATION;
    int waveCenter = progress * (numLeds + WAVE_WIDTH) - (WAVE_WIDTH / 2);

    for (uint8_t i = 0; i < numLeds; i++) {
        int dist = abs(waveCenter - i);
        if (dist < WAVE_WIDTH) {
            uint8_t whiteAmount = map(dist, 0, WAVE_WIDTH, 220, 0);
            leds[i] = blend(leds[i], CRGB::White, whiteAmount);
        }
    }
}