# OMNIA Implementation — как тестировать временной связкой S3+STM32 только на слух + Web UI + UARTы

Вопрос заказчика: "проверить временную связку esp32-s3 + stm32f401rct6 (только для модернизации ё-радио) я могу лишь на слух, через свой родной web ui ё-радио и uart esp32-s3, uart stm32f401rct6 (через него я проверял мьют и прочее. наверное в модернизации этот uart не нужен?)"

Ответ в TESTING_STM32_S3_LINK.md — STM32 UART полезен как debug для mute/rate retune, но не обязателен для функционала. После готовности Ё-радио стенд STM32 убирается, заменяется AT32.

Папки:
- myoptions_omnia_final.h — FINAL пины GPIO2 MUTE GPIO4 RATE GPIO19/20 USB Host
- src/core/omniaplus/ — модули progress/seek/shuffle_fix/usb_host/cli_extensions — вызывают из player.loop() и main.cpp

Как подключать в существующий проект:
1. В yoRadio.ino или core/options.h подключить myoptions_omnia_final.h вместо myoptions.h (или скопировать пины в существующий).
2. В player.cpp loop добавить:
   #include "omniaplus/progress.h"
   #include "omniaplus/seek.h"
   #include "omniaplus/usb_host.h"
   #include "omniaplus/cli_extensions.h"
   omnia_progress_loop(); omnia_usb_host_loop();
3. В telnet/command handler добавить вызов omnia_cli_handle(line) до старой обработки — чтобы новые команды seek/shuffle/repeat/usb_* перехватывались.
4. Для SD без WiFi фикса: в main.cpp убрать зависимость network.status от SD ready — в твоем main.cpp уже есть if (network.status != CONNECTED && network.status!=SDREADY) — это уже почти фикс, проверь что config.initPlaylistMode и player.initHeaders вызываются даже без WiFi.

Тест только с Web UI + UART S3 + ухо — достаточно, STM32 UART опционально.

См. MASTER_SPEC_v8.2_CLEAN_FINAL.md раздел Часть 0.
