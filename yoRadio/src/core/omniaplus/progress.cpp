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
  uint32_t interval = isSeeking ? 100 : 500;
  if(now - lastProgressMs < interval) return;
  lastProgressMs = now;
  if(!player.isRunning()) return;
  if(config.getMode()!=1) return; // PM_SDCARD =1

  uint32_t curSec = player.getAudioCurrentTime(); // can be 0 for MP3 early
  uint32_t durSec = player.getAudioFileDuration(); // can be 0 if not yet estimated
  uint32_t filePos = player.getFilePos();
  uint32_t fileSize = player.getFileSize();
  uint32_t bitrate = 0;
  // Try to get bitrate from station
  // config.station.bitrate is in kbps? Actually it's in kbps? In log 169892 is bps? It's 169892
  // Use player bitrate if available via getBitRate? There's no direct API, but we have config.station.bitrate
  // For fallback, use fileSize and bitrate to estimate duration
  // fileSize is total file size, but audio data size may be less due to ID3, but close enough

  // If durSec is 0 but fileSize and bitrate known, estimate dur
  if(durSec==0 && fileSize>0){
    // Try to get bitrate from config or use 128k default
    // In SD mode, config.station.bitrate may be 0, but we have in log BitRate 169892 etc from Audio
    // We can approximate: if filePos>0 and curSec>0, we can estimate bitrate = filePos*8 / curSec
    // For now, if fileSize and filePos known, use filePos/fileSize for percent
  }

  uint32_t curMs = curSec*1000UL;
  uint32_t durMs = durSec*1000UL;

  // If curMs is 0 but filePos/fileSize known and durMs known, estimate curMs from percent
  if(fileSize>0 && filePos>0){
    uint32_t percentFromPos = (uint64_t)filePos*1000 / fileSize;
    if(curMs==0 && durMs>0){
      curMs = (uint64_t)percentFromPos * durMs / 1000;
    }else if(durMs==0 && curMs==0){
      // No time at all, estimate dur from fileSize and bitrate if possible
      // Use bitrate from player? We have config.station.bitrate may be 0, but we can estimate from filePos/cur
      // For now, leave durMs 0 but percent from pos
    }
  }

  // If durMs still 0, try to estimate from fileSize and bitrate
  if(durMs==0 && fileSize>0){
    // Try to get bitrate: if curSec>0 and filePos>0, bitrate = filePos*8 / curSec
    uint32_t estBitrate = 0;
    if(curSec>0 && filePos>0){
      estBitrate = (uint64_t)filePos*8 / curSec;
    }else{
      // Use last known bitrate from log? For MP3 we have 320k etc, try config
      // For fallback, use 128k
      estBitrate = 128000;
    }
    if(estBitrate>0){
      durMs = (uint64_t)fileSize*8*1000 / estBitrate;
    }
  }

  uint16_t percentX10 = 0;
  if(durMs>0 && curMs>0){
    percentX10 = (uint16_t)(curMs*1000 / durMs);
  }else if(fileSize>0 && filePos>0){
    percentX10 = (uint16_t)((uint64_t)filePos*1000 / fileSize);
    if(durMs>0 && curMs==0){
      curMs = (uint64_t)percentX10 * durMs / 1000;
    }
  }

  if(percentX10>1000) percentX10=1000;

  uint16_t idx = config.lastStation();
  uint16_t total = config.playlistLength();
  ProgressInfo p;
  p.curMs = curMs;
  p.durMs = durMs;
  p.state = 1;
  p.percentX10 = percentX10;
  p.fileIdx = idx;
  p.fileTotal = total;

  // Always send if we have at least filePos or curMs
  if(curMs>0 || filePos>0 || percentX10>0){
    omnia_progress_send(p);
  }
}
