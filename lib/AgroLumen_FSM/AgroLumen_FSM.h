#ifndef AGROLUMEN_FSM_H
#define AGROLUMEN_FSM_H

#include <Arduino.h>

enum class SystemState : uint8_t {
    MONITORING,
    MENU_SELECT,
    MANUAL_CONFIG,
    CLOCK_CONFIG,
    HISTORY_VIEW,
    ALARM_TRIGGERED
};

#endif // AGROLUMEN_FSM_H