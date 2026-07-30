# OMNIA — Что ещё нужно после базы v8.2 (прогресс для всех форматов, USB-Flash как копия SD, UART, телеметрия, Web UI / App)

**Дата:** 2026-07-30
**Статус текущего модуля:** База V7G + SD без WiFi + Shuffle fix без пропуска + Repeat OFF/ONE/ALL + Shuffle комбинации + Seek 60000/rel/percent + sdlist без лимита 267+ + PROGRESS для FLAC (338000ms) — работает, Web UI прогресс бар уже есть, но для MP3 пока 0 0.

**Вопрос заказчика:** "Да, прогресс нужен для всех форматов которые читаются модом maleksm. Точно больше ничего не нужно кроме поднять baud? а как же добавить чтение usb-flash (всё под копирку из sd mode)? Ещё раз перечитай ТЗ и особенно обсуждения на тему добавления нужных функций, уарт команд и телеметрии ответа, усб, модернизации веб морды (приложения омниа). что ещё может быть нужно добавить для омниа?"

**Ответ:** Поднять baud — далеко не всё. Ниже полный чек-лист из MASTER SPEC v8.2 CLEAN FINAL + обсуждений 2213 строк + Comments v7.2.

---

## 1. Прогресс для всех форматов Maleksm (MP3, FLAC, WAV, AAC, OGG, OPUS, M4A?)

**Сейчас:** PROGRESS работает для FLAC (Duration 338000 ms) потому что FLAC имеет total samples в stream, `getAudioFileDuration()` возвращает секунды. Для MP3 CBR/VBR иногда возвращает 0, поэтому в логе `PROGRESS#: 0 0 1 0 262 267`.

**Нужно:** Для всех форматов отдавать `curMs/durMs/percentX10` даже если `getAudioFileDuration()==0`.

**Как (из обсуждений + Audio.cpp):**
- `getFileSize()` + `getFilePos()` всегда есть для FS файлов (SD/USB).
- `getAudioCurrentTime()` — секунды, может быть 0 для MP3 без Xing header.
- `getAudioFileDuration()` — секунды, 0 если не распарсилось.
- Вычислять percent по двум источникам:
  - Если durMs>0: `percent = curMs*1000/durMs`
  - Иначе если fileSize>0: `percent = filePos*1000/fileSize`, `curMs = percent*durMs/1000` если durMs известно из ID3, иначе `curMs = filePos * 8 * 1000 / (bitrate*1000)` — bitrate есть в AUDIO.INFO BitRate
  - Для CBR MP3: `dur = fileSize*8 / bitrate`
  - Для VBR MP3: использовать Xing header если есть, иначе fallback на filePos/fileSize для percent, а время — `cur = percent*duration/1000` где duration из `getAudioFileDuration()` или из ID3 `Duration` (в логе `Duration: 264 (s)` уже есть)
- Отправлять `##PROGRESS#: curMs durMs state percentX10 idx total` 2-5Hz обычно, 10Hz при seek.

**Проверка для всех форматов:**
- MP3 CBR 128/320k, MP3 VBR, FLAC 16bit/44.1k, WAV, AAC, OGG, OPUS — seek внутри и прогресс без 0 0.

---

## 2. USB-Flash чтение — всё под копирку из SD mode

**Железо:** уже сделано GPIO19 D- / GPIO20 D+ S3 internal OTG PHY + 5V 1A+ LDO + 100nF+10uF VBUS + TVS опц + polyfuse 0.75A 6V на USB-A мама. Onboard USB-C только Device — Host через 19/20 правильный путь.

**Софт — копирка SD:**

1. **USB Host MSC Stack:** ESP-IDF `usb_host` + `esp_vfs_fat` или Arduino `USBH_MSC`. Инициализация `usb_host_install()`, task `usb_host_lib_handle_events()`, callback connect/disconnect → `##USB.STATUS#: mounted/ejected/error size=... fs=... files=...`
2. **FatFS mount:** `esp_vfs_fat_usb_host_mount`, поддержка FAT32/FAT16/exFAT (включить exFAT в menuconfig). Общий GND.
3. **Сканирование:** рекурсивно `listSD()` аналог для USB — `listUSB()`, фильтр расширений `.mp3 .flac .aac .m4a? .wav .ogg .opus` — те же что SD, но `.m4a` в логе сейчас `not supported` — либо добавить decoder, либо фильтровать из списка чтобы не пытаться играть (как в логе `Error connecting to /Зелёный храм.m4a`).
4. **Индекс:** как SD: `/data/playlistusb.csv` + `/data/indexusb.dat` или переиспользовать тот же механизм что SD, но отдельный список USB.
5. **Audio:** `connecttoFS(usbman, path, pos)` — тот же API что SD, через VFS file handle.
6. **Команды:** `mode usb` / `mode 3`, `usb_scan`, `usb_list`, `usb_play <path/index>`, `next/prev/seek/progress/meta` идентично SD.
7. **Hot-plug:** detect mount/eject, safe unmount, сообщение `##USB.STATUS#` + toast на S2, без краша/heap leak, повторная вставка → авто rescan.
8. **Power:** разные флешки требуют больше тока — защита `USB Power Low`.

