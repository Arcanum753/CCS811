#include "core_web/FSWebServerLib.h"

#include "core_state/core_state.h"

#include "module_ccs811.h"

#include <Wire.h>

// ============================================================
// I2C — низкоуровневые операции
// ============================================================

bool CLASS_MODULE_CCS811::readRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
    if (_addr == 0) { return false; }

    Wire.beginTransmission(_addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) { return false; }
    if (Wire.requestFrom((int)_addr, (int)len) != (int)len) { return false; }

    for (uint8_t i = 0; i < len; i++) {
        if (Wire.available() < 1) { return false; }
        buf[i] = (uint8_t)Wire.read();
    }
    return true;
}

bool CLASS_MODULE_CCS811::writeReg(uint8_t reg, uint8_t val) {
    return writeBytes(reg, &val, 1);
}

bool CLASS_MODULE_CCS811::writeBytes(uint8_t reg, const uint8_t *buf, uint8_t len) {
    if (_addr == 0) { return false; }

    Wire.beginTransmission(_addr);
    Wire.write(reg);
    for (uint8_t i = 0; i < len; i++) { Wire.write(buf[i]); }
    return (Wire.endTransmission() == 0);
}

// ============================================================
// Поиск и инициализация датчика
// ============================================================

bool CLASS_MODULE_CCS811::detect() {
    uint8_t addrs[2] = {
        CCS811_I2C_ADDR_PRIMARY, CCS811_I2C_ADDR_SECONDARY
    };
    if (_addr == CCS811_I2C_ADDR_SECONDARY) {
        addrs[0] = CCS811_I2C_ADDR_SECONDARY;
        addrs[1] = CCS811_I2C_ADDR_PRIMARY;
    }

    for (uint8_t i = 0; i < 2; i++) {
        uint8_t a = addrs[i];

        Wire.beginTransmission(a);
        if (Wire.endTransmission() != 0) { continue; }

        _addr = a;
        uint8_t id = 0;
        if (!readRegs(CCS811_REG_HW_ID, &id, 1)) {
            _addr = 0;
            continue;
        }

        if (id == CCS811_HW_ID_VALUE) {
            _hwId = id;
            DEBUGCCS811("CCS811 found at 0x%02X\r\n", a);
            return true;
        }

        _hwId = id;
        _addr = 0;
    }

    _hwId = 0;
    return false;
}

bool CLASS_MODULE_CCS811::initSensor() {
    if (!detect()) {
        DEBUGCCS811("CCS811: not found\r\n");
        return false;
    }

    if (!readStatus()) {
        DEBUGCCS811("status read failed\r\n");
        return false;
    }

    // После подачи питания чип может быть в boot mode: нужен APP_START.
    if (!(_status & CCS811_STATUS_APP_VALID)) {
        DEBUGCCS811("APP_VALID=0, software reset\r\n");

        const uint8_t resetSeq[4] = { 0x11, 0xE5, 0x01, 0x00 };
        if (!writeBytes(CCS811_REG_SW_RESET, resetSeq, 4)) {
            DEBUGCCS811("software reset failed\r\n");
            return false;
        }

        delay(100);

        if (!readStatus()) {
            DEBUGCCS811("status read after reset failed\r\n");
            return false;
        }
        if (!(_status & CCS811_STATUS_APP_VALID)) {
            DEBUGCCS811("APP_VALID still 0 after reset\r\n");
            return false;
        }
    }

    // APP_START — команда без данных.
    Wire.beginTransmission(_addr);
    Wire.write(CCS811_REG_APP_START);
    if (Wire.endTransmission() != 0) {
        DEBUGCCS811("APP_START failed\r\n");
        return false;
    }

    // После APP_START приложению нужно время до записи MEAS_MODE.
    delay(2);

    // Drive mode 1: измерение раз в секунду.
    if (!writeReg(CCS811_REG_MEAS_MODE, CCS811_MODE_1S)) {
        DEBUGCCS811("meas mode write failed\r\n");
        return false;
    }

    _initialized = true;
    _lastEnvMs = 0;
    DEBUGCCS811("initialized at 0x%02X\r\n", _addr);
    return true;
}

bool CLASS_MODULE_CCS811::readStatus() {
    uint8_t status = 0;
    if (!readRegs(CCS811_REG_STATUS, &status, 1)) { return false; }
    _status = status;
    return true;
}

