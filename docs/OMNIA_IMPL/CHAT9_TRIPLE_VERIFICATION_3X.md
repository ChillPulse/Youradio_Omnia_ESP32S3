# Chat Recommendation 9 — Тройная проверка 3x FINAL
Дата: 2026-08-06 05:51 MSK, коммит origin/main 9e9344c
Ссылка: https://github.com/ChillPulse/Info-spravka/blob/main/Chat%20reomedation%209
Статус: ✅ ВСЕ 4 ПУНКТА ИСПРАВЛЕНЫ + скрытый 5-й

## Методика тройной проверки
1-я проверка: чтение исходника из Chat9 + сравнение с текущим main (grep)
2-я проверка: поиск остатков бага `sd_min` вычитаний (`grep -R "sdResumePos -"` и `val.*sd_min`)
3-я проверка: клон clean main + визуальная сверка функций с эталонным скелетом из Chat9

---

### Пункт 1 — setSDpos убийца seek (config.cpp)

**Требование Chat9:**
> Сейчас в `config.cpp` у вас по-прежнему: `setResumeFilePos(val-player.sd_min)` и `setFilePos(val-player.sd_min)`.
> Заменить на робастный вариант с `clampAbsSdPos` принимающий абсолютное и относительное.

**Факт в main после фикса (строки 391-432):**
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
  if(getMode()!=PM_SDCARD) return;
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
```
**Проверки:**
- `grep "val.*sd_min" config.cpp` → 0 совпадений (кроме комментария FIX)
- `grep "clampAbsSdPos"` → 1 определение + 1 вызов
- Логика = эталон из Chat9 (абсолют vs относительное, clamp sdmax-1)
✅ Пройдено

### Пункт 2 — audio_eof_stream() минус sd_min (audiohandlers.h)

**Требование:**
> В `audiohandlers.h` сейчас осталось: `config.sdResumePos==0?0:config.sdResumePos-player.sd_min`.
> Заменить на `player.setResumeFilePos(config.sdResumePos);` и обнулять `sdResumePos=0` в `onAudioEofCommon`

**Факт в main (строки 164-199):**
```cpp
static void onAudioEofCommon(const char* info){
    g_lastEofMs = millis(); g_eofHandled = true;
    config.sdResumePos = 0; // FIX: reset ...
    ...
}
void audio_eof_stream(const char *info){
  player.sendCommand({PR_STOP, 0});
  if(!player.resumeAfterUrl) return;
  if(config.getMode()==PM_WEB) player.sendCommand({PR_PLAY, config.lastStation()});
  else{
    player.setResumeFilePos(config.sdResumePos); // FIX: was minus sd_min
    player.sendCommand({PR_PLAY, config.lastStation()});
  }
}
```
**Проверки:**
- `grep "sdResumePos.*sd_min" audiohandlers.h` → 0
- `grep "sdResumePos = 0" audiohandlers.h` → строка 167 есть
✅ Пройдено

### Пункт 3 — seek.cpp CODEC_M4A == 7 баг

**Требование:**
> `if(player.getCodec()==7)` — но `CODEC_M4A=4`, `7=CODEC_OPUS`, M4A seek никогда не вызывается.
> Заменить на `Audio::CODEC_M4A`

**Факт в main (seek.cpp:57):**
```cpp
if(player.getCodec() == Audio::CODEC_M4A){ // FIX: was 7 which is CODEC_OPUS=7, CODEC_M4A=4
    if(player.omnia_m4aSeekMs(ms)) ...
}
```
**Проверки:**
- `grep "CODEC_M4A" seek.cpp` → есть
- `grep "== 7" seek.cpp` → нет
- В AudioEx.h: `CODEC_M4A=4, CODEC_OPUS=7` подтверждается
✅ Пройдено

### Пункт 4 — SDLEN fallback (netserver.cpp)

**Требование:**
> Сейчас fallback только если `sdmax==0`. Сделать `sdmax==0 || sdmax<=sdmin`

**Факт в main (строки 321-331):**
```cpp
case SDLEN: {
  uint32_t sdmin = player.sd_min;
  uint32_t sdmax = player.sd_max;
  if(sdmax==0 || sdmax<=sdmin){
    sdmax = player.getFileSize();
    sdmin = 0;
  }
  sprintf(wsbuf,"{\"sdmin\": %d,\"sdmax\": %d}", sdmin, sdmax);
```
**Проверки:**
- `grep -A2 "case SDLEN" netserver.cpp` → содержит `|| sdmax<=sdmin`
✅ Пройдено

### Бонус пункт 5 — player.cpp скрытый убийца (не в списке, но критичен)

**Найдено при grep:**
`connecttoFS(..., sdResumePos-player.sd_min)` — тот же баг что в п.1, ломает resume после URL.

**Исправлено в коммите 3f2f80f строка 296:**
```cpp
isConnected=connecttoFS(sdman,config.station.url,config.sdResumePos==0?_resumeFilePos:config.sdResumePos);
```
Без вычитания. Проверка `grep "sdResumePos.*sd_min" player.cpp` → 0 (только в комментарии).

---

## Итоговые grep-логи (3-я проверка, clean clone)

```
config.cpp setSDpos:
391:static uint32_t clampAbsSdPos
412:void Config::setSDpos
audiohandlers.h:
164:onAudioEofCommon
167:config.sdResumePos = 0;
189:audio_eof_stream
195:setResumeFilePos(config.sdResumePos);
seek.cpp:
57: if(player.getCodec() == Audio::CODEC_M4A)
netserver SDLEN:
326: if(sdmax==0 || sdmax<=sdmin){
player.cpp:
296: connecttoFS(..., config.sdResumePos==0?_resumeFilePos:config.sdResumePos);
```

Все вычитания `sdResumePos - sd_min` и `val - sd_min` удалены.

## Как проверять после прошивки (для проверки на ушах + WebUI + UART 115200)

См. предыдущий документ CHAT9_TRIPLE_CHECK_FINAL.md — тесты sample3.aac, sample4.aac, WAV конец, M4A dash и Свежесть леса, seek 60000, seek_start +/-, клик 90%.

## Push

- 3f2f80f Fix Chat9 triple-check
- 9e9344c Docs triple-check final
- origin/main up-to-date, token очищен

Вывод: все задания по https://github.com/ChillPulse/Info-spravka/blob/main/Chat%20reomedation%209 выполнены, трижды перепроверены, запушены.
