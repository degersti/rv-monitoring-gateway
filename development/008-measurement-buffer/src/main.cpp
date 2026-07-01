#include <Arduino.h>
#include "esp_sleep.h"
#include "config.h"
#include "runtime_manager.h"
#include "time_manager.h"
#include "measurement_buffer.h"

int cnt = 1;
MeasurementRecord record;

void printMeasurementRecord(const MeasurementRecord& record)
{
    Serial.println("----------------------------------------");
    Serial.printf("Boot Epoch ID : %lu\n", record.bootEpochId);
    Serial.printf("Timestamp     : %lu\n", record.timestamp);
    Serial.printf("House Battery : %.2f V\n", record.houseBatteryVoltage);
    Serial.printf("Engine Battery: %.2f V\n", record.engineBatteryVoltage);
    Serial.printf("Temperature   : %.2f °C\n", record.temperature);
    Serial.printf("Humidity      : %.2f %%\n", record.humidity);
    Serial.printf("Water Alarm   : %s\n", record.waterAlarm ? "YES" : "NO");
    Serial.printf("Smoke Alarm   : %s\n", record.smokeAlarm ? "YES" : "NO");
    Serial.println("----------------------------------------");
}

void setup()
{

    Serial.begin(115200);
    delay(2000);

    pinMode(1, INPUT_PULLUP);
    Serial.print("LEVEL PIN 1: ");
    Serial.println(digitalRead(1));

    initRuntimeManager();
    initTimeManager();
    bool state = initBuffer();
    Serial.print("Buffer intitialisation Status: ");
    Serial.println(state);
}
void loop()
{
    Serial.println(cnt);

    if(cnt <= 5 && (digitalRead(1) == LOW))
    {
        
        record.bootEpochId = getBootEpochId();
        record.timestamp = getCurrentTimestamp();
        record.houseBatteryVoltage = 12.6f;
        record.engineBatteryVoltage = 12.8f;
        record.temperature = 21.5f;
        record.humidity = 55.0f;
        record.waterAlarm = false;
        record.smokeAlarm = false;
        pushRecord(record);
    }
    /*
    if (cnt == 6 || cnt == 7)
    {
        removeOldestRecord();
    }
    if (cnt == 8 || cnt == 9)
    {
        
        record.bootEpochId = getBootEpochId();
        record.timestamp = getCurrentTimestamp();
        record.houseBatteryVoltage = 12.6f;
        record.engineBatteryVoltage = 12.8f;
        record.temperature = 21.5f;
        record.humidity = 55.0f;
        record.waterAlarm = false;
        record.smokeAlarm = false;
        pushRecord(record);
    }
    */
    int recordCount = getRecordCount();
    if(cnt ==10 && (digitalRead(1) == HIGH))
    {
        Serial.print("Record Count: ");
        Serial.println(recordCount);
        for(int i = 0; i < recordCount; i++)
        {
            readOldestRecord(record);
            printMeasurementRecord(record);
            removeOldestRecord();
        }
    }
    if(cnt == 12 && (digitalRead(1) == HIGH))
    {
        Serial.print("Overflow Count: ");
        Serial.println(getOverflowCount());
        Serial.print("Reset Overflow Count status: ");
        Serial.println(resetOverflowCount());
        Serial.print("Overflow Count: ");
        Serial.println(getOverflowCount());
    }
    

    cnt = cnt + 1;

    delay(2000);
}