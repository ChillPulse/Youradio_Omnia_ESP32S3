#include "seek.h"
#include "../player.h"
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
  // For M4A, disable byte-seek as it's unstable for MP4 container (as per Chat 5 recommendation)
  // Check codec
  uint8_t codec = player.getCodec();
  const uint8_t CODEC_M4A = 7; // from AudioEx.h, approximate — actual value may differ, we check via getCodec() == 5? We'll just check file extension fallback
  // Simple check: if current file is .m4a, disable seek
  // For flagship, we disable seek for M4A to avoid next track jump
  // We can detect via file name in config.station.url
  String path = config.station.url;
  path.toLowerCase();
  if(path.endsWith(".m4a")){
    Serial.println("##SEEK#: M4A seek disabled (byte-seek unstable for MP4 container) — use MP3/FLAC/WAV for seek");
    return;
  }
  Serial.printf("##SEEK#: absolute %lu ms\n", (unsigned long)ms);
  if(!player.isRunning()) return;
  uint32_t fileSize = player.getFileSize();
  uint32_t durMs = player.getAudioFileDuration()*1000UL;
  if(durMs==0){
    uint32_t bitrate = player.getBitRate();
    if(bitrate==0) bitrate=128000;
    if(fileSize>0) durMs = (uint64_t)fileSize*8*1000 / bitrate;
  }
  if(durMs==0 || fileSize==0) return;
  if(ms>durMs) ms=durMs;
  uint32_t newPos = (uint64_t)ms * fileSize / durMs;
  if(newPos>=fileSize) newPos=fileSize-1;
  player.setOutputPins(false);
  delay(50);
  bool ok = player.setFilePos(newPos);
  delay(50);
  player.setOutputPins(true);
  Serial.printf("##SEEK#: setFilePos %lu ok=%d\n", (unsigned long)newPos, ok);
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
  if(fileSize==0) return;
  // Disable for M4A as well
  String path = config.station.url;
  path.toLowerCase();
  if(path.endsWith(".m4a")){
    Serial.println("##SEEK#: M4A seek disabled");
    return;
  }
  uint32_t newPos = (uint64_t)permille * fileSize / 1000;
  if(newPos>=fileSize) newPos=fileSize-1;
  player.setOutputPins(false);
  delay(30);
  bool ok = player.setFilePos(newPos);
  delay(30);
  player.setOutputPins(true);
  Serial.printf("##SEEK#: percent setFilePos %lu ok=%d\n", (unsigned long)newPos, ok);
}

void omnia_seek_start(bool forward){
  seekingActive=true; seekDirForward=forward; seekHoldStartMs=millis();
  Serial.printf("##SEEK#: start %s\n", forward?"+":"-");
}

void omnia_seek_stop(){
  seekingActive=false;
  Serial.println("##SEEK#: stop");
}
