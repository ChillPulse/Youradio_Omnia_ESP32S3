# YoRadio Morernization v8.2 — Quick Start for OMNIA (5 sources FINAL)

Источники FINAL 5: WEB / SD / USB-Flash (Host MSC GPIO19/20) / BT401 Bluetooth / BT401 USB Audio PC/Android. TF/U-disk отключены.

## Pinout FINAL (твое V7G + USB)
- ESP17→PB4 DATA, PB3→16 BCLK, PA15→18 LRCK, PB5→DAC DIN
- GPIO2→PA8 MUTE HIGH, GPIO4→PA9 RATE LOW44.1 HIGH48
- GPIO19 D- / GPIO20 D+ USB Host, 5V 1A+ LDO, 100nF+10uF VBUS, polyfuse 0.75A 6V
- GND общий

## UART
Старый CLI 115200: mode 0/1/2 prev/next/toggle/stop/start/play/vol/list/info...
Новые: seek <ms>, seek_rel +/-, seek_percent 0-1000, sdpos, seek_start +/-, seek_stop, shuffle on/off, repeat off/one/all, shuffle_repeat, random, usb_scan/list/play, mode usb, status, ping

Исходящие: ##PROGRESS# cur_ms dur_ms state percent_x10 idx total 2-5Hz до 10Hz seek
##META# title=artist=album=year=bitrate=fmt=sr=ch=
##TRACK# ##SD/USB.STATUS#

Поведение кнопок: Short NEXT/PREV трек, Hold NEXT/PREV seek внутри с ускорением 2x→4x→8x→16x

## Этапы
USB-0 hardware, USB-1 Host VID/PID, USB-2 Mount list root, USB-3 Play one file, USB-4 Full playlist+seek, USB-5 Hot-plug+Web UI

Проверка каждого: mount/list/play без дропов, seek, выдергивание → stop без краша, rescan.

## Интеграция OMNIA
AT32 I2S MASTER RADIO domain, BT401 MASTER BT401 domain, S2 AURA только рендер, Primary control via yoRadio App↔WiFi↔S3→UART→AT32→UART→S2

См. полную доку docs/OMNIA_SPEC/MASTER_SPEC_v8.2_CLEAN_FINAL.md
