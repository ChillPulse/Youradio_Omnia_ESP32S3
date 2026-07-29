#ifndef OMNIA_SHUFFLE_FIX_H
#define OMNIA_SHUFFLE_FIX_H
#include <Arduino.h>

enum RepeatMode { REPEAT_OFF, REPEAT_ONE, REPEAT_ALL };
enum ShuffleMode { SHUFFLE_OFF, SHUFFLE_ON };

struct PlaylistState {
  uint16_t total;
  uint16_t currentIdx;
  ShuffleMode shuffle;
  RepeatMode repeat;
  uint16_t *shuffledOrder;
  uint16_t shuffledPos;
};

void omnia_shuffle_init(uint16_t total);
void omnia_shuffle_set(ShuffleMode s);
void omnia_shuffle_set_repeat(RepeatMode r);
RepeatMode omnia_shuffle_get_repeat();
ShuffleMode omnia_shuffle_get_shuffle();
PlaylistState* omnia_shuffle_get_state();
uint16_t omnia_shuffle_next();
uint16_t omnia_shuffle_prev();
void omnia_shuffle_on_track_change(uint16_t newIdx);
bool omnia_shuffle_is_last();

#endif
