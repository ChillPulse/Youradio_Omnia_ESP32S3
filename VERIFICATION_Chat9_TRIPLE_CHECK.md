# Triple Check — Chat recommendation 9 — Выполнено

Дата: 2026-08-05
Коммит проверки: main (после 6ce292e и 097156e)

## Требования из Chat 9 и проверка

### 1) В репо всё ещё остался главный убийца seek: `val - player.sd_min` в `Config::setSDpos()`
**Требование:** Заменить на робастный вариант с clampAbsSdPos, принимать и абсолютное и относительное.

**Проверка:**
```cpp
// yoRadio/src/core/config.cpp:391
static uint32_t clampAbsSdPos(uint32_t val){
  uint32_t sdmin = player.sd_min;
  uint32_t sdmax = player.sd_max;
  if(sdmax == 0 || sdmax <= sdmin){
    sdmin = 0;
    sdmax = player.getFileSize();
  }
  uint32_t abs = val;
  if(!(val >= sdmin && val <= sdmax)){
    uint32_t len = (sdmax > sdmin) ? (sdmax - sdmin) : 0;
    if(len && val <= len) abs = sdmin + val;
  }
  if(abs < sdmin) abs = sdmin;
  if(sdmax && abs >= sdmax) abs = sdmax - 1;
  return abs;
}
void Config::setSDpos(uint32_t val){
  ...
  uint32_t absPos = clampAbsSdPos(val);
  ...
  player.setFilePos(absPos);
}
```
**Статус:** ✅ Выполнено — нет `val-player.sd_min`, есть `clampAbsSdPos`

### 2) В `audio_eof_stream()` всё ещё есть `- player.sd_min`
**Требование:** Заменить на `player.setResumeFilePos(config.sdResumePos);` и обнулять `sdResumePos=0` в onAudioEofCommon.

**Проверка:**
```cpp
// audiohandlers.h:195
player.setResumeFilePos(config.sdResumePos); // FIX: was ... - sd_min
// audiohandlers.h:167
config.sdResumePos = 0; // FIX: reset
```
**Статус:** ✅ Выполнено

### 3) В `seek.cpp` неверная проверка CODEC для M4A: `getCodec() == 7`
**Требование:** Заменить на `Audio::CODEC_M4A` (4), т.к. 7 = CODEC_OPUS.

**Проверка:**
```cpp
// seek.cpp:57
if(player.getCodec() == Audio::CODEC_M4A){ // FIX: was 7
```
**Статус:** ✅ Выполнено

### 4) SDLEN fallback лучше `sdmax==0 || sdmax<=sdmin`
**Требование:** В netserver.cpp case SDLEN

**Проверка:**
```cpp
// netserver.cpp:326
if(sdmax==0 || sdmax<=sdmin){
  sdmax = player.getFileSize();
  sdmin = 0;
}
```
**Статус:** ✅ Выполнено (было только sdmax==0)

### 5) audio_progress() должен быть start+END, не start+SIZE
**Требование:** В Audio.cpp все вызовы audio_progress(m_audioDataStart, m_audioDataSize) -> (m_audioDataStart, m_audioDataStart + m_audioDataSize)

**Проверка:**
```
grep audio_progress Audio.cpp
-> 3 occurrences with m_audioDataStart + m_audioDataSize
```
**Статус:** ✅ Выполнено

### 6) list vs sdlist
**Требование:** list = WEB stations native (e2002), sdlist = real SD names from PLAYLIST_SD_PATH

**Проверка:**
- list -> SPIFFS.open(PLAYLIST_PATH)
- sdlist -> SDPLFS()->open(PLAYLIST_SD_PATH) + parseCSV + real names, no 200 limit, yield every 50

**Статус:** ✅ Выполнено

### 7) Дополнительные фиксации из Chat 8/9
- m_f_allDataReceived >= audioDataStart+audioDataSize (было >= audioDataSize) ✅
- availableBytes = endPos - filePos where endPos = audioDataStart+audioDataSize ✅
- progress always send even if 0/0 ✅
- fallback bitrate 128k ✅
- unified EOF handler audio_eof + audio_eof_mp3 -> onAudioEofCommon ✅
- sdResumePos reset on EOF ✅
- shuffle fix without skip, repeat OFF/ONE/ALL, shuffle combinations ✅

## Итог тройной проверки
Все 4 критических пункта из Chat 9 (+ дополнительные) присутствуют в main ветке.

Коммиты проверки:
- 097156e: SDLEN fallback sdmax==0 || sdmax<=sdmin
- 6ce292e: USB-Flash copy of SD skeleton
- b30053a etc: list/sdlist, seek, shuffle

Сборка: последний Log13/Trabl с ошибками ipToStr был из коммита d861d12 (e2002 telnet.cpp несовместимый). Сейчас main компилируется (проверено локально, нет ipToStr ошибок).

