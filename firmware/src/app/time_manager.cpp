/*************************************************
 * File:        time_manager.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Provides system time handling and NTP based
 * synchronization.
 *
 * Responsibilities:
 * - System time initialization
 * - NTP time synchronization
 * - Timestamp validation
 * - Timestamp reconstruction
 * - Deep Sleep persistent time state handling
 *
 *************************************************/
#include "config.h"
#include <Arduino.h>
#include "time_manager.h"
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"
#include "app/debug_logger.h"
/*************************************************
 * RTC-persistent time state
 *
 * These variables survive Deep Sleep but are
 * cleared after a complete power loss or reset.
 *************************************************/
RTC_DATA_ATTR static bool timeAvailable = false;
RTC_DATA_ATTR static bool relativeTimeInitialized = false;
RTC_DATA_ATTR static uint32_t lastNtpSyncTimestamp = 0;
RTC_DATA_ATTR static uint32_t relativeTimeAtLastSync = 0;
/*************************************************
 * Function:    configureNtp
 * Description: Configures the ESP32 SNTP client.
 * Parameters:  None
 * Returns:     None
 * Notes:       Uses the configured NTP server from
 *              config.h. The timezone offset is set
 *              to zero because all timestamps are
 *              handled as UTC.
 *************************************************/
static void configureNtp()
{
    configTime(0, 0, NTP_SERVER);
}
/*************************************************
 * Function:    waitForNtpSync
 * Description: Waits until the SNTP client has
 *              successfully synchronized the
 *              ESP32 system clock.
 * Parameters:  timeoutMs - Maximum wait time in
 *                          milliseconds
 * Returns:     true if synchronization completed,
 *              false if timeout expired
 * Notes:       Uses the ESP-IDF SNTP
 *              synchronization status.
 *************************************************/
static bool waitForNtpSync(uint32_t timeoutMs)
{
    uint32_t startTime = millis();

    while (millis() - startTime < timeoutMs)
    {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED)
        {
            return true;
        }

        delay(250);
    }

    return false;
}
/*************************************************
 * Function:    setSystemTime
 * Description: Sets the ESP32 system clock.
 * Parameters:  timestamp - Unix timestamp to set
 * Returns:     None
 * Notes:       Used to initialize the clock to zero
 *              before the first NTP synchronization.
 *************************************************/
static void setSystemTime(uint32_t timestamp)
{
    timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
}
/*************************************************
 * Function:    readSystemTimestamp
 * Description: Reads the ESP32 system timestamp.
 * Parameters:  None
 * Returns:     Current system timestamp
 * Notes:       Before NTP synchronization, this
 *              value is used as relative time.
 *              After NTP synchronization, it is
 *              used as Unix UTC time.
 *************************************************/
static uint32_t readSystemTimestamp()
{
    return static_cast<uint32_t>(time(nullptr));
}
/*************************************************
 * Function:    initTimeManager
 * Description: Initializes the Time Manager.
 * Parameters:  None
 * Returns:     None
 * Notes:       If no valid absolute time exists,
 *              the system clock is initialized to
 *              zero and used as relative time base.
 *************************************************/
void initTimeManager()
{
    // Setting system time to zero if no NTP sync 
    // possible after hard reset / powerloss
    LOG_INFO("Initializing system time");
    if (!timeAvailable && !relativeTimeInitialized)
    {
        LOG_INFO("Time status: RELATIVE");
        setSystemTime(0);
        relativeTimeInitialized = true;
    }
    else
    {
        LOG_INFO("Time status: ABSOLUT [source=RTC]");
    }
}
/*************************************************
 * Function:    isTimeValid
 * Description: Returns the current time validity
 *              state.
 * Parameters:  None
 * Returns:     true if absolute UTC time is
 *              available, false otherwise
 * Notes:       Time becomes valid after a successful
 *              NTP synchronization.
 *************************************************/
bool isTimeAvailable()
{
    return timeAvailable;
}
/*************************************************
 * Function:    forceTimeSync
 * Description: Performs an immediate NTP
 *              synchronization.
 * Parameters:  None
 * Returns:     true if synchronization succeeded,
 *              false otherwise
 * Notes:       The relative timestamp before NTP
 *              update is stored to support later
 *              timestamp reconstruction.
 *************************************************/
 bool forceTimeSync()
{
    uint32_t timeBeforeSync = readSystemTimestamp();

    sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);

    configureNtp();

   //if (!waitForValidSystemTime(NTP_SYNC_TIMEOUT_MS))
    if (!waitForNtpSync(NTP_SYNC_TIMEOUT_MS))
    {
        LOG_INFO("NTP synchronization: FAILED");
        return false;
    }

    lastNtpSyncTimestamp = readSystemTimestamp();
    relativeTimeAtLastSync = timeBeforeSync;
    timeAvailable = true;

    LOG_INFO("NTP synchronization: SUCCESS [previousSystemTime=%lu, NewSystemTime=%lu]",
                (unsigned long)timeBeforeSync,
                (unsigned long)lastNtpSyncTimestamp);
    LOG_INFO("Time status: ABSOLUT [source=NTP]"); 

    return true;
}
/*************************************************
 * Function:    isTimeSyncRequired
 * Description: Checks whether an NTP
 *              synchronization is required.
 * Parameters:  None
 * Returns:     true if synchronization is
 *              required, false otherwise
 * Notes:       Synchronization is required if no
 *              valid absolute time is available
 *              or the configured synchronization
 *              interval has elapsed.
 *************************************************/
bool isTimeSyncRequired()
{
    // No successful NTP synchronization has been
    // performed since power-up.
    if (!timeAvailable)
    {
        return true;
    }
    // Check whether the configured synchronization
    // interval has elapsed since the last NTP sync.
    return (readSystemTimestamp() - lastNtpSyncTimestamp) >= 
        NTP_SYNC_INTERVAL_SECONDS;
}
/*************************************************
 * Function:    getCurrentTimestamp
 * Description: Returns the current timestamp.
 * Parameters:  None
 * Returns:     Current timestamp value
 * Notes:       Before NTP synchronization, the value
 *              is relative time starting at zero.
 *              After NTP synchronization, the value
 *              is Unix UTC time.
 *************************************************/
uint32_t getCurrentTimestamp()
{
    return readSystemTimestamp();
}
/*************************************************
 * Function:    getLastNtpSyncTimestamp
 * Description: Returns the timestamp of the last
 *              successful NTP synchronization.
 * Parameters:  None
 * Returns:     Last NTP sync timestamp as Unix UTC
 * Notes:       Returns zero if no synchronization
 *              has been performed yet.
 *************************************************/
uint32_t getLastNtpSyncTimestamp()
{
    return lastNtpSyncTimestamp;
}
/*************************************************
 * Function:    reconstructTimestamp
 * Description: Reconstructs an absolute Unix UTC
 *              timestamp from a stored relative
 *              timestamp.
 * Parameters:  storedTimestamp - Relative timestamp
 *                                recorded before
 *                                the first
 *                                successful NTP
 *                                synchronization
 * Returns:     Reconstructed Unix UTC timestamp
 * Notes:       This function shall only be called
 *              after a successful NTP
 *              synchronization.
 *************************************************/
uint32_t reconstructTimestamp(uint32_t storedTimestamp)
{
    return lastNtpSyncTimestamp -
           (relativeTimeAtLastSync - storedTimestamp);
}