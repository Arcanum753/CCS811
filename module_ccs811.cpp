#include "core_web/FSWebServerLib.h"

#include "core_state/core_state.h"
#include "core_task/core_task.h"
#include "core_terminal/core_terminal.h"
#include "core_terminal/ErriezSerialTerminal.h"

#include "module_ccs811.h"
#include "module_ccs811_version.h"
#include "core_sys/eertos.h"

#include <Wire.h>
#include <stdio.h>

CLASS_MODULE_CCS811 module_ccs811;

CLASS_MODULE_CCS811::CLASS_MODULE_CCS811() {
    _wireStarted = false;
    _available   = false;
    _initialized = false;
    _addr        = 0;
    _hwId        = 0;
    _status      = 0;
    _errorId     = 0;
    _eco2        = 0;
    _tvoc        = 0;
    _nextInitMs  = 0;
    _lastEnvMs   = 0;
}

// ============================================================
// begin()
// ============================================================
void CLASS_MODULE_CCS811::begin() {
    DEBUGCCS811("%s\r\n", __FUNCTION__);

    if (!_wireStarted) { _wireStarted = true; Wire.begin(); }

    _addr        = 0;
    _hwId        = 0;
    _status      = 0;
    _errorId     = 0;
    _available   = false;
    _eco2        = 0;
    _tvoc        = 0;

    _initialized = initSensor();
    if (!_initialized) {
        DEBUGCCS811("CCS811: init failed, retry in %u ms\r\n", (unsigned)CCS811_INIT_RETRY_MS);
        _nextInitMs = millis() + CCS811_INIT_RETRY_MS;
    }

    terminalRegister();

    core_task.every("ccs811_poll", pollTask, CCS811_POLL_MS, true);
}

void CLASS_MODULE_CCS811::begin(ModContext& ctx) {
    (void)ctx;
    begin();
}

// ============================================================
// web_Init()
// ============================================================
void CLASS_MODULE_CCS811::web_Init() {
    DEBUGCCS811("%s\r\n", __FUNCTION__);

    ESPHTTPServer.on("/ccs811/info", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleInfo(request);
    });

    ESPHTTPServer.on("/ccs811/read", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!ESPHTTPServer.checkAuth(request)) { return request->requestAuthentication(); }
        this->handleRead(request);
    });

    ESPHTTPServer.on("/ccs811/ver", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->html_ver_get(request);
    });
}

// ============================================================
// register_resources() — публикация ресурсов на шине core_state
// ============================================================
void CLASS_MODULE_CCS811::register_resources() {
    DEBUGCCS811("%s\r\n", __FUNCTION__);

    core_state.regState("available", BusValue::BOOL, "CCS811 present and ready", false);
    core_state.regState("eco2",      BusValue::I32,  "equivalent CO2 (ppm)",     false);
    core_state.regState("tvoc",      BusValue::I32,  "TVOC (ppb)",               false);
}

// ============================================================
// Веб-обработчики
// ============================================================
String CLASS_MODULE_CCS811::statusText() const {
    if (_addr == 0) { return "no device"; }
    if (_available) { return "OK"; }
    if (_status & CCS811_STATUS_ERROR) {
        char buf[24];
        snprintf(buf, sizeof(buf), "error 0x%02X", _errorId);
        return String(buf);
    }
    if (!_initialized) { return "initializing"; }
    return "warming up";
}

void CLASS_MODULE_CCS811::appendValues(String &values) {
    values += "ccs_available|" + String(_available ? "available" : "not available") + "|div\n";

    if (_available) {
        values += "ccs_eco2|" + String(_eco2) + " ppm|div\n";
        values += "ccs_tvoc|" + String(_tvoc) + " ppb|div\n";
    } else {
        values += "ccs_eco2|--|div\n";
        values += "ccs_tvoc|--|div\n";
    }

    values += "ccs_status|" + statusText() + "|div\n";
}

void CLASS_MODULE_CCS811::handleInfo(AsyncWebServerRequest *request) {
    DEBUGCCS811("%s\r\n", __FUNCTION__);

    String values = "";
    char buf[16];

    snprintf(buf, sizeof(buf), "0x%02X", _addr);
    values += "ccs_addr|" + String(buf) + "|div\n";

    if (_errorId != 0) {
        snprintf(buf, sizeof(buf), "0x%02X", _errorId);
        values += "ccs_error|" + String(buf) + "|div\n";
    } else {
        values += "ccs_error|--|div\n";
    }

    appendValues(values);
    request->send(200, "text/plain", values);
}

void CLASS_MODULE_CCS811::handleRead(AsyncWebServerRequest *request) {
    String values = "";
    appendValues(values);
    request->send(200, "text/plain", values);
}

// ============================================================
// Версионные методы
// ============================================================
String CLASS_MODULE_CCS811::getVersionStr() {
    return String(MODULE_CCS811_VERSION);
}

String CLASS_MODULE_CCS811::getGeneratedTime() {
    return String(MODULE_CCS811_GENERATED_TIME);
}

String CLASS_MODULE_CCS811::getCommitDateStr() {
    return String(MODULE_CCS811_COMMIT_DATE_STR);
}

void CLASS_MODULE_CCS811::html_ver_get(AsyncWebServerRequest *request) {
    DEBUGCCS811("%s\r\n", __FUNCTION__);
    String values = "";
    values += "ccsversion|" + getVersionStr()    + "|div\n";
    values += "ccsgentime|" + getGeneratedTime() + "|div\n";
    values += "ccsgendate|" + getCommitDateStr() + "|div\n";
    request->send(200, "text/plain", values);
}

// ============================================================
// Публичное API
// ============================================================
bool CLASS_MODULE_CCS811::isAvailable() const { return _available; }
int32_t CLASS_MODULE_CCS811::getEco2() const { return _eco2; }
int32_t CLASS_MODULE_CCS811::getTvoc() const { return _tvoc; }
uint8_t CLASS_MODULE_CCS811::getAddr() const { return _addr; }
uint8_t CLASS_MODULE_CCS811::getStatus() const { return _status; }
uint8_t CLASS_MODULE_CCS811::getErrorId() const { return _errorId; }
