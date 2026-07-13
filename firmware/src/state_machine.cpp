/*****************************************************
 * Module: State Machine
 *
 * Controls the application flow:
 *
 * BOOT
 *   -> INIT_HARDWARE
 *   -> CONNECT_WIFI
 *   -> CONNECT_MQTT
 *   -> READ_SENSORS
 *   -> PUBLISH_DATA
 *   -> WAIT_NEXT_CYCLE
 *
 * Error conditions may transition to:
 *   -> BUFFER_DATA
 *   -> ERROR
 *****************************************************/

#include <Arduino.h>
#include "config.h"
#include "state_machine.h"
#include "app/runtime_manager.h"
#include "app/debug_logger.h"
#include "app/sensor_manager.h"
#include "app/data_manager.h"
#include "app/time_manager.h"
#include "app/measurement_buffer.h"
#include "app/wifi_manager.h"
#include "app/mqtt_client.h"
#include "app/data_manager.h"
#include "app/status_indicator.h"

// Current program state
ProgramState state; 
PublishSource publishSource;
// Timestamp of current state entry
uint32_t stateStartTime;

/*************************************************
 * Function:    initStateMachine
 * Description: Initializes the program state
 *              machine and enters the BOOT state.
 * Parameters:  None
 * Returns:     None (void function)
 * Notes:       Must be called once during system
 *              startup.
 *************************************************/
void initStateMachine()
{
    state = ProgramState::BOOT;
    publishSource = PublishSource::CURRENT_MEASUREMENT;
    stateStartTime = millis();
}
/*************************************************
 * Function:    setState
 * Description: Changes the current program state
 *              and updates the state entry
 *              timestamp if the state changed.
 * Parameters:  newState - New program state
 * Returns:     None (void function)
 * Notes:       Should be called to transition between
 *              program states.
 *************************************************/
void setState(ProgramState newState)
{
    if (state != newState)
    {
        state = newState;
        stateStartTime = millis();
    }
}
/*************************************************
 * Function:    runStateMachine
 * Description: Executes the current program state
 *              and performs state transitions.
 * Parameters:  None
 * Returns:     None (void function)
 * Notes:       Must be called cyclically from the
 *              main application loop to ensure 
 *              proper state machine operation.
 *************************************************/
