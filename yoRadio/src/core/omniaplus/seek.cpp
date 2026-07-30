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
  uint32_t fileSize = player.getFileSize();
  uint32_t durMs = player.getAudioFileDuration()*1000UL;
  if(durMs==0){
    // Estimate dur from fileSize if possible, or use percent
    if(fileSize>0){
      // For MP3, use fileSize proportion
      uint32_t newPos = (fileSize>0 && durMs>0) ? (uint64_t)ms * fileSize / durMs : (fileSize * (ms % 100000) / 100000);
      // Actually if durMs==0, compute pos from ms assuming 128k bitrate as fallback or use percent
      // Use percent if ms is actually percent*?
      // For absolute ms when dur unknown, we use fileSize * ms / estimatedDur
      // Estimate dur from fileSize and bitrate 128k as fallback
      uint32_t estDur = fileSize*8*1000 / 128000;
      if(estDur==0) estDur = 180000; // 3min default
      newPos = (uint64_t)ms * fileSize / estDur;
      if(newPos>=fileSize) newPos=fileSize-1;
      player.setOutputPins(false);
      delay(30);
      bool ok = player.setFilePos(newPos);
      delay(30);
      player.setOutputPins(true);
      Serial.printf("##SEEK#: fallback setFilePos %lu (estDur %lu) ok=%d\n", (unsigned long)newPos, (unsigned long)estDur, ok);
      return;
    }
    return;
  }
  if(ms>durMs) ms=durMs;
  if(fileSize==0) return;
  uint32_t newPos = (uint64_t)ms * fileSize / durMs;
  if(newPos>=fileSize) newPos=fileSize-1;
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
  // If cur is 0, try filePos based
  if(cur==0){
    uint32_t filePos = player.getFilePos();
    uint32_t fileSize = player.getFileSize();
    uint32_t durMs = player.getAudioFileDuration()*1000UL;
    if(fileSize>0 && durMs>0){
      cur = (uint64_t)filePos * durMs / fileSize;
    }else if(fileSize>0){
      // estimate cur from filePos
      uint32_t estDur = fileSize*8*1000 / 128000;
      cur = (uint64_t)filePos * estDur / fileSize;
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
  Serial.printf("##SEEK#: start %s (hold acceleration 5s/10s/30s/60s)\n", forward?"+":"-");
}

void omnia_seek_stop(){
  seekingActive=false;
  Serial.println("##SEEK#: stop");
}
