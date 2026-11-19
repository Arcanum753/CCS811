#ifndef _MODULE_CCS811_TYPES_h
#define _MODULE_CCS811_TYPES_h

// ============================================================
// module_ccs811_types.h — адреса, регистры и константы CCS811.
// Реализация: module_ccs811.cpp (шаблон) и module_ccs811_engine.cpp
// (I2C, инициализация, опрос, компенсация t/RH).
// ============================================================

#include <Arduino.h>
#include <stdint.h>

// Адреса на шине I2C (выбираются перемычкой ADDR)
#define CCS811_I2C_ADDR_PRIMARY     0x5A
#define CCS811_I2C_ADDR_SECONDARY   0x5B

// Регистры CCS811
#define CCS811_REG_STATUS           0x00
#define CCS811_REG_MEAS_MODE        0x01
#define CCS811_REG_ALG_RESULT_DATA  0x02
#define CCS811_REG_ENV_DATA         0x05
#define CCS811_REG_BASELINE         0x11
#define CCS811_REG_HW_ID            0x20
#define CCS811_REG_ERROR_ID         0xE0
#define CCS811_REG_APP_START        0xF4
#define CCS811_REG_SW_RESET         0xFF

// Идентификатор чипа
#define CCS811_HW_ID_VALUE          0x81

// Биты регистра STATUS
#define CCS811_STATUS_ERROR         0x01
#define CCS811_STATUS_DATA_READY    0x08
#define CCS811_STATUS_APP_VALID     0x10
#define CCS811_STATUS_FW_MODE       0x80

// Режим измерения: drive mode 1 (измерение раз в секунду).
// DRIVE_MODE — биты 6:4 регистра MEAS_MODE (001 = 1 с).
#define CCS811_MODE_1S              0x10

// Периоды, мс
#define CCS811_POLL_MS              1000
#define CCS811_INIT_RETRY_MS        5000
#define CCS811_ENV_PERIOD_MS        60000

#endif // _MODULE_CCS811_TYPES_h
