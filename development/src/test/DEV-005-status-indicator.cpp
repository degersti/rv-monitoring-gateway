#include <Arduino.h>
#include "app/status_indicator.h"

static uint32_t lastStateChangeTime = 0;
static uint8_t testStep = 0;

static const uint32_t STATE_DURATION_MS = 5000;

void setupDevStatusIndicator()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("DEV-005 Status Indicator Test");
    Serial.println("--------------------------------");

    initStatusIndicator();

    setIndicatorState(IndicatorState::BOOT);
    lastStateChangeTime = millis();

    Serial.println("State: BOOT");
}

void loopDevStatusIndicator()
{
    updateStatusIndicator();

    uint32_t now = millis();

    if (now - lastStateChangeTime < STATE_DURATION_MS)
    {
        return;
    }

    lastStateChangeTime = now;
    testStep++;

    switch (testStep)
    {
        case 1:
            Serial.println("State: WIFI_CONNECTING");
            setIndicatorState(IndicatorState::WIFI_CONNECTING);
            break;

        case 2:
            Serial.println("State: WIFI_CONNECTED");
            setIndicatorState(IndicatorState::WIFI_CONNECTED);
            break;

        case 3:
            Serial.println("State: MQTT_CONNECTING");
            setIndicatorState(IndicatorState::MQTT_CONNECTING);
            break;

        case 4:
            Serial.println("State: MQTT_CONNECTED");
            setIndicatorState(IndicatorState::MQTT_ONLINE);
            break;

        case 5:
            Serial.println("Trigger telemetry flash");
            triggerTelemetryFlash();
            break;

        case 6:
            Serial.println("State: ALARM_ACTIVE");
            setIndicatorState(IndicatorState::ALARM_ACTIVE);
            break;

        case 7:
            Serial.println("State: ERROR_ACTIVE");
            setIndicatorState(IndicatorState::ERROR_ACTIVE);
            break;

        case 8:
            Serial.println("State: OFF");
            setIndicatorState(IndicatorState::OFF);
            break;

        default:
            Serial.println("Restart test sequence");
            testStep = 0;
            setIndicatorState(IndicatorState::BOOT);
            break;
    }
}