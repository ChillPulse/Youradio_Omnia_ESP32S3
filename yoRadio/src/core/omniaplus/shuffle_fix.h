#ifndef OMNIA_SHUFFLE_FIX_H
#define OMNIA_SHUFFLE_FIX_H
#include <Arduino.h>
// Фикс бага shuffle пропуска 1 трека + Repeat modes OFF/ONE/ALL + No Repeat

enum RepeatMode { REPEAT_OFF, REPEAT_ONE, REPEAT_ALL };
enum ShuffleMode { SHUFFLE_OFF, SHUFFLE_ON };

struct PlaylistState {
  uint16_t total;
  uint16_t currentIdx; //1-based
  ShuffleMode shuffle;
  RepeatMode repeat;
  // Для shuffle без повторов
  uint16_t *shuffledOrder; // массив индексов
  uint16_t shuffledPos;
};

void omnia_shuffle_init(uint16_t total);
void omnia_shuffle_set(ShuffleMode s);
void omnia_shuffle_set_repeat(RepeatMode r);
uint16_t omnia_shuffle_next(); // возвращает следующий idx без пропуска, 0 если стоп
uint16_t omnia_shuffle_prev();
void omnia_shuffle_on_track_change(uint16_t newIdx);

// Тест на слух + UART: 10 треков shuffle repeat off → 10 уникальных, затем стоп, без пропуска
// Старый баг: off-by-one when idx==size-1, random(1,total) не включал последний или пропускал один — фикс: random(1,total+1) + проверка visited set

#endif
