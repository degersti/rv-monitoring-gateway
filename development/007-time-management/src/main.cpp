#include <Arduino.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sleep.h"

RTC_DATA_ATTR uint32_t lastNtpSyncTimestamp = 0;
RTC_DATA_ATTR uint32_t wakeupCounter = 0;

constexpr uint64_t SLEEP_TIME_S = 10;
constexpr uint32_t TEST_TIMESTAMP = 1700000000UL;

void printWakeupReason()
{
    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();

    switch (wakeupReason)
    {
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup Reason        : TIMER");
            break;

        default:
            Serial.printf("Wakeup Reason        : %d\n", wakeupReason);
            break;
    }
}

void setTestSystemTime(uint32_t timestamp)
{
    timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;

    settimeofday(&tv, nullptr);
}

void setup()
{
    Serial.begin(115200);
    delay(10000);

    wakeupCounter++;

    Serial.println();
    Serial.println("================================");
    Serial.println("ESP32 RTC Time Test");
    Serial.println("================================");

    Serial.printf("Wakeup Counter       : %lu\n", wakeupCounter);
    printWakeupReason();

    if (lastNtpSyncTimestamp == 0)
    {
        Serial.println("First start detected");
        Serial.println("Setting test system time...");

        setTestSystemTime(TEST_TIMESTAMP);
        lastNtpSyncTimestamp = TEST_TIMESTAMP;
    }

        Serial.printf("Last NTP Sync        : %lu\n", lastNtpSyncTimestamp);
    Serial.printf("Current Timestamp    : %lld\n", static_cast<long long>(now));
    Serial.printf("Elapsed Since Sync   : %lld s\n",
                  static_cast<long long>(now) - lastNtpSyncTimestamp);

    time_t now = time(nullptr);
    
    Serial.println();
    Serial.printf("Going to Deep Sleep for %llu seconds...\n", SLEEP_TIME_S);

    delay(2000);

    esp_sleep_enable_timer_wakeup(SLEEP_TIME_S * 1000000ULL);
    esp_deep_sleep_start();

}

void loop()
{

    
}