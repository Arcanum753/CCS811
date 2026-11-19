#include "core_web/FSWebServerLib.h"

#include <Arduino.h>
#include <stdio.h>
#include <string.h>

#include "module_ccs811.h"

#include "core_terminal/core_terminal.h"

// ============================================================
// Терминальные команды module_ccs811
// ============================================================

static const char* const kCcsCommands[] = {
    "show", nullptr
};

static const char kCcsHelp[] PROGMEM =
    "ccs - CCS811 air quality sensor (eCO2/TVOC)\r\n"
    "  ccs        show availability, address and readings\r\n"
    "  ccs show   same as 'ccs'\r\n"
    "  ccs help   this help\r\n"
    "Note: CCS811 needs 20-30 min burn-in after power-up.\r\n";

void CLASS_MODULE_CCS811::terminalRegister() {
    TerminalRegisterModule("module_ccs811", "ccs", "CCS811 air quality sensor",
                           &CLASS_MODULE_CCS811::s_terminalCmd,
                           kCcsCommands, kCcsHelp, false);
}

void CLASS_MODULE_CCS811::s_terminalCmd() {
    char* sub = term.getNext();
    if (sub == nullptr) {
        s_cmdShow();
        return;
    }
    if (strcmp(sub, "help") == 0) {
        TerminalPrintText(kCcsHelp);
        return;
    }

    if (strcmp(sub, "show") == 0) {
        s_cmdShow();
        return;
    }

    Serial.printf("Unknown ccs subcommand '%s'. Type 'ccs help'.\r\n", sub);
}

void CLASS_MODULE_CCS811::s_cmdShow() {
    if (term.getNext() != nullptr) {
        Serial.println("Usage: ccs [show]");
        return;
    }

    Serial.printf("\r\nCCS811 @0x%02X: %s\r\n",
                  module_ccs811._addr,
                  module_ccs811._available ? "available" : "not available");
    Serial.printf("Status:  %s\r\n", module_ccs811.statusText().c_str());

    if (!module_ccs811._available) { return; }

    Serial.printf("eCO2:    %ld ppm\r\n", (long)module_ccs811._eco2);
    Serial.printf("TVOC:    %ld ppb\r\n", (long)module_ccs811._tvoc);
}
