#include "progress.h"
#include "../player.h"
#include "../config.h"
#include <Arduino.h>

static uint32_t lastProgressMs = 0;

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
  if(now - lastProgressMs < 300) return; // ~3Hz, avoid spam
  lastProgressMs = now;
  if(!player.isRunning()) return;
  // Fix as per chat recommendation: don't hardcode !=1, allow SD and USB (and future)
  // PM_WEB =0, PM_SDCARD=1
  if(config.getMode()==0) return; // was !=1, now only skip WEB

  uint32_t filePos = player.getFilePos();
  uint32_t fileSize = player.getFileSize();
  uint32_t bitrate = player.getBitRate();
  uint32_t durSec = player.getAudioFileDuration();
  uint32_t curSec = player.getAudioCurrentTime();

  uint32_t curMs = curSec*1000UL;
  uint32_t durMs = durSec*1000UL;

  if(durMs==0 && fileSize>0 && bitrate>0){
    durMs = (uint64_t)fileSize*8*1000 / bitrate;
  }

  uint16_t percentX10 = 0;
  if(fileSize>0 && filePos>0){
    percentX10 = (uint64_t)filePos*1000 / fileSize;
    if(durMs>0 && curMs==0){
      curMs = (uint64_t)percentX10 * durMs / 1000;
    }
  }else if(durMs>0 && curMs>0){
    percentX10 = (uint64_t)curMs*1000 / durMs;
  }

  if(percentX10>1000) percentX10=1000;
  if(durMs>0 && curMs>durMs) curMs=durMs;

  uint16_t idx = config.lastStation();
  uint16_t total = config.playlistLength();

  ProgressInfo p;
  p.curMs = curMs;
  p.durMs = durMs;
  p.state = 1;
  p.percentX10 = percentX10;
  p.fileIdx = idx;
  p.fileTotal = total;

  // Fix as per chat: always send, even if 0/0, so UI doesn't think no progress
  omnia_progress_send(p);
}
