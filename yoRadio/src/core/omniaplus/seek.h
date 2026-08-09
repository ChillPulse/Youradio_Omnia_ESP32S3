#ifndef OMNIA_SEEK_H
#define OMNIA_SEEK_H
#include <Arduino.h>
// Seek handling for v8.2 — short NEXT/PREV vs hold seek acceleration

void omnia_seek_init();
bool omnia_seek_handle(const char* cmd); // возвращает true если cmd обработан как seek
void omnia_seek_absolute(uint32_t ms);
void omnia_seek_relative(int32_t deltaMs);
void omnia_seek_percent(uint16_t permille); //0..1000
void omnia_seek_start(bool forward); // seek_start + / -
void omnia_seek_stop();
void omnia_seek_loop(); // Chat25: must be called from Player::loop() for hold acceleration

// Hold acceleration: 0-2s ±5s, 2-4s ±10s, 4-7s ±30s, >7s ±60s
int32_t omnia_seek_accelerated_step(uint32_t holdMs);

#endif
