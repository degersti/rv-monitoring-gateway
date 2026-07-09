#include "config.h"
#include <Arduino.h>   
#include "app/debug_logger.h"

// Serial debug initialization state
bool serialInitialized = false;
bool serialStarted = false;

/*************************************************
 * Function:    isSerialInitialized
 * Description: Returns whether the serial debug
 *              interface has been fully initialized
 *              and is ready for logging.
 * Parameters:  None
 * Returns:     true if serial logging is available
 * Note:        This function returns true only after
 *              the serial interface has been initialized
 *              and the startup delay has elapsed.
 *************************************************/
bool isSerialInitialized(void)
{
    return serialInitialized;
}
/*************************************************
 * Function:    isDebugModeEnabled
 * Description: Checks whether serial debug mode
 *              is enabled via the hardware debug
 *              input pin.
 * Parameters:  None
 * Returns:     true if debug mode is enabled
 * Note:        This function reads the state of the
 *              debug enable pin and returns true if
 *         it is pulled low.
 *************************************************/
bool isDebugModeEnabled(void)
{
    return digitalRead(PIN_SERIAL_DEBUG_ENABLE) == LOW;
}
/*************************************************
 * Function:    initSerialDebugDelayed
 * Description: Starts the serial interface and
 *              enables debug output after a
 *              configurable startup delay.
 * Parameters:  startTime - System startup time
 * Returns:     None
 * Note:        This function should be called repeatedly
 *              in the main loop until serial debug
 *              output is initialized.
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
 * Description: Immediately initializes the serial
 *              debug interface without waiting
 *              for the startup delay.
 * Parameters:  None
 * Returns:     None
 * Note:        This function initializes the serial 
 *              debug interface immediately, 
 *              bypassing any startup delay. Used
 *              when deep-sleep is disabled.
 *************************************************/
void initSerialDebugNow(void)
{
    Serial.begin(115200);
    Serial.println("==================");
    Serial.println("Debug Mode Active");
    Serial.println("==================");              
    serialInitialized = true;
}

/*************************************************
 * Function:    log
 * Description: Writes a formatted log message to
 *              the serial debug interface.
 * Parameters:  level  - Log message severity
 *              format - Printf-style format string
 *              ...    - Optional format arguments
 * Returns:     None
 * Notes:       Logging is ignored when debug mode
 *              is disabled or the serial interface
 *              has not yet been initialized.
 *************************************************/
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
            Serial.print("[INFO] ");
            break;

        case LogLevel::Warning:
            Serial.print("[WARN] ");
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

/*************************************************
 * Function:    logProgress
 * Description: Writes a progress message to the
 *              serial debug interface without
 *              appending a newline.
 * Parameters:  msg - Progress text to output
 * Returns:     None
 * Notes:       Logging is ignored when debug mode
 *              is disabled or the serial interface
 *              has not yet been initialized.
 *************************************************/
void logProgress(const char* msg)
{
    if (!isDebugModeEnabled() || !serialInitialized)
    {
        return;
    }

    Serial.print(msg);
}