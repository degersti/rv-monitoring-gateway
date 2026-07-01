#include "config.h"
#include <Arduino.h>   
#include "app/debug_logger.h"

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
    return digitalRead(PIN_SERIAL_DEBUG_ENABLE) == LOW;
}
/*************************************************
 * Function:    initSerialDebugDelayed
 * Description: Initializes serial debug output
 *              after a defined startup delay.
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
 * Function:    initSerialDebugNow
 * Description: Initializes serial debug output
 *              immediately without startup delay.
 *************************************************/
void initSerialDebugNow(void)
{
    Serial.begin(115200);
    Serial.println("==================");
    Serial.println("Debug Mode Active");
    Serial.println("==================");              
    serialInitialized = true;
}

void log(LogLevel level, const char* format, ...)
{
    if (!isDebugModeEnabled() || !serialInitialized)
    {
        return;
    }

    switch (level)
    {
        case LogLevel::Debug:
            Serial.print("[DEBUG] ");
            break;

        case LogLevel::Info:
            Serial.print("[INFO ] ");
            break;

        case LogLevel::Warning:
            Serial.print("[WARN ] ");
            break;

        case LogLevel::Error:
            Serial.print("[ERROR] ");
            break;
    }

    char buffer[256];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.println(buffer);
}
void logProgress(const char* msg)
{
    if (!isDebugModeEnabled() || !serialInitialized)
    {
        return;
    }

    Serial.print(msg);
}