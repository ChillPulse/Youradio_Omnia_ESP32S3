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
  Serial.printf("##SEEK#: absolute %lu ms\n", (unsigned long)ms);
  if(!player.isRunning()) return;
  uint32_t durMs = player.getAudioFileDuration()*1000UL;
  if(durMs==0){
    // Fallback: use file size
    uint32_t fsize = player.getFileSize();
    uint32_t pos = player.getFilePos();
    // Can't compute, just try setFilePos by percent if ms < 1000 treat as percent?
    return;
  }
  if(ms>durMs) ms=durMs;
  uint32_t fileSize = player.getFileSize();
  if(fileSize==0) return;
  // Approximate filePos = ms/dur * fileSize
  uint32_t newPos = (uint64_t)ms * fileSize / durMs;
  // Soft-mute to avoid click
  player.setOutputPins(false);
  delay(50);
  bool ok = player.setFilePos(newPos);
  delay(30);
  player.setOutputPins(true);
  Serial.printf("##SEEK#: setFilePos %lu (fileSize %lu dur %lu) ok=%d\n", (unsigned long)newPos, (unsigned long)fileSize, (unsigned long)durMs, ok);
}

void omnia_seek_relative(int32_t deltaMs){
  Serial.printf("##SEEK#: relative %+ld ms\n", (long)deltaMs);
  uint32_t cur = player.getAudioCurrentTime()*1000UL;
  int32_t newMs = (int32_t)cur + deltaMs;
  if(newMs<0) newMs=0;
  omnia_seek_absolute((uint32_t)newMs);
}

void omnia_seek_percent(uint16_t permille){
  Serial.printf("##SEEK#: percent %u/1000\n", permille);
  uint32_t durMs = player.getAudioFileDuration()*1000UL;
  if(durMs==0) return;
  uint32_t ms = (uint64_t)permille * durMs / 1000;
  omnia_seek_absolute(ms);
}

void omnia_seek_start(bool forward){
  seekingActive=true; seekDirForward=forward; seekHoldStartMs=millis();
  Serial.printf("##SEEK#: start %s\n", forward?"+":"-");
}

void omnia_seek_stop(){
  seekingActive=false;
  Serial.println("##SEEK#: stop");
}
