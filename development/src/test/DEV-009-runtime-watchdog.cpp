#include <Arduino.h>
#include "app/runtime_manager.h"

void setupDevRuntimeWatchdog()
{
    Serial.begin(115200);
    delay(200);
    initRuntimeManager();
    Serial.println("Runtime Watchdog initialized");
    Serial.print("Current boot epoch ID: ");
    Serial.println(getBootEpochId());
}

void loopDevRuntimeWatchdog()
{
    int sec = 0;
    Serial.print("Testing runtime watchdog");
    while(true)
    {
        delay(1000);
        Serial.print(".");
        sec = sec + 1;
    }

    feedRuntimeWatchdog();
}