**Этапы из обсуждений:**
- USB-0 hardware check (питание, полифьюз, кондеры, D+/D-)
- USB-1 Host init VID/PID
- USB-2 Mount FatFS list root
- USB-3 Play one file via Audio
- USB-4 Full playlist + next/prev + progress/seek + meta
- USB-5 Hot-plug + Web UI

**Проверка:** флешки FAT32/exFAT 8-64GB, кириллица, длинные пути, >2GB, выдергивание во время play → stop без краша, повторная вставка → rescan.

---

## 3. UART команды и телеметрия — полный список из обсуждений

**Старый CLI сохраняется (из файла UART 5KB):** `mode 0/1/2`, `prev/next/toggle/stop/start/play`, `vol/vol+/vol-/vol x`, `audioinfo`, `smartstart`, `list`, `play x`, `info`, `dspon`, `dim`, `sleep`, `tzo`, `date`, `version`, `heap`, `boot`, `reset`, `wifi.list/con/station/status/rssi`

**Новые v8.2 (уже частично реализованы, но нужно добить):**

| Команда | Направление | Назначение | Статус сейчас |
|---|---|---|---|
| `mode usb` / `mode 3` | A→Y | USB-Flash | реализовано частично, нужен mount |
| `seek <ms>` `seek_rel +/-ms` `seek_percent 0-1000` | A→Y | Seek внутри трека | работает (setFilePos), нужно добавить soft-mute |
| `sdpos <pos>` | A→Y | Байтовая позиция | есть |
| `seek_start +/-` `seek_stop` | A→Y | Hold ускоренная перемотка | реализовано, нужно ускорение 5s/10s/30s/60s |
| `shuffle on/off` `repeat off/one/all` `shuffle_repeat` `random` | A→Y | Shuffle/Repeat | работает, но repeat one/off нужно добить auto-next в audio_eof_mp3 |
| `usb_scan/list/play` | A→Y | USB | заглушка TODO |
| `status` | A→Y | Расширенный статус | сейчас WiFi status, нужен OMNIA status |
| `omnia_status`/`mstatus` | A→Y | Активные функции mode/shuffle/repeat/total/cur | реализовано |
| `sdlist` / `list sd` | Y→A | SD реальные имена | реализовано без лимита |
| `ping <seq>` | A→Y | Watchdog | реализовано |

**Телеметрия Y→A→S2 (для OMNIA):**

| Сообщение | Частота | Содержимое | Статус |
|---|---|---|---|
| `##PROGRESS#: curMs durMs state percentX10 idx total` | 2-5Hz, 10Hz при seek | cur/dur/state/percent/idx/total | работает для FLAC, нужен для всех форматов |
| `##META#: title=...;artist=...;album=...;year=...;bitrate=...;fmt=...;sr=...;ch=...` | on-change | ID3 | работает частично (ID3 есть) |
| `##TRACK#: idx/total/path` | on-change | Track info | есть |
| `##SD.STATUS#: mounted/ejected/indexing/error size fs files` | on-change | SD status | есть |
| `##USB.STATUS#: mounted/ejected/error` | on-change | USB status | заглушка, нужен mount |
| `##AUDIO.INFO#`, `##CLI.META#`, `##CLI.NAMESET#` | on-change | Совместимость | есть |

**Что ещё нужно из обсуждений (UART дисциплина):**
- AT32 = умный хаб: парсит, валидирует, rate-limit, форвардит, добавляет seq/timestamp, приоритизирует очереди FreeRTOS HIGH commands/SPEC, MEDIUM PROGRESS, LOW STATUS.
- Высокочастотные SPEC 20-30Hz — бинарный протокол с header `0xAA msg_id len seq payload CRC8/16`
- Baud: стенд 115200, OMNIA цель 460800/921600 (меньше latency)
- Quality Scaling: при перегрузке UART снижать rate SPEC/PROGRESS
- Документация `docs/uart_protocol_v1.md` с примерами пакетов, state machine 5 источников (WEB/SD/USB-Flash/BT/USB Audio), error codes

---

## 4. Модернизация Web морды (приложения OMNIA)

