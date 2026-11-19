#ifndef _MODULE_CCS811_h
#define _MODULE_CCS811_h

#include "main.h"

#include "mod_context.h"

#ifdef DEBUG_CCS811
#define DEBUGCCS811(...) DBG_MOD("[M_CCS811] ", __VA_ARGS__)
#else
#define DEBUGCCS811(...)
#endif

#include "module_ccs811_types.h"

// ============================================================
// module_ccs811 — датчик качества воздуха CCS811 (I2C): eCO2 и TVOC.
// Публикует на шину только доступность и значения; настроек нет.
// ============================================================
class CLASS_MODULE_CCS811 {
public:
    CLASS_MODULE_CCS811();
    void begin();
    void begin(ModContext& ctx);
    void web_Init();
    void register_resources();

    // ===== Публичное API =====
    void pollStep();
    bool isAvailable() const;
    int32_t getEco2() const;
    int32_t getTvoc() const;
    uint8_t getAddr() const;
    uint8_t getStatus() const;
    uint8_t getErrorId() const;

private:
    // Терминальные команды: статические методы-члены, поэтому имеют
    // прямой доступ к приватному состоянию модуля (см. _terminal.cpp).
    void terminalRegister();
    static void s_terminalCmd();
    static void s_cmdShow();

    // Версионные методы
    String getVersionStr();
    String getGeneratedTime();
    String getCommitDateStr();
    void html_ver_get(AsyncWebServerRequest *request);

    // Веб-обработчики
    void handleInfo(AsyncWebServerRequest *request);
    void handleRead(AsyncWebServerRequest *request);
    void appendValues(String &values);
    String statusText() const;

    // I2C и логика датчика
    bool detect();
    bool initSensor();
    bool readStatus();
    bool readResults();
    bool updateEnvCompensation();
    bool readRegs(uint8_t reg, uint8_t *buf, uint8_t len);
    bool writeReg(uint8_t reg, uint8_t val);
    bool writeBytes(uint8_t reg, const uint8_t *buf, uint8_t len);
    void invalidate();
    void publish(bool available, int32_t eco2, int32_t tvoc);

    // EERTOS-задача
    static void pollTask();

protected:
    bool     _wireStarted;
    bool     _available;
    bool     _initialized;
    uint8_t  _addr;
    uint8_t  _hwId;
    uint8_t  _status;
    uint8_t  _errorId;
    int32_t  _eco2;
    int32_t  _tvoc;
    uint32_t _nextInitMs;
    uint32_t _lastEnvMs;
};

extern CLASS_MODULE_CCS811 module_ccs811;

#endif // _MODULE_CCS811_h
