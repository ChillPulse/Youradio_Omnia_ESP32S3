#include "usb_host.h"
#include <Arduino.h>

void omnia_usb_host_init(){
  Serial.println("##USB.HOST#: init GPIO19 D- GPIO20 D+ (S3 internal OTG PHY)");
  Serial.println("##USB.STATUS#: init 5V power check polyfuse 0.75A");
}

void omnia_usb_host_loop(){}

bool omnia_usb_is_mounted(){ return false; }

void omnia_usb_list(const char* path){
  Serial.printf("##USB.LIST#: path=%s (TODO scan FAT)\n", path?path:"/");
}

bool omnia_usb_play(const char* pathOrIndex){
  Serial.printf("##USB.PLAY#: %s\n", pathOrIndex?pathOrIndex:"");
  return false;
}

void omnia_usb_scan(){
  Serial.println("##USB.SCAN#: start");
  omnia_usb_list("/");
}
