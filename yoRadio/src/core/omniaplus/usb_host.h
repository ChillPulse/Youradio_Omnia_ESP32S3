#ifndef OMNIA_USB_HOST_H
#define OMNIA_USB_HOST_H
#include <Arduino.h>
// USB-Flash Host MSC for ESP32-S3 on GPIO19/20 — flagship, copy of SD mode
// 5V 1A+ LDO + 100nF+10uF VBUS + TVS + polyfuse 0.75A
// For testing: UART ##USB.STATUS# + Web UI list + ear, STM32 UART optional

void omnia_usb_host_init();
void omnia_usb_host_loop();
bool omnia_usb_is_mounted();
void omnia_usb_list(const char* path);
bool omnia_usb_play(const char* pathOrIndex);
void omnia_usb_scan();

#endif
