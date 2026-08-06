# Chat Recommendation 9 — Тройная проверка FINAL
Дата: 2026-08-06
Репо: Youradio_Omnia_ESP32S3 main 3f2f80f
Источник: https://github.com/ChillPulse/Info-spravka/blob/main/Chat%20reomedation%209

## Что было сломано (корень STOP при клике по шкале)

WebUI (и telnet `sdpos`) шлёт позицию как **абсолютный байт файла** (например 2 000 000), а иногда как относительный (0..len).
Старый код делал `val - player.sd_min`. Если WebUI уже прислал абсолют, вычитание даёт `abs - sd_min = относительный`, но потом `setFilePos` и `setResumeFilePos` ожидают **абсолют**. Итог:
- `pos` становится слишком маленьким или уходит в `sdmax-1` => underflow/клэмп в конец файла
- `setFilePos(endPos-1)` => декодер AAC/M4A/WAV не находит sync, `audio_eof` сразу, STOP
- Для `player.cpp _play` та же ошибка: `sdResumePos - sd_min` при resume после URL/переключения приводит к старту с конца прошлого трека → мгновенное закрытие (логи Log14 для AAC).

Поэтому клик по шкале → прыжок в неверную позицию → EOF → STOP на AAC/M4A/WAV.

## 4 пункта из Chat 9 — проверка

### 1) Config::setSDpos() — главный убийца seek

**Было:** `setResumeFilePos(val - sd_min)` и `setFilePos(val - sd_min)`

**Стало (config.cpp:391-435):**
```cpp
static uint32_t clampAbsSdPos(uint32_t val){
  uint32_t sdmin = player.sd_min;
  uint32_t sdmax = player.sd_max;
  if(sdmax == 0 || sdmax <= sdmin){
    sdmin = 0;
    sdmax = player.getFileSize();
  }
  uint32_t absPos = val;
  if(!(val >= sdmin && val <= sdmax)){
    uint32_t len = (sdmax > sdmin) ? (sdmax - sdmin) : 0;
    if(len && val <= len) absPos = sdmin + val;
  }
  if(absPos < sdmin) absPos = sdmin;
  if(sdmax && absPos >= sdmax) absPos = sdmax - 1;
  return absPos;
}
void Config::setSDpos(uint32_t val){
  if (getMode()==PM_SDCARD){
    sdResumePos = 0;
    uint32_t absPos = clampAbsSdPos(val);
    if(!player.isRunning()){
      player.setResumeFilePos(absPos);
      player.sendCommand({PR_PLAY, config.store.lastSdStation});
    }else{
      player.setOutputPins(false); delay(30);
      player.setFilePos(absPos); delay(30);
      player.setOutputPins(true);
    }
  }
}
```
Робастный авто-детектор: если val уже в [sdmin..sdmax] — считаем абсолютным, иначе если val <= len — считаем относительным и делаем sdmin+val. Клэмп `sdmax-1` чтобы не попасть точно в EOF.

Проверка grep: `val - player.sd_min` в config.cpp отсутствует.

### 2) audio_eof_stream() + onAudioEofCommon()

**Было в audiohandlers.h:** `config.sdResumePos==0?0:config.sdResumePos-player.sd_min`

**Стало:**
```cpp
void audio_eof_stream(...){
  player.sendCommand({PR_STOP,0});
  if(!player.resumeAfterUrl) return;
  if(config.getMode()==PM_WEB) player.sendCommand({PR_PLAY, config.lastStation()});
  else{
    player.setResumeFilePos(config.sdResumePos); // без minus sd_min
    player.sendCommand({PR_PLAY, config.lastStation()});
  }
}
static void onAudioEofCommon(...){
    g_lastEofMs = millis(); g_eofHandled = true;
    config.sdResumePos = 0; // сброс, чтобы след трек не стартовал с конца прошлого (Log14)
    ...
}
```
Теперь resume коректен, нет прыжка в конец.

Дополнительно в **player.cpp:296** (скрытый 5-й пункт) было то же:
`connecttoFS(..., config.sdResumePos==0?_resumeFilePos:config.sdResumePos-player.sd_min)`
Исправлено на `config.sdResumePos` без вычитания. Это именно та строка, которая ломала resume после STOP/URL для SD.

### 3) seek.cpp CODEC_M4A проверка

**Было:** `if(player.getCodec() == 7) // CODEC_M4A approx`

**Стало:** `if(player.getCodec() == Audio::CODEC_M4A) // FIX: was 7 which is CODEC_OPUS=7, CODEC_M4A=4`

В AudioEx.h:
```
CODEC_NONE 0, WAV 1, MP3 2, AAC 3, M4A 4, FLAC 5, AACP 6, OPUS 7, OGG 8, VORBIS 9
```
Старый код никогда не вызывал `omnia_m4aSeekMs` для M4A. Теперь вызывает.

**Результат:** time-seek по stsz таблице (если индекс готов) или fallback byte-пропорционально внутри sd_min..sd_max.

### 4) SDLEN fallback в netserver.cpp

**Было:** только `if(sdmax==0)`

**Стало:**
```cpp
uint32_t sdmin = player.sd_min;
uint32_t sdmax = player.sd_max;
if(sdmax==0 || sdmax<=sdmin){
  sdmax = player.getFileSize();
  sdmin = 0;
}
sprintf(wsbuf,"{\"sdmin\": %d,\"sdmax\": %d}", sdmin, sdmax);
```
Теперь даже если гонка/ошибка и `sdmax <= sdmin`, UI не выключает шкалу/seek. Прогресс бар остаётся активным.

