#include <Arduino.h>
#include "esp_sleep.h"
#include "config.h"
#include "state_machine.h"
#include "status_indicator.h"


void setup()
{
    pinMode(SERIAL_DEBUG_PIN, INPUT_PULLUP);
    pinMode(WATER_ALARM_PIN, INPUT);
    pinMode(SMOKE_ALARM_PIN, INPUT);
    initStatusIndicator();
    initStateMachine();
}
void loop()
{
    runStateMachine();
    updateStatusIndicator();
}