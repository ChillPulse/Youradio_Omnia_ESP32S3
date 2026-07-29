#ifndef myoptions_omnia_final_h
#define myoptions_omnia_final_h
// OMNIA FINAL pinmap for YoRadio ESP32-S3 (v8.2) — 5 sources FINAL
// БАЗА из myoptions.h-generator + добавки V7G + USB Host
// https://github.com/ChillPulse/Youradio_Omnia_ESP32S3

#define ARDUINO_ESP32S3_DEV
#define L10N_LANGUAGE RU

// I2S к STM32/AT32 (AT32 MASTER, S3 SLAVE) — проверенный V7G рубеж
#define I2S_DOUT  17  // ESP → STM PB4 DATA OUT (I2S3ext_SD)
#define I2S_BCLK  16  // STM PB3 → ESP BCLK
#define I2S_LRC   18  // STM PA15 → ESP LRCK/WS
#define I2S_MCLK  -1  // не используем, PCM5102A external MCLK не требует, MCLK нужен AT32 для Fs детекта

// Управление
#define MUTE_PIN  2   // ESP GPIO2 → STM PA8 MUTE HIGH=mute
#define STM_RATE_PIN 4 // ESP GPIO4 → STM PA9 RATE LOW=44.1 HIGH=48k
#define SDC_CS    39  // SD CS (SPI SD)

// USB-Flash Host MSC — ESP32-S3 OTG на GPIO19/20 (фиксировано в чипе, не настраивается через myoptions, но документируем)
// GPIO19 = USB D- , GPIO20 = USB D+ , VBUS 5V 1A+ отдельный LDO + 100nF+10uF + TVS опц + polyfuse 0.75A 6V на разъеме USB-A female
// Onboard USB-C — только Device, Host через 19/20 — правильный путь (ты уже сделал)

// Опционально: LED, display etc — оставляем как в основном myoptions.h
// #define REAL_LEDBUILTIN 48 // пример

// Для OMNIA интеграции
#define OMNIA_UART_BAUD 921600 // финальный для AT32, для стенда STM32 пока 115200
#define STM32_DEBUG_UART 1 // 1 оставь для логов s/s1000, 0 можно выключить — функционал не ломается (ответ на вопрос нужен ли UART STM32 — для модернизации полезен как debug, но не обязателен)

#endif
