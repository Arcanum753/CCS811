# module_ccs811

Модуль датчика качества воздуха CCS811 (eCO2/TVOC) для прошивки
[avr-fota](https://github.com/Arcanum753/avr-fota).

- I2C, адреса `0x5A`/`0x5B` (автоопределение), `HW_ID = 0x81`.
- Ресурсы шины: `ccs811.available`, `ccs811.eco2` (ppm), `ccs811.tvoc` (ppb).
- Компенсация t/RH из `hdc1080.*` (если модуль HDC1080 в сборке).
- Веб-страница `ccs811.html` — только показ значений.
- Терминал: `ccs`, `ccs show`, `ccs help`.
- Конфига нет; baseline не сохраняется, после включения нужен burn-in 20–30 минут.

Подключение: склонировать репозиторий в `src/module_ccs811` и добавить
`+<module_ccs811/>` в `src_filter` нужного env. Подробности — `AGENTS.md`.
