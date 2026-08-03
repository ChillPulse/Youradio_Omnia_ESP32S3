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
  if(now - lastProgressMs < 400) return; // 2.5Hz max to avoid spam
  lastProgressMs = now;
  if(!player.isRunning()) return;
  if(config.getMode()!=1) return;

  uint32_t curSec = player.getAudioCurrentTime();
  uint32_t durSec = player.getAudioFileDuration();
  uint32_t filePos = player.getFilePos();
  uint32_t fileSize = player.getFileSize();

  // Get bitrate from audio info if available, otherwise estimate
  // For MP3, bitrate can be obtained from config or from file header, but we approximate
  uint32_t bitrate = 0;
  // Try to get from last known bitrate via station info? For SD, we have no direct API, use 128k fallback if needed
  // We can use fileSize and durSec to estimate if durSec>0: bitrate = fileSize*8/durSec

  uint32_t curMs = curSec*1000UL;
  uint32_t durMs = durSec*1000UL;

  // If durMs is 0 but fileSize known, estimate dur from fileSize and bitrate
  // For MP3 with ID3, fileSize includes ID3, but close enough
  if(durMs==0 && fileSize>0){
    // Try to estimate bitrate from filePos/curSec if curSec>0
    if(curSec>0 && filePos>0){
      bitrate = (uint64_t)filePos*8 / curSec;
    }
    // If still 0, try to use fileSize and assume average bitrate from file header if available?
    // Fallback to 160k for MP3, 900k for FLAC, 1411k for WAV estimation
    // For now, if bitrate still 0, estimate dur from fileSize assuming 192k MP3 average
    if(bitrate==0){
      // Guess based on file extension? For MP3 assume 192k, for FLAC assume 800k, for WAV 1411k
      // We don't have extension here, so use 192k as generic for MP3
      bitrate = 192000;
    }
    if(bitrate>0){
      durMs = (uint64_t)fileSize*8*1000 / bitrate;
    }
  }

  // Percent from filePos/fileSize if available (more stable than time for MP3 VBR)
  uint16_t percentX10 = 0;
  if(fileSize>0 && filePos>0){
    percentX10 = (uint64_t)filePos*1000 / fileSize;
    // If curMs is 0 but we have percent and dur, estimate curMs
    if(curMs==0 && durMs>0){
      curMs = (uint64_t)percentX10 * durMs / 1000;
    }
    // If durMs still 0 but curMs>0 and percent>0, estimate durMs
    if(durMs==0 && curMs>0 && percentX10>0){
      durMs = (uint64_t)curMs * 1000 / percentX10;
    }
  }else if(durMs>0 && curMs>0){
    percentX10 = (uint64_t)curMs*1000 / durMs;
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

  if(percentX10>0 || curMs>0){
    omnia_progress_send(p);
  }
}
