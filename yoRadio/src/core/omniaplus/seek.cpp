#include "seek.h"
#include "../player.h"
#include "../../audioI2S/AudioEx.h"
#include "../config.h"
#include <Arduino.h>

static bool seekingActive = false;
static bool seekDirForward = true;
static uint32_t seekHoldStartMs = 0;

void omnia_seek_init(){ seekingActive=false; }

int32_t omnia_seek_accelerated_step(uint32_t holdMs){
  if(holdMs < 2000) return 5000;
  if(holdMs < 4000) return 10000;
  if(holdMs < 7000) return 30000;
  return 60000;
}

bool omnia_seek_handle(const char* cmd){
  if(!cmd) return false;
  if(strncmp(cmd,"seek ",5)==0){
    uint32_t ms = atol(cmd+5);
    omnia_seek_absolute(ms);
    return true;
  }
  if(strncmp(cmd,"seek_rel ",9)==0){
    int32_t d = atol(cmd+9);
    omnia_seek_relative(d);
    return true;
  }
  if(strncmp(cmd,"seek_percent ",13)==0){
    uint16_t p = atoi(cmd+13);
    omnia_seek_percent(p);
    return true;
  }
  if(strcmp(cmd,"seek_start +")==0){ omnia_seek_start(true); return true; }
  if(strcmp(cmd,"seek_start -")==0){ omnia_seek_start(false); return true; }
  if(strcmp(cmd,"seek_stop")==0){ omnia_seek_stop(); return true; }
  return false;
}

void omnia_seek_absolute(uint32_t ms){
  if(!player.isRunning()) return;
  uint32_t durMs = player.getAudioFileDuration()*1000UL;
  if(durMs==0){
    // Fallback from fileSize
    uint32_t fileSize = player.getFileSize();
    uint32_t bitrate = player.getBitRate();
    if(bitrate==0) bitrate=128000;
    if(fileSize>0) durMs = (uint64_t)fileSize*8*1000 / bitrate;
  }
  if(durMs==0) return;
  if(ms>durMs) ms=durMs;

  // For M4A, use time->sample->offset via stsz index if available (flagship)
  // FIX: Audio::CODEC_M4A is private in AudioEx.h, use numeric value 4 (M4A=4, OPUS=7)
  // Original bug was getCodec()==7 (OPUS) instead of 4 (M4A) -> M4A seek never called
  if(player.getCodec() == 4){ // CODEC_M4A = 4
    if(player.omnia_m4aSeekMs(ms)){
      Serial.printf("##SEEK#: M4A time seek %lu ms via stsz index\n", (unsigned long)ms);
      return;
    }
    // If index not ready, fall back to byte proportional within audio range
  }

  // Chat16 Step4: AAC ADTS flagship seek via index
  if(player.getCodec() == 3){ // CODEC_AAC = 3
    if(player.omnia_aacSeekMs(ms)){
      Serial.printf("##SEEK#: AAC time seek %lu ms via ADTS index\n", (unsigned long)ms);
      return;
    }
  }

  uint32_t start = player.sd_min;
  uint32_t end = player.sd_max;
  if(end<=start){
    start = 0;
    end = player.getFileSize();
  }
  if(end<=start) return;
  uint32_t pos = start + (uint64_t)ms * (end - start) / durMs;
  Serial.printf("##SEEK#: absolute %lu ms -> pos %lu (range %lu-%lu dur %lu)\n", (unsigned long)ms, (unsigned long)pos, (unsigned long)start, (unsigned long)end, (unsigned long)durMs);
  player.setOutputPins(false);
  delay(30);
  bool ok = player.setFilePos(pos);
  delay(30);
  player.setOutputPins(true);
  Serial.printf("##SEEK#: setFilePos %lu ok=%d\n", (unsigned long)pos, ok);
}

void omnia_seek_relative(int32_t deltaMs){
  Serial.printf("##SEEK#: relative %+ld ms\n", (long)deltaMs);
  uint32_t cur = player.getAudioCurrentTime()*1000UL;
  if(cur==0){
    uint32_t filePos = player.getFilePos();
    uint32_t fileSize = player.getFileSize();
    uint32_t durMs = player.getAudioFileDuration()*1000UL;
    if(fileSize>0 && durMs>0){
      cur = (uint64_t)filePos * durMs / fileSize;
    }
  }
  int32_t newMs = (int32_t)cur + deltaMs;
  if(newMs<0) newMs=0;
  omnia_seek_absolute((uint32_t)newMs);
}

void omnia_seek_percent(uint16_t permille){
  Serial.printf("##SEEK#: percent %u/1000\n", permille);
  uint32_t fileSize = player.getFileSize();
  uint32_t start = player.sd_min;
  uint32_t end = player.sd_max;
  if(end<=start){
    start = 0;
    end = fileSize;
  }
  if(end<=start) return;
  String path = config.station.url;
  path.toLowerCase();
  if(path.endsWith(".m4a")){
    // For M4A, convert percent to ms then use m4aSeek
    uint32_t durMs = player.getAudioFileDuration()*1000UL;
    if(durMs==0 && fileSize>0){
      uint32_t br = player.getBitRate(); if(br==0) br=128000;
      durMs = (uint64_t)fileSize*8*1000 / br;
    }
    uint32_t ms = (uint64_t)permille * durMs / 1000;
    omnia_seek_absolute(ms);
    return;
  }
  uint32_t newPos = start + (uint64_t)permille * (end - start) / 1000;
  if(newPos>=end) newPos=end-1;
  player.setOutputPins(false);
  delay(30);
  bool ok = player.setFilePos(newPos);
  delay(30);
  player.setOutputPins(true);
  Serial.printf("##SEEK#: percent setFilePos %lu ok=%d\n", (unsigned long)newPos, ok);
}

void omnia_seek_start(bool forward){
  seekingActive=true; seekDirForward=forward; seekHoldStartMs=millis();
  Serial.printf("##SEEK#: start %s (accel 5s/10s/30s/60s)\n", forward?"+":"-");
}

void omnia_seek_stop(){
  seekingActive=false;
  Serial.println("##SEEK#: stop");
}
