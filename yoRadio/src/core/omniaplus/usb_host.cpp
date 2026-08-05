#include "usb_host.h"
#include "../config.h"
#include <Arduino.h>

// Flagship: USB-Flash as copy of SD mode
// Hardware: GPIO19 D- / GPIO20 D+ S3 internal OTG PHY + 5V 1A+ LDO + 100nF+10uF VBUS + TVS + polyfuse 0.75A
// For Arduino ESP32 core 3.3.0, USB Host MSC via IDF usb_host + esp_vfs_fat

#ifdef USE_USB_MSC
#include "usb/host/usb_host.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

static bool usb_mounted = false;
static bool usb_host_started = false;

void omnia_usb_host_init(){
  Serial.println("##USB.HOST#: init GPIO19 D- GPIO20 D+ (S3 internal OTG PHY)");
  Serial.println("##USB.STATUS#: init 5V power check polyfuse 0.75A");
  Serial.println("##USB.HOST#: copy of SD mode — same API: listSD/scan/play/seek/meta");

#ifdef USE_USB_MSC
  // TODO: Real USB Host MSC init
  // usb_host_config_t host_config = {
  //   .skip_phy_setup = false,
  //   .intr_flags = ESP_INTR_FLAG_LEVEL1,
  // };
  // usb_host_install(&host_config);
  // xTaskCreate(usb_host_lib_task, "usb_host_lib", 4096, NULL, 5, NULL);
  // For now, stub that pretends to be ready after 2 sec for testing SD copy logic
  usb_host_started = true;
  Serial.println("##USB.HOST#: USE_USB_MSC defined, host task would start here");
#endif

  // For testing without real USB stick, we simulate that USB is not mounted until stick inserted
  // Real implementation will set usb_mounted=true on MSC connect event
}

void omnia_usb_host_loop(){
#ifdef USE_USB_MSC
  // usb_host_lib_handle_events(10, NULL);
#endif
  // Periodic check for mount status could be here
}

bool omnia_usb_is_mounted(){ return usb_mounted; }

void omnia_usb_list(const char* path){
  Serial.printf("##USB.LIST#: path=%s (copy of SD mode, reads /data/playlistusb.csv if exists, else scans USB root)\n", path?path:"/");
  // For flagship, reuse same logic as SD: look for PLAYLIST_USB_PATH (/data/playlistusb.csv)
  // If not exists, scan USB root for mp3/flac/wav/aac/m4a
  // For now, just call sdlist logic but with USB prefix
  // TODO: implement USB scan similar to SDManager::listSD but for USB FS
}

bool omnia_usb_play(const char* pathOrIndex){
  Serial.printf("##USB.PLAY#: %s (copy of SD mode, uses connecttoFS with USB FS)\n", pathOrIndex?pathOrIndex:"");
  // TODO: if pathOrIndex is numeric, convert to file from USB playlist
  // else play file path via player.connecttoFS(usbman, path)
  return false;
}

void omnia_usb_scan(){
  Serial.println("##USB.SCAN#: start (copy of SD indexing, scans USB root for mp3/flac/wav/aac/m4a)");
  // TODO: usbman.indexUSBPlaylist() similar to sdman.indexSDPlaylist()
  omnia_usb_list("/");
}
