#ifndef OMNIA_USB_HOST_H
#define OMNIA_USB_HOST_H
#include <Arduino.h>
// USB-Flash Host MSC для ESP32-S3 на GPIO19/20 — v8.2
// 5V 1A+ LDO + 100nF+10uF VBUS + TVS опц + polyfuse 0.75A 6V
// Для теста: UART ESP32-S3 ##USB.STATUS# + Web UI list + на слух, STM32 UART не обязателен

void omnia_usb_host_init();
void omnia_usb_host_loop(); // вызывать из main loop
bool omnia_usb_is_mounted();
void omnia_usb_list(const char* path);
bool omnia_usb_play(const char* pathOrIndex);
void omnia_usb_scan();

// Проверка после каждого подэтапа: определяется, монтируется, листается, играет без дропов, seek, выдергивание → stop без краша, rescan
#endif
