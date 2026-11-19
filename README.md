# module_ccs811

Модуль датчика качества воздуха CCS811 для прошивки
[avr-fota](https://github.com/Arcanum753/avr-fota).

## Description

**RU.** Опциональный модуль CCS811 (I2C): эквивалентный CO₂ (eCO2) и TVOC. Публикует
на ресурсную шину `core_state` доступность и значения, показывает их на веб-странице и
в последовательном терминале. Компенсация t/RH берётся из ресурсов `hdc1080.*`, если
`module_hdc1080` присутствует в сборке. Настроек нет.

**EN.** Optional CCS811 module (I2C): equivalent CO₂ (eCO2) and TVOC. Publishes
availability and readings to the `core_state` resource bus and exposes them via a web page
and the serial terminal. Temperature/humidity compensation is taken from `hdc1080.*`
resources when `module_hdc1080` is built in. No settings.

## Features

- I2C addresses `0x5A`/`0x5B`, auto-detection; `HW_ID == 0x81`.
- Init: `APP_VALID` check, optional software reset, `APP_START`, drive mode 1 (`0x10`).
- Read-only resources: `ccs811.available`, `ccs811.eco2`, `ccs811.tvoc`.
- Environmental compensation from HDC1080 (`ENV_DATA`, every 60 s).
- Error handling with `ERROR_ID` and a 5-second re-init backoff.
- Web page `ccs811.html` with 1-second refresh; terminal command `ccs`.
- No config file; baseline is not persisted. Burn-in is required after power-up.

## Resources

| Resource | Type | Description |
|---|---|---|
| `ccs811.available` | BOOL ro | initialized, valid data, no error |
| `ccs811.eco2` | I32 ro | equivalent CO₂, ppm |
| `ccs811.tvoc` | I32 ro | TVOC, ppb |

## Web and terminal

- `GET /ccs811/info` — state, address, status, error, readings (CVT).
- `GET /ccs811/read` — readings only (polled every second).
- `GET /ccs811/ver` — module version.
- `ccs`, `ccs show`, `ccs help`.

## Macro example

```lua
every("*/30 * * * * *", function()
    print("CCS811", ccs811.available, ccs811.eco2, ccs811.tvoc)
end)
```

Ready-to-use cron examples: `module_macros/web/macros/sensors_15s.lua`,
`sensors_30s.lua`, `sensors_1m.lua`, `sensors_5m.lua`.

## Installation

1. Clone this repository into `src/module_ccs811`.
2. Add `+<module_ccs811/>` to the target env `src_filter`.
3. Add `src/module_ccs811/module_ccs811.ini` to `extra_configs`.
4. Optional: add `module_hdc1080` for t/RH compensation.
5. Build the env; the module is registered via `[registry]`.

## Tested

2026-11-19, ESP32, env `esp32-i2c-mapper`, firmware `0.117.20261119_1754.2104`:

- Address `0x5A`, `HW_ID 0x81`.
- Readings after warm-up: `400–420 ppm` eCO2, `0–3 ppb` TVOC.
- Compensation confirmed in the log:
  `[M_CCS811] env compensation 29.4 C / 72.8 % -> ok`.
- Web pages/endpoints and terminal commands verified.
- Fixed during testing: drive mode is written as `0x10` (bits 6:4 of `MEAS_MODE`).

## Documentation

See `AGENTS.md` for architecture, requirements and limitations.
