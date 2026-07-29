#include "progress.h"
#include "../player.h"
#include "../config.h"
#include <Arduino.h>

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
  uint32_t interval = isSeeking ? 100 : 500; // 2Hz normally, 10Hz when seeking
  if(now - lastProgressMs < interval) return;
  lastProgressMs = now;
  if(!player.isRunning()) return;
  if(config.getMode()!=1) return; // only SD mode for now (PM_SDCARD =1)
  uint32_t curSec = player.getAudioCurrentTime();
  uint32_t durSec = player.getAudioFileDuration();
  uint32_t filePos = player.getFilePos();
  uint32_t fileSize = player.getFileSize();
  if(durSec==0 && fileSize==0) return;
  uint32_t curMs = curSec*1000UL;
  uint32_t durMs = durSec*1000UL;
  uint16_t percentX10 = 0;
  if(durMs>0) percentX10 = (uint16_t)(curMs*1000/durMs);
  else if(fileSize>0) percentX10 = (uint16_t)(filePos*1000/fileSize);
  uint16_t idx = config.lastStation();
  uint16_t total = config.playlistLength();
  ProgressInfo p;
  p.curMs = curMs;
  p.durMs = durMs;
  p.state = 1;
  p.percentX10 = percentX10;
  p.fileIdx = idx;
  p.fileTotal = total;
  // Send only if duration known
  if(durMs>0 || fileSize>0){
    omnia_progress_send(p);
  }
}
