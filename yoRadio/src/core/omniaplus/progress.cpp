#include "progress.h"
#include "../player.h"
// #include "../../audioI2S/Audio.h" removed — player.h already includes AudioEx.h
#include "../config.h"

static uint32_t lastProgressMs = 0;
static bool isSeeking = false;

void omnia_progress_init(){ lastProgressMs=0; }

void omnia_progress_send(const ProgressInfo &p){
  Serial.printf("##PROGRESS#: %lu %lu %u %u %u %u\n", (unsigned long)p.curMs, (unsigned long)p.durMs, p.state, p.percentX10, p.fileIdx, p.fileTotal);
}

void omnia_meta_send(const char* title, const char* artist, const char* album, const char* year, int bitrate, const char* fmt, int sr, int ch){
  Serial.printf("##META#: title=%s;artist=%s;album=%s;year=%s;bitrate=%d;fmt=%s;sr=%d;ch=%d\n",
    title?title:"", artist?artist:"", album?album:"", year?year:"", bitrate, fmt?fmt:"", sr, ch);
}

void omnia_track_send(uint16_t idx, uint16_t total, const char* path){
  Serial.printf("##TRACK#: idx=%u;total=%u;path=%s\n", idx, total, path?path:"");
}

void omnia_sd_status_send(const char* state, const char* fs, uint64_t size, int files){
  Serial.printf("##SD.STATUS#: %s size=%llu fs=%s files=%d\n", state?state:"unknown", (unsigned long long)size, fs?fs:"", files);
}

void omnia_usb_status_send(const char* state, const char* fs, uint64_t size, int files){
  Serial.printf("##USB.STATUS#: %s size=%llu fs=%s files=%d\n", state?state:"unknown", (unsigned long long)size, fs?fs:"", files);
}

void omnia_progress_loop(){
  uint32_t now = millis();
  uint32_t interval = isSeeking ? 100 : 300;
  if(now - lastProgressMs < interval) return;
  lastProgressMs = now;
  uint32_t cur = 0, dur = 0;
  // TODO: replace with real AudioEx calls: player.getAudioCurrentTime() etc.
  ProgressInfo p;
  p.curMs = cur;
  p.durMs = dur;
  p.state = 1;
  p.percentX10 = dur ? (uint16_t)(cur*1000/dur) : 0;
  p.fileIdx = 1;
  p.fileTotal = 1;
  //if(dur>0 || cur>0) omnia_progress_send(p);
}
