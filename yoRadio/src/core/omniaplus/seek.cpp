#include "seek.h"
#include "../player.h"

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
}

void omnia_seek_relative(int32_t deltaMs){
  Serial.printf("##SEEK#: relative %+ld ms\n", (long)deltaMs);
}

void omnia_seek_percent(uint16_t permille){
  Serial.printf("##SEEK#: percent %u/1000\n", permille);
}

void omnia_seek_start(bool forward){
  seekingActive=true; seekDirForward=forward; seekHoldStartMs=millis();
  Serial.printf("##SEEK#: start %s\n", forward?"+":"-");
}

void omnia_seek_stop(){
  seekingActive=false;
  Serial.println("##SEEK#: stop");
}