bool CLASS_MODULE_CCS811::readResults() {
    uint8_t b[4];
    if (!readRegs(CCS811_REG_ALG_RESULT_DATA, b, 4)) { return false; }

    _eco2 = ((int32_t)b[0] << 8) | (int32_t)b[1];
    _tvoc = ((int32_t)b[2] << 8) | (int32_t)b[3];
    return true;
}

// ============================================================
// Компенсация по температуре/влажности (из HDC1080 через шину)
// ============================================================

bool CLASS_MODULE_CCS811::updateEnvCompensation() {
    if (!core_state.has("hdc1080.available")) { return false; }
    if (!core_state.getBool("hdc1080.available", false)) { return false; }

    float t = core_state.getF32("hdc1080.temperature", 25.0f);
    float h = core_state.getF32("hdc1080.humidity", 50.0f);

    if (t < -25.0f) { t = -25.0f; }
    if (t > 50.0f)  { t = 50.0f; }
    if (h < 0.0f)   { h = 0.0f; }
    if (h > 100.0f) { h = 100.0f; }

    // Формат CCS811: влажность в 1/512 %RH, температура в 1/512 °C с offset 25 °C.
    uint16_t hRaw = (uint16_t)(h * 512.0f + 0.5f);
    uint16_t tRaw = (uint16_t)((t + 25.0f) * 512.0f + 0.5f);

    uint8_t buf[4];
    buf[0] = (uint8_t)(hRaw >> 8);
    buf[1] = (uint8_t)(hRaw & 0xFF);
    buf[2] = (uint8_t)(tRaw >> 8);
    buf[3] = (uint8_t)(tRaw & 0xFF);

    bool ok = writeBytes(CCS811_REG_ENV_DATA, buf, 4);
    DEBUGCCS811("env compensation %.1f C / %.1f %% -> %s\r\n",
                t, h, ok ? "ok" : "failed");
    _lastEnvMs = millis();
    return ok;
}

// ============================================================
// Публикация и опрос
// ============================================================

void CLASS_MODULE_CCS811::publish(bool available, int32_t eco2, int32_t tvoc) {
    _available = available;
    if (available) {
        _eco2 = eco2;
        _tvoc = tvoc;
    } else {
        _eco2 = 0;
        _tvoc = 0;
    }

    core_state.signal("ccs811.available", BusValue::bo(_available));
    core_state.signal("ccs811.eco2",      BusValue::i32(_eco2));
    core_state.signal("ccs811.tvoc",      BusValue::i32(_tvoc));
}

void CLASS_MODULE_CCS811::invalidate() {
    _initialized = false;
    _nextInitMs = millis() + CCS811_INIT_RETRY_MS;
    publish(false, 0, 0);
}

void CLASS_MODULE_CCS811::pollStep() {
    if (!_wireStarted) { _wireStarted = true; Wire.begin(); }

    uint32_t now = millis();

    if (!_initialized) {
        if ((int32_t)(now - _nextInitMs) < 0) {
            publish(false, 0, 0);
            return;
        }
        if (!initSensor()) {
            _nextInitMs = now + CCS811_INIT_RETRY_MS;
            publish(false, 0, 0);
            return;
        }
    }

    if (!readStatus()) {
        DEBUGCCS811("status read failed, re-init\r\n");
        invalidate();
        return;
    }

    if (_status & CCS811_STATUS_ERROR) {
        uint8_t err = 0;
        if (readRegs(CCS811_REG_ERROR_ID, &err, 1)) { _errorId = err; }
        DEBUGCCS811("sensor error 0x%02X\r\n", _errorId);
        invalidate();
        return;
    }
    _errorId = 0;

    if (!(_status & CCS811_STATUS_DATA_READY)) {
        // Новых данных ещё нет: не публикуем старые значения как новые.
        if (!_available) { publish(false, 0, 0); }
        return;
    }

    if (!readResults()) {
        DEBUGCCS811("result read failed, re-init\r\n");
        invalidate();
        return;
    }

    publish(true, _eco2, _tvoc);

    if ((int32_t)(now - _lastEnvMs) >= (int32_t)CCS811_ENV_PERIOD_MS) {
        updateEnvCompensation();
    }
}

void CLASS_MODULE_CCS811::pollTask() {
    module_ccs811.pollStep();
}
