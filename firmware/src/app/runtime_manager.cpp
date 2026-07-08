/*************************************************
 * File:        runtime_manager.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Manages runtime context including boot epoch 
 * tracking and alarm input monitoring.
 *
 * Responsibilities:
 * - Boot epoch ID management
 * - Alarm input state monitoring
 * - Deep sleep preparation and wake-up handling 
 *************************************************/
#include "config.h"
#include <Arduino.h>   
#include <Preferences.h>
#include "esp_task_wdt.h"
#include "esp_idf_version.h"
#include "app/debug_logger.h"


// RTC initialization marker and current boot epoch
RTC_DATA_ATTR static uint32_t rtcMagic = 0;
RTC_DATA_ATTR static uint32_t bootEpochId = 0;
// Valid RTC marker.
constexpr uint32_t RTC_MAGIC = 0x52564D47;   // "RVMG"
// Non-volatile storage instance.
static Preferences preferences;
// Unique identification of the ESP32 device.
static char deviceId[13];

/*************************************************
 * Function:    readBootEpochIdFromFlash
 * Description: Reads the current boot epoch ID
 *              from non-volatile storage.
 * Parameters:  None
 * Returns:     Stored boot epoch ID.
 * Notes:       Returns 0 if no value is stored.
 *************************************************/
static uint32_t readBootEpochIdFromFlash(void)
{
    preferences.begin("runtime", true);

    uint32_t bootEpochId = preferences.getUInt("bootEpochId", 0);

    preferences.end();

    return bootEpochId;
}
/*************************************************
 * Function:    writeBootEpochIdToFlash 
 * Description: Stores the current boot epoch ID
 *              in non-volatile storage.
 * Parameters:  bootEpochId - Boot epoch ID
 * Returns:     None
 * Notes:       Overwrites the previously stored
 *              boot epoch ID.
 *************************************************/
static void writeBootEpochIdToFlash(uint32_t bootEpochId)
{
    preferences.begin("runtime", false);

    preferences.putUInt("bootEpochId", bootEpochId);

    preferences.end();
}
/*************************************************
 * Function:    loadDeviceId
 * Description: Reads the unique ESP32 hardware ID
 *              from eFuse and stores it as the
 *              runtime device identifier.
 * Parameters:  None
 * Returns:     None    
 * Notes:       The device ID is a 12-character
 *              hexadecimal string derived from the
 *              ESP32 MAC address.
 *************************************************/
static void loadDeviceId()
{
    uint64_t mac = ESP.getEfuseMac();

    snprintf(deviceId,
             sizeof(deviceId),
             "%04X%08X",
             (uint16_t)(mac >> 32),
             (uint32_t)mac);

    LOG_INFO("Device ID: %s", deviceId);
}

/*************************************************
 * Function:    initWatchdog
 * Description: Initializes the ESP32 hardware
 *              watchdog and registers the current
 *              task for supervision.
 * Parameters:  None
 * Returns:     None
 * Notes:       Must be called once during 
 *              application initialization.   
 *************************************************/
void initWatchdog(void)
{
#if ESP_IDF_VERSION_MAJOR >= 5
    esp_task_wdt_config_t watchdogConfig =
    {
        .timeout_ms = WATCHDOG_TIMEOUT_SECONDS * 1000,
        .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
        .trigger_panic = true
    };

    esp_err_t result = esp_task_wdt_init(&watchdogConfig);
#else
    esp_err_t result = esp_task_wdt_init(WATCHDOG_TIMEOUT_SECONDS, true);
#endif

    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE)
    {
        LOG_ERROR("Failed to initialize watchdog (%d)", result);
        return;
    }

    result = esp_task_wdt_add(NULL);

    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE)
    {
        LOG_ERROR("Failed to register watchdog task (%d)", result);
        return;
    }

    LOG_INFO("Runtime watchdog initialized.");
}
/*************************************************
 * Function:    initRuntimeManager (added 30.06.26) 
 * Description: Initializes the runtime context.
 * Parameters:  None
 * Returns:     None
 * Notes:       Restores the current boot epoch
 *              from Flash or creates a new one
 *              if the RTC context is invalid.
 *************************************************/
