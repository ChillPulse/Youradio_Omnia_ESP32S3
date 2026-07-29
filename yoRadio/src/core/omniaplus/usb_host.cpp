#include "usb_host.h"
// ЗАГЛУШКА с TODO для ESP32-S3 USB Host MSC
// В Arduino ESP32 core 3.x: #include "USB.h" + "USBMSC.h" или ESP-IDF usb_host

// Для ESP-IDF путь:
// #include "usb/host/usb_host.h"
// #include "esp_vfs_fat.h"
// usb_host_install, usb_host_lib_handle_events, esp_vfs_fat_usb_host_mount etc.

void omnia_usb_host_init(){
  Serial.println("##USB.HOST#: init GPIO19 D- GPIO20 D+ (S3 internal OTG PHY)");
  Serial.println("##USB.STATUS#: init 5V power check polyfuse 0.75A");
  // TODO:
  // - USBHost.begin()
  // - Register callback connect/disconnect → omnia_usb_status_send("mounted"...)

  // Пример для Arduino:
  // USB.onEvent([](arduino_usb_event_data_t *data){ ... });
  // USBH_MSC msc;
  // USBH.begin()
}

void omnia_usb_host_loop(){
  // TODO: handle USB host events, check mount
}

bool omnia_usb_is_mounted(){ return false; /* TODO */ }

void omnia_usb_list(const char* path){
  Serial.printf("##USB.LIST#: path=%s (TODO scan FAT)\n", path?path:"/");
  // TODO: list files filter .mp3 .flac .aac .m4a .wav .ogg .opus
}

bool omnia_usb_play(const char* pathOrIndex){
  Serial.printf("##USB.PLAY#: %s\n", pathOrIndex?pathOrIndex:"");
  // TODO: connecttoFS with USB FS handle like SD
  return false;
}

void omnia_usb_scan(){
  Serial.println("##USB.SCAN#: start");
  omnia_usb_list("/");
}
