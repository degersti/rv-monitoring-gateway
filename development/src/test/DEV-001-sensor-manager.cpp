#include <Arduino.h>
#include "app/debug_logger.h"
#include "app/hardware_manager.h"
#include "app/data_manager.h"
#include "app/sensor_manager.h"

static uint32_t lastReadTime = 0;
static constexpr uint32_t READ_INTERVAL_MS = 2000;
 MeasurementRecord record;

void setupDevSensorManager()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("========================================");
    Serial.println("DEV-001 Sensor Manager Test");
    Serial.println("========================================");

    initHardwareManager();
    bool initOk = initSensorManager();

    Serial.print("Hardware Init: ");
    Serial.println(initOk ? "OK" : "FAILED");

    Serial.println("----------------------------------------");
}

void loopDevSensorManager()
{
    if (millis() - lastReadTime < READ_INTERVAL_MS)
    {
        return;
    }

    lastReadTime = millis();



    readBatteryVoltages(record);
    readAlarmPins(record);
    readEnvironmentalValues(record);


    Serial.println();
    Serial.println("Sensor Data");
    Serial.println("----------------------------------------");

    Serial.print("House Battery : ");
    Serial.print(record.houseBatteryVoltage, 2);
    Serial.println(" V");

    Serial.print("Engine Battery: ");
    Serial.print(record.engineBatteryVoltage, 2);
    Serial.println(" V");

    Serial.print("Temperature   : ");
    Serial.print(record.temperature, 2);
    Serial.println(" °C");

    Serial.print("Humidity      : ");
    Serial.print(record.humidity, 2);
    Serial.println(" %");

    Serial.print("Water Alarm   : ");
    Serial.println(record.waterAlarm ? "ACTIVE" : "OK");

    Serial.print("Smoke Alarm   : ");
    Serial.println(record.smokeAlarm ? "ACTIVE" : "OK");
}