**Сейчас родная Web UI yoRadio уже имеет:**
- Список SD треков + прогресс бар со временем `сколько всего будет играть / сколько уже сыграно` + шкала которую можно мышкой двигать и менять место
- `sdpos/sdtpos/sdlen/sdmin/sdmax` via WebSocket JSON

**Что нужно для OMNIA (из обсуждений):**
- **Web UI обновить под:** USB list, progress/seek/USB, shuffle/repeat иконки, `sdlist` без лимита, отображение `##OMNIA.STATUS#`
- **Primary control via yoRadio:** Phone App ↔ WiFi ↔ YoRadio S3 ↔ UART ↔ AT32 ↔ UART ↔ S2 (AURA). S3 остаётся точкой входа WiFi, S2 максимально разгружен под render + local input. Secondary optional S2 MQTT/WebSocket для HA/OTA.
- **App / PWA:** лёгкий PWA на S3 или S2, который показывает 5 источников (WEB/SD/USB-Flash/BT/USB Audio PC/Android), прогресс, seek, meta, shuffle/repeat, volume, presets. Форвард команд через yoRadio UART.
- **Интеграция с AT32:** AT32 форвардит PROGRESS/META/TRACK/STATUS на S2, S2 рендерит красивую шкалу + time + percent + waveform для OMNIA дисплея

---

## 5. Что ещё может понадобиться для OMNIA (из ТЗ v8.2 и обсуждений)

- **Volume sync + Soft Mute координация:** `vol` команды должны синхронизироваться между yoRadio и AT32 GainSmoother (soft unmute 400ms default)
- **Source Matrix FINAL 5:** yoRadio WEB/SD/USB-Flash + BT401 BT/USB Audio, TF/U-disk отключены — уже зафиксировано в `SOURCES_v8.1_MATRIX.md`, нужно в коде деактивировать BT401 TF/U-disk
- **External Flash GPIO10-13 any vendor JEDEC:** для AURA assets (Needle фоны, шрифты кириллица, иконки, темы) — уже в ТЗ, но для yoRadio не нужно
- **Visual Pack / RGB кольцо дыхание 8с / Dual PEQ Room 16+Creative 12** — для S2, не для yoRadio, но в OMNIA интеграции нужно чтобы PROGRESS/META доходили до S2
- **Backup/Restore / Onboarding:** `omniactl.py export-presets / import-presets`, backup NVS, Room profiles — для OMNIA, не для yoRadio, но Web UI должен иметь кнопку backup

---

## 6. Чек-лист оставшихся задач для OMNIA после текущей базы

- [ ] PROGRESS для всех форматов Maleksm (MP3 CBR/VBR, FLAC, WAV, AAC, OGG, OPUS) — вычисление через fileSize/bitrate/duration, не только FLAC
- [ ] USB-Flash Host MSC полная реализация (не заглушка) — mount/list/play/next/prev/seek/meta/hot-plug
- [ ] Фильтр .m4a — либо добавить decoder, либо исключить из SD/USB скана чтобы не было `Error connecting to /...m4a`
- [ ] UART телеметрия: `##PROGRESS#` для всех форматов, `##META#` полностью, `##TRACK#`, `##SD/USB.STATUS#` с size/fs/files
- [ ] Команды: `seek` с soft-mute, `seek_start +/-` + `seek_stop` с ускорением, `shuffle/repeat` с ответами `##SHUFFLE#:`, `##REPEAT#:`, `omnia_status`
- [ ] SD без WiFi уже работает (твой лог `Ready in SD Mode!` + `SD Mounted` даже без роутера) — оставить как есть, не запускать infinite searchWiFi tasks (пофиксили)
- [ ] Web UI модернизация: USB list, progress bar seekable для всех форматов, shuffle/repeat иконки, omnia_status
- [ ] Поднять baud до 460800/921600, увеличить UART buffer, DMA, FreeRTOS queues, приоритеты, binary framing для SPEC+PROGRESS
- [ ] AT32 как хаб: парсинг/валидация/rate-limit/форвард/seq/timestamp
- [ ] Документация `uart_protocol_v1.md`, `yoRadio_modernization.md`, `vendor_bt401_mclk_confirm.md` + `SOURCES_v8.1_MATRIX.md` уже есть
- [ ] Интеграция в OMNIA: заменить STM32F401 стенд на AT32, сохранить RATE/MUTE handshake, форвард PROGRESS/META на S2

---

**Итог:** Поднять baud — далеко не всё. Нужно добить PROGRESS для всех форматов, USB-Flash как копия SD, полную UART телеметрию и команды с ответами, Web UI / App для OMNIA (прогресс бар, seek мышкой, SD/USB списки, shuffle/repeat), и интеграцию AT32 как хаб.

Этот файл готов к вставке как `docs/OMNIA_REMAINING_TASKS.md` и закрывает вопрос из обсуждений.