Также SDPOS (строки 324-342) уже содержит fallback по bitrate 128k если `sdtend==0`.

## Тройная проверка (run)

1. `grep -R "sd_min" yoRadio/src/core/config.cpp yoRadio/src/core/audiohandlers.h yoRadio/src/core/player.cpp` — показывает только корректные использования (хранение sdmin/sdmax, clamp), нет `val - sd_min` кроме комментариев `// FIX: was ...`
2. `grep -R "sdResumePos -"` — пусто, все вычитания удалены
3. `grep -R "CODEC_M4A\|== 7" seek.cpp` — только `Audio::CODEC_M4A`
4. `grep -R "sdmax==0"` netserver.cpp — содержит `|| sdmax<=sdmin`
5. Собранный бинарь? Проверяется прошивкой.

## Как проверить после прошивки (для чайника, на пальцах)

Ты можешь проверить только на слух + родной WebUI + UART ESP32-S3 115200. Этого достаточно.

1. **Прошей** ESP32-S3 (Arduino IDE: Board ESP32S3 Dev Module, PSRAM OPI, 16MB, Partition 8M with spiffs, 240MHz, CDC+Hardware, Flash 921600). Скопируй папку `yoRadio` в `C:\Youradio_Omnia_ESP32S3\yoRadio`, открой `yoRadio.ino`, нажми Upload.

2. **Открой UART** 115200 (Serial Monitor). При загрузке увидишь:
```
##[BOOT]# player.init done
SD Mounted
...
Ready in SD Mode!
##PROGRESS#: ...
```
Free Heap ~ 100-177k — норма.

3. **WebUI**: открой `http://<ip esp>/` . Вкладка SD. Убедись шкала прогресса не чёрная, а двигается. В консоли браузера WebSocket сообщения: `{"sdpos":..., "sdend":..., "sdtpos":..., "sdtend":...}` и `{"sdmin":0,"sdmax":16744789}` — `sdmax` не 0.

4. **Тест AAC** (самый ломкий):
   - Запусти `sample3.aac` или `sample4.aac`
   - В WebUI кликни в середину шкалы → в UART `##SEEK#: absolute X ms -> pos Y (range ...)` и `setFilePos ... ok=1`, музыка продолжает играть с нового места, **не** STOP
   - В UART команды (через Telnet 23 или Serial `telnet`? Просто ввод в UART если подключен CLI) : `seek 60000` → должен прыгнуть на 60 сек, в логе `##SEEK#: absolute 60000 ms`
   - Клик ближе к концу (90%) → не должно быть STOP, должен играть до конца и перейти на следующий трек.

5. **Тест WAV**:
   - WAV всегда + и -, клик в конец — должен seek работать без STOP (раньше `given position is too large`).

6. **Тест M4A**:
   - Файл `Свежесть леса.m4a` (которая раньше работала) + `skysea` где `dash` бренд. Клик по шкале — должен работать (fallback byte пропорционально). В UART `##SEEK#: M4A time seek ... via stsz index` если индекс готов, иначе просто byte pos.
   - Если `.m4a` на самом деле raw AAC ADTS (0xFF F1) — лог `m4a: no ftyp, looks like raw AAC (ADTS). Switch to CODEC_AAC` и играет как AAC.

7. **Проверка resume после URL**:
   - В WEB режиме проиграй радио, потом `mode sd` или перезапусти ESP с `smartstart` — SD трек должен стартовать с начала, а не с конца (проверка `sdResumePos=0` в onEOF).

8. **Проверка seek_start ускорения** (если подключишь кнопки позже):
   - `seek_start +` держит +5s каждые 0.5s, через 2 сек шаг 10s, через 4 сек 30s, >7 сек 60s. `seek_stop` — стоп.
   - Пока только через UART: `seek_start +`, подожди 3 сек, `seek_stop` → должно прыгнуть ~ + (5+10) sec.

Если после клика всё равно STOP → скопируй UART лог с `##SEEK#`, `##PROGRESS#`, `audio_eof`, `setFilePos`, `sdmin/sdmax` и пришли — будем смотреть, но после этих 5 правок STOP от `val-sd_min` должен пропасть радикально.

## Что дальше по ТЗ (OMNIA)

- Эти 4 пункта — база для флагманского seek. Только после них имеет смысл добивать реальный M4A time-seek по таблицам stsz/stsc/stco (RAM 8 байт/сек).
- USB-Flash уже скелет в `omniaplus/usb_host.h/cpp` — после стабилизации SD seek делать реальный `esp_usbh` + `vfs_fat` mount как копию SD (`listUSB`/`indexUSBPlaylist`).
- WebUI: убрать whitelist форматов для progress bar, добавить кнопки shuffle/repeat/usb/sdlist/seek, потом новая OMNIA WebUI.

## Файлы изменённые в этом коммите

- yoRadio/src/core/player.cpp:296 — убран `- player.sd_min` — критический фикс
- (ранее) config.cpp, audiohandlers.h, seek.cpp, netserver.cpp — уже были в main 84bc435

Все изменения запушены в main 3f2f80f.

