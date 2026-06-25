#include "config.h"
#include <Arduino.h>   

// Serial debug initialization state
bool serialInitialized = false;
bool serialStarted = false;


/*************************************************
 * Function:    isSerialInitialized
 * Description: Returns whether serial debug output
 *              has already been initialized.
 *************************************************/
bool isSerialInitialized()
{
    return serialInitialized;
}

/*************************************************
 * Function:    isDebugModeEnabled
 * Description: Checks whether debug mode is
 *              requested via the serial debug pin.
 *************************************************/
bool isDebugModeEnabled(void)
{
    return digitalRead(SERIAL_DEBUG_PIN) == LOW;
}

/*************************************************
 * Function:    isAlarmActive
 * Description: Checks whether any alarm input is
 *              currently active.
 *************************************************/
bool isAlarmActive(void)
{
    return  digitalRead(WATER_ALARM_PIN) == LOW ||
            digitalRead(SMOKE_ALARM_PIN) == LOW;
}

/*************************************************
 * Function:    prepareDeepSleep
 * Description: Configures all wake-up sources
 *              before entering deep sleep.
 *************************************************/
void prepareDeepSleep(void)
{
    if(isDebugModeEnabled()){
        Serial.println("Preparing Deep Sleep");
    }
    esp_sleep_enable_ext1_wakeup(WAKEUP_PIN_MASK,ESP_EXT1_WAKEUP_ANY_LOW);
    esp_sleep_enable_timer_wakeup((uint64_t)CYCLE_INTERVAL_SEC * 1000000ULL);
}

/*************************************************
 * Function:    initSerialDebugDelayed
 * Description: Initializes serial debug output
 *              after a defined startup delay.
 *************************************************/
void enterDeepSleep(void)
{
    if(isDebugModeEnabled()){
        Serial.println("Entering Deep Sleep");
        Serial.flush();
    }
    esp_deep_sleep_start();
}

/*************************************************
 * Function:    initSerialDebugNow
 * Description: Initializes serial debug output
 *              immediately without startup delay.
 *************************************************/
void initSerialDebugDelayed(uint32_t startTime)
{
    if (!serialStarted)
    {
        Serial.begin(115200);
        serialStarted = true;
    }

    if (millis() - startTime > SERIAL_MONITOR_WAIT_MS)
    {    
        Serial.println("==================");
        Serial.println("Debug Mode Active");
        Serial.println("==================");              
        serialInitialized = true;
        serialStarted = false;
    }
}

/*************************************************
 * Function:    debugPrintWakeupReason
 * Description: Prints the ESP32 wake-up reason
 *              and alarm input states in debug
 *              mode.
 *************************************************/
void initSerialDebugNow(void)
{
    Serial.begin(115200);
    Serial.println("==================");
    Serial.println("Debug Mode Active");
    Serial.println("==================");              
    serialInitialized = true;
}


void debugPrintWakeupReason(void)
{
    if(!isDebugModeEnabled())
    {
        return;
    }
    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();
    Serial.print("WAKEUP REASON: ");
    switch (wakeupReason)
    {
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("EXT1 interrupt");
            Serial.print("WATER ALARM: ");
            Serial.println(digitalRead(WATER_ALARM_PIN) == LOW ? "active" : "inactive");
            Serial.print("SMOKE ALARM: ");
            Serial.println(digitalRead(SMOKE_ALARM_PIN) == LOW ? "active" : "inactive");
            break;

        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Timer interrupt");
            break;

        default:
            Serial.println("Boot / Reset");
            break;
    }
}
