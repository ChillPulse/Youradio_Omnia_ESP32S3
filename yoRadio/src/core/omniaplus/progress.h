#ifndef OMNIA_PROGRESS_H
#define OMNIA_PROGRESS_H
#include <Arduino.h>
// PROGRESS + META + TRACK + SD/USB STATUS для OMNIA v8.2
// Тестируется через UART ESP32-S3 + Web UI + на слух, без обязательного STM32 UART

struct ProgressInfo {
  uint32_t curMs;
  uint32_t durMs;
  uint8_t state; //0 stop 1 play 2 pause 3 seeking 4 buffering 5 error
  uint16_t percentX10; //0..1000
  uint16_t fileIdx;
  uint16_t fileTotal;
};

void omnia_progress_init();
void omnia_progress_loop(); // вызывать из player.loop() ~2-5Hz, до 10Hz при seek
void omnia_progress_send(const ProgressInfo &p);
void omnia_meta_send(const char* title, const char* artist, const char* album, const char* year, int bitrate, const char* fmt, int sr, int ch);
void omnia_track_send(uint16_t idx, uint16_t total, const char* path);
void omnia_sd_status_send(const char* state, const char* fs, uint64_t size, int files);
void omnia_usb_status_send(const char* state, const char* fs, uint64_t size, int files);

#endif
