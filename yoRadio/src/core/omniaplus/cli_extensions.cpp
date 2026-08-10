#include "cli_extensions.h"
#include "seek.h"
#include "shuffle_fix.h"
#include "usb_host.h"
#include "progress.h"
#include "../config.h"
#include "../player.h"
#include <string.h>
#include <Arduino.h>

// Helper to print current active functions
static void print_omnia_status(uint8_t clientId = 100){
  // clientId 100 = Serial only, otherwise also websocket
  const char* modeStr = config.getMode()==1 ? "SD" : (config.getMode()==0 ? "WEB" : "UNKNOWN");
  const char* shuffleStr = config.store.sdsnuffle ? "ON" : "OFF";
  // repeat mode stored in shuffle_fix
  extern PlaylistState* omnia_shuffle_get_state(); // we'll not use, just use global
  // For simplicity, get from config? We'll store repeat in shuffle engine, but also show
  // Use omnia function to get repeat
  extern RepeatMode omnia_shuffle_get_repeat();
  const char* repeatStr = "ALL";
  switch(omnia_shuffle_get_repeat()){
    case REPEAT_OFF: repeatStr="OFF"; break;
    case REPEAT_ONE: repeatStr="ONE"; break;
    case REPEAT_ALL: repeatStr="ALL"; break;
  }
  uint16_t total = config.playlistLength();
  uint16_t cur = config.lastStation();
  Serial.printf("##OMNIA.STATUS#: mode=%s shuffle=%s repeat=%s total=%u cur=%u\n", modeStr, shuffleStr, repeatStr, total, cur);
  Serial.printf("##SD.STATUS#: shuffle=%s repeat=%s\n", shuffleStr, repeatStr);
  // Also send via telnet if clientId != 100
  // telnet is global
  extern class Telnet telnet;
  // We can't easily get telnet instance here, so just Serial for now, telnet will also get via printf in on_input
}

bool omnia_cli_handle(const char* line){
  if(!line) return false;
  // Trim
  String s = String(line);
  s.trim();
  const char* cmd = s.c_str();

  // Seek
  if(omnia_seek_handle(cmd)) return true;

  // Shuffle ON/OFF/TOGGLE/STATUS
  if(strncmp(cmd,"shuffle",7)==0){
    if(strstr(cmd,"on")){
      config.setSnuffle(true);
      Serial.println("##SHUFFLE#: ON (sdsnuffle=1) — next track will be shuffled, current preserved");
      return true;
    }
    if(strstr(cmd,"off")){
      config.setSnuffle(false);
      Serial.println("##SHUFFLE#: OFF (sdsnuffle=0) — order normal");
      return true;
    }
    if(strstr(cmd,"status") || strlen(cmd)==7){
      const char* st = config.store.sdsnuffle ? "ON" : "OFF";
      Serial.printf("##SHUFFLE#: %s (sdsnuffle=%d) — %s\n", st, config.store.sdsnuffle, st[0]=='O' && st[1]=='N' ? "random without repeats" : "normal order");
      return true;
    }
  }

  // Repeat
  if(strncmp(cmd,"repeat",6)==0){
    if(strstr(cmd,"off")){
      config.setSdRepeat(0);
      Serial.println("##REPEAT#: OFF — stop at end of list");
      return true;
    }
    if(strstr(cmd,"one")){
      config.setSdRepeat(1);
      Serial.println("##REPEAT#: ONE — repeat current track");
      return true;
    }
    if(strstr(cmd,"all")){
      config.setSdRepeat(2);
      Serial.println("##REPEAT#: ALL — loop playlist");
      return true;
    }
    // status
    Serial.print("##REPEAT#: ");
    switch(omnia_shuffle_get_repeat()){
      case REPEAT_OFF: Serial.println("OFF"); break;
      case REPEAT_ONE: Serial.println("ONE"); break;
      case REPEAT_ALL: Serial.println("ALL"); break;
    }
    return true;
  }

  if(strcmp(cmd,"usb_scan")==0){ omnia_usb_scan(); return true; }
  if(strncmp(cmd,"usb_list",8)==0){ omnia_usb_list("/"); return true; }
  if(strncmp(cmd,"usb_play",8)==0){ omnia_usb_play(cmd+9); return true; }

  // Omnia status — shows active functions, not WiFi status
  if(strcmp(cmd,"omnia_status")==0 || strcmp(cmd,"mstatus")==0 || strcmp(cmd,"shuffle_status")==0 || strcmp(cmd,"ostatus")==0){
    print_omnia_status();
    // Also print progress if available
    Serial.printf("##PROGRESS#: cur/dur via WebUI sdpos/sdtpos — open WebUI for bar\n");
    return true;
  }

  // Legacy status is wifi status — keep it, but omnia_status is new
  if(strcmp(cmd,"status")==0){
    // Let original wifi status handle it, but also show omnia status after
    // Return false to let original handler print wifi status, then we also print omnia status
    // To avoid double, we print omnia status here and return false so original also prints
    // Actually we want both, so handle here and return false to let telnet also print wifi
    // For simplicity, return false and let telnet print wifi, but also print omnia in next call? We'll just handle wifi elsewhere
    // Here we intercept and print both
    print_omnia_status();
    return false; // let original wifi status also print
  }

  if(strcmp(cmd,"ping")==0 || strncmp(cmd,"ping ",5)==0){
    Serial.printf("##PONG#: %s\n", cmd);
    return true;
  }

  return false;
}