void initRuntimeManager(void)
{
    if (rtcMagic != RTC_MAGIC)
    {
        rtcMagic = RTC_MAGIC;

        bootEpochId = readBootEpochIdFromFlash();
        bootEpochId++;

        writeBootEpochIdToFlash(bootEpochId);
    }
    loadDeviceId();
}
/*************************************************
 * Function:    getBootEpochId
 * Description: Returns the current boot epoch ID.
 * Parameters:  None
 * Returns:     Current boot epoch ID.
 * Notes:       None
 *************************************************/
uint32_t getBootEpochId(void)
{
    return bootEpochId;
}
/*************************************************
 * Function:    feedRuntimeWatchdog
 * Description: Resets the watchdog timer after a
 *              successful application cycle to
 *              prevent an unintended system reset.
 * Parameters:  None
 * Returns:     None
 * Notes:       Must be called at the end of each
 *              successful application cycle.
 *************************************************/
const char* getDeviceId()
{
    return deviceId;
}
/*************************************************
 * Function:    getDeviceId
 * Description: Returns the unique device
 *              identifier of the gateway.
 * Parameters:  None
 * Returns:     Pointer to a null-terminated string
 *              containing the device ID.
 * Notes:       The device ID is derived from the
 *              ESP32 MAC address and is unique to
 *              each device.
 *************************************************/
void feedRuntimeWatchdog(void)
{
    esp_err_t result = esp_task_wdt_reset();

    if (result != ESP_OK)
    {
        LOG_ERROR("Failed to feed watchdog (%d)", result);
    }
}
/*************************************************
 * Function:    isAlarmActive
 * Description: Checks whether any alarm input is
 *              currently active.
 * Parameters:  None
 * Returns:     true  - At least one alarm is active   
 *              false - No alarms are active
 * Notes:       Alarm inputs are active LOW.
 *************************************************/
bool isAlarmActive(void)
{
    return  digitalRead(PIN_WATER_ALARM) == LOW ||
            digitalRead(PIN_SMOKE_ALARM) == LOW;
}

/*************************************************
 * Function:    prepareDeepSleep
 * Description: Configures all wake-up sources
 *              before entering deep sleep.
 * Parameters:  None
 * Returns:     None
 * Notes:       Configures both alarm inputs and
 *              the periodic timer as wake-up 
 *              sources.
 *************************************************/
void prepareDeepSleep(void)
{
    LOG_INFO("Preparing Deep Sleep");
    esp_sleep_enable_ext1_wakeup(WAKEUP_PIN_MASK,ESP_EXT1_WAKEUP_ANY_LOW);
    esp_sleep_enable_timer_wakeup((uint64_t)CYCLE_INTERVAL_SEC * 1000000ULL);
}


void enterDeepSleep(void)
{

    LOG_INFO("Entering Deep Sleep");
    esp_deep_sleep_start();
}
/*************************************************
 * Function:    debugPrintWakeupReason
 * Description: Prints the ESP32 wake-up reason
 *              and alarm input states in debug
 *              mode.
 * Parameters:  None
 * Returns:     None
 * Notes:       Should be called after wake-up to
 *              provide context for the current
 *              boot cycle.
 *************************************************/
void printWakeupReason(void)
{
    esp_sleep_wakeup_cause_t wakeupReason = esp_sleep_get_wakeup_cause();

    switch (wakeupReason)
    {
        case ESP_SLEEP_WAKEUP_EXT1:
            LOG_INFO(
                "WAKEUP REASON: EXT1 interrupt | WATER ALARM: %s | SMOKE ALARM: %s",
                digitalRead(PIN_WATER_ALARM) == LOW ? "active" : "inactive",
                digitalRead(PIN_SMOKE_ALARM) == LOW ? "active" : "inactive"
            );
            break;

        case ESP_SLEEP_WAKEUP_TIMER:
            LOG_INFO("WAKEUP REASON: Timer interrupt");
            break;

        default:
            LOG_INFO("WAKEUP REASON: Boot / Reset");
            break;
    }
}