void runStateMachine()
{
    switch (state)
    {
        /*****************************************************
         * STATE: BOOT
         * Entry state after reset or power-up.
         *****************************************************/
        case ProgramState::BOOT:
            setIndicatorState(IndicatorState::BOOT);
            if(isDebugModeEnabled() && !isSerialInitialized())
            {                 
                initSerialDebugDelayed(stateStartTime);  
                break;   
            } 
            if (isDebugModeEnabled())
            {
                printWakeupReason();
            }
            setState(ProgramState::INIT_SYSTEM);
            break;

        /*****************************************************
         * STATE: INIT_SYSTEM
         * Initializes all system modules and peripherals.
         *****************************************************/
        case ProgramState::INIT_SYSTEM:
            LOG_INFO("Initializing system modules");
            initTimeManager();
            if(!initBuffer())
            {
                LOG_ERROR("Measurement buffer initialization failed");
                setState(ProgramState::ERROR);
                break;
            }
            initWifi();
            initMqtt(getWifiClient());
            if (initSensorManager())
            {
                setState(ProgramState::CONNECT_WIFI);
            }
            else
            {
                setState(ProgramState::ERROR);
            }
            if(isAlarmActive)
            {
                setIndicatorState(IndicatorState::ALARM_ACTIVE);
            }
            break;
        /*****************************************************
         * STATE: CONNECT_WIFI
         * Non-blocking WiFi connection handling.
         *****************************************************/
        case ProgramState::CONNECT_WIFI:
        {
            WiFiConnectionState wifiState = processWifiConnection();

            switch (wifiState)
            {
                case WiFiConnectionState::CONNECTED:
                    setIndicatorState(IndicatorState::WIFI_CONNECTED);
                    if(isTimeSyncRequired())
                    {
                        setState(ProgramState::SYNC_TIME);
                    }
                    else
                    {
                        setState(ProgramState::CONNECT_MQTT);
                    }
                    break;

                case WiFiConnectionState::FAILED:
                    setIndicatorState(IndicatorState::WIFI_CONNECTING);
                    setState(ProgramState::CREATE_RECORD);
                    break;

                case WiFiConnectionState::CONNECTING:
                    setIndicatorState(IndicatorState::WIFI_CONNECTING);
                    break;
            }
            break;
        }

        /*****************************************************
         * STATE: CONNECT_MQTT
         * MQTT connection handling with retry timing.
         *****************************************************/
        case ProgramState::CONNECT_MQTT:
        {
            MqttConnectionState mqttState = processMqttConnection(getDeviceId());

            switch (mqttState)
            {
                case MqttConnectionState::CONNECTED:
                    setIndicatorState(IndicatorState::MQTT_ONLINE);
                    setState(ProgramState::CREATE_RECORD);
                    break;

                case MqttConnectionState::FAILED:
                    setIndicatorState(IndicatorState::MQTT_CONNECTING);
                    setState(ProgramState::CREATE_RECORD);
                    break;

                case MqttConnectionState::CONNECTING:
                    setIndicatorState(IndicatorState::MQTT_CONNECTING);
                    break;
            }
            break;
        }
        /*****************************************************
         * STATE: CREATE_RECORD
         * Collects sensor data and updates the current 
         * measurement record.
         *****************************************************/
        case ProgramState::CREATE_RECORD:

            LOG_INFO("Collecting data...");
            if (!updateData())
            {
                LOG_ERROR("Data update failed.");
                setState(ProgramState::ERROR);
                break;
            }
            else
            {
                publishSource = PublishSource::CURRENT_MEASUREMENT;
            }

            if(isTimeAvailable())
            {
                setState(ProgramState::PUBLISH_DATA);
            }
            else
            {
                setState(ProgramState::BUFFER_DATA);
            }
            break;
        /*****************************************************
         * STATE: LOAD_BUFFERED_RECORD
         * Loads and validates the oldest buffered measurement
         * and prepares its telemetry payload for publication.
         *****************************************************/
        case ProgramState::LOAD_BUFFERED_RECORD:

            // Load the oldest buffered record.
            // Leave if the buffer is empty.
            if (!readOldestRecord(getCurrentData()))
            {
                setState(ProgramState::WAIT_NEXT_CYCLE);
                break;
            }
            //
            switch (checkValidity())
            {
                case RecordValidity::VALID:
                    publishSource = PublishSource::BUFFERED_RECORD;
                    setState(ProgramState::PUBLISH_DATA);
                    break;

                case RecordValidity::KEEP:
                    setState(ProgramState::WAIT_NEXT_CYCLE);
                    break;

                case RecordValidity::DISCARD:
                    LOG_WARN("Buffered record cannot be reconstructed. Discarding.");
                    // Invalid records must be removed, otherwise
                    // they would block the buffer permanently
                    removeOldestRecord();
                    setState(ProgramState::LOAD_BUFFERED_RECORD);
                    break;
            }
            break;
        /*****************************************************
        * STATE: PUBLISH_DATA
        * Publishes the currently prepared telemetry payload.
        * After a successful publish, buffered records are
        * removed and the next available record is prepared.
        *****************************************************/
        case ProgramState::PUBLISH_DATA:

            // Verify connectivity and attempt MQTT publication
            if (getWiFiConnectionStatus() &&
                getMqttConnectionStatus() &&
                mqttPublish(getDeviceId(), getTelemetry()))
            {
                LOG_INFO("Telemetry data published successfully.");

                // Remove the record only if the published data
                // originated from the persistent buffer
                if (publishSource == PublishSource::BUFFERED_RECORD)
                {
                    LOG_INFO("Removing published record from buffer.");
                    removeOldestRecord();
                }

                // Publish the next buffered record, if available
                if (!isBufferEmpty())
                {
                    setState(ProgramState::LOAD_BUFFERED_RECORD);
                }
                else
                {
                    setState(ProgramState::WAIT_NEXT_CYCLE);
                }
            }
            else
            {
                LOG_WARN("Failed to publish telemetry data.");

                // Store a new measurement if its initial publish failed
                if (publishSource == PublishSource::CURRENT_MEASUREMENT)
                {
                    setState(ProgramState::BUFFER_DATA);
                }
                else
                {
                    // The historical record remains in the buffer
                    // and can be retried during the next cycle
                    setState(ProgramState::WAIT_NEXT_CYCLE);
                }
            }
            break;

        /*****************************************************
         * STATE: BUFFER_DATA
         * Stores telemetry data locally while network
         * communication is unavailable.
         *****************************************************/
        case ProgramState::BUFFER_DATA:
            pushRecord(getCurrentData());
            setState(ProgramState::WAIT_NEXT_CYCLE);
            break;

        /*****************************************************
         * STATE: WAIT_NEXT_CYCLE
         * Idle state between telemetry transmissions.
         * Maintains active MQTT connections and waits for
         * the next measurement interval.
         *****************************************************/
        case ProgramState::WAIT_NEXT_CYCLE:

            mqttLoop();

            if (millis() - stateStartTime >= (CYCLE_INTERVAL_SEC * 1000UL))
            {
                if (!getWiFiConnectionStatus())
                {
                    disconnectMqtt();
                    setState(ProgramState::CONNECT_WIFI);
                    break;
                }

                if (!getMqttConnectionStatus())
                {
                    setState(ProgramState::CONNECT_MQTT);
                    break;
                }
                setState(ProgramState::CREATE_RECORD);
            }
            break;
        /*****************************************************
         * STATE: SYNC_TIME
         * Synchronizes the system clock with an NTP server.
         *****************************************************/
        case ProgramState::SYNC_TIME:
            LOG_INFO("Synchronizing system time with NTP server");
            if (forceTimeSync())
            {
                LOG_INFO("NTP synchronization successful");   
            }
            else
            {
                LOG_WARN("NTP synchronization failed");
            }
            setState(ProgramState::CONNECT_MQTT);
            break;
        /*****************************************************
         * STATE: ERROR
         * Critical system error.
         *****************************************************/
        case ProgramState::ERROR:
            setIndicatorState(IndicatorState::ERROR_ACTIVE);
            break;
    }
}
