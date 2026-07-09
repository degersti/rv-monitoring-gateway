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
                    setState(ProgramState::COLLECT_DATA);
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
                    setState(ProgramState::COLLECT_DATA);
                    break;

                case MqttConnectionState::FAILED:
                    setIndicatorState(IndicatorState::MQTT_CONNECTING);
                    setState(ProgramState::COLLECT_DATA);
                    break;

                case MqttConnectionState::CONNECTING:
                    setIndicatorState(IndicatorState::MQTT_CONNECTING);
                    break;
            }
            break;
        }

        /*****************************************************
         * STATE: COLLECT_DATA
         * Collects all data required for the current
         * measurement cycle.
         *****************************************************/
        case ProgramState::COLLECT_DATA:
            LOG_INFO("Collecting data...");

            if (!updateData())
            {
                LOG_ERROR("Data update failed.");
                setState(ProgramState::ERROR);
                break;
            }

            if (isAlarmActive())
            {
                setIndicatorState(IndicatorState::ALARM_ACTIVE);
            }

            setState(ProgramState::PUBLISH_DATA);
            break;

        /*****************************************************
         * STATE: PUBLISH_DATA
         * Creates and publishes the telemetry payload.
         *****************************************************/
        case ProgramState::PUBLISH_DATA:

            if (getWiFiConnectionStatus() &&
                getMqttConnectionStatus() &&
                mqttPublish(getDeviceId(), getTelemetry()))
            {
                setState(ProgramState::WAIT_NEXT_CYCLE);
            }
            else
            {
                setState(ProgramState::BUFFER_DATA);
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

            if (!isAlarmActive())
            {
                setIndicatorState(IndicatorState::MQTT_ONLINE);
            }

            if (millis() - stateStartTime >= TELEMETRY_INTERVAL_MS)
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
                setState(ProgramState::COLLECT_DATA);
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
