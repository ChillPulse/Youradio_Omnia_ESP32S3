#include "cli_extensions.h"
#include "seek.h"
#include "shuffle_fix.h"
#include "usb_host.h"
#include "progress.h"
#include <string.h>

bool omnia_cli_handle(const char* line){
  if(!line) return false;
  if(seek_handle(line)) return true; // seek, seek_rel, seek_percent, seek_start +/-, seek_stop — из seek.cpp (alias)
  // На самом деле seek_handle в seek.h — omnia_seek_handle
  if(omnia_seek_handle(line)) return true;
  if(strncmp(line,"shuffle ",8)==0){
    if(strstr(line,"on")) omnia_shuffle_set(SHUFFLE_ON);
    else omnia_shuffle_set(SHUFFLE_OFF);
    Serial.printf("##SHUFFLE#: %s\n", line);
    return true;
  }
  if(strncmp(line,"repeat ",7)==0){
    if(strstr(line,"one")) omnia_shuffle_set_repeat(REPEAT_ONE);
    else if(strstr(line,"all")) omnia_shuffle_set_repeat(REPEAT_ALL);
    else omnia_shuffle_set_repeat(REPEAT_OFF);
    Serial.printf("##REPEAT#: %s\n", line);
    return true;
  }
  if(strcmp(line,"usb_scan")==0){ omnia_usb_scan(); return true; }
  if(strncmp(line,"usb_list",8)==0){ omnia_usb_list("/"); return true; }
  if(strncmp(line,"usb_play",8)==0){ omnia_usb_play(line+9); return true; }
  if(strcmp(line,"status")==0){
    Serial.println("##STATUS#: WEB/SD/USB-Flash/BT/USB Audio 5 FINAL, Shuffle/Repeat, PROGRESS/SEEK/USB");
    return true;
  }
  if(strncmp(line,"ping",4)==0){
    Serial.printf("##PONG#: %s\n", line);
    return true;
  }
  return false;
}
