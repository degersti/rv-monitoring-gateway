#pragma once

#include <Arduino.h>

enum class IndicatorState
{
    BOOT,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    MQTT_CONNECTING,
    MQTT_CONNECTED,
    ALARM_ACTIVE,
    ERROR_ACTIVE,
    OFF
};

void initStatusIndicator(void);
void setIndicatorState(IndicatorState state);
void updateStatusIndicator(void);
void triggerTelemetryFlash(void);