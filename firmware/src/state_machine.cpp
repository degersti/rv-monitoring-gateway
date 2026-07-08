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
#include "app/wifi_manager.h"
#include "app/mqtt_client.h"
#include "app/data_manager.h"
#include "app/status_indicator.h"

// Timestamp of last telemetry transmission
static unsigned long lastTelemetry = 0;

// Temporary JSON payload buffer
static char payload[TELEMETRY_PAYLOAD_SIZE];

// Current sensor and alarm values
static SensorData data = {
    .houseBatteryVoltage = -999.0f,
    .engineBatteryVoltage = -999.0f,
    .temperature = -999.0f,
    .humidity = -999.0f,
    .waterAlarm = false,
    .smokeAlarm = false
};

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
            setState(ProgramState::INIT_HARDWARE);
            break;

        /*****************************************************
         * STATE: INIT_HARDWARE
         * Initializes all hardware peripherals and sensors.
         *****************************************************/
        case ProgramState::INIT_HARDWARE:
            initWifi();
            initMqtt(getWifiClient());
            if (initSensorManager())
            {
                state = ProgramState::CONNECT_WIFI;
            }
            else
            {
                state = ProgramState::ERROR;
            }
            break;

        /*****************************************************
         * STATE: CONNECT_WIFI
         * Non-blocking WiFi connection handling.
         *****************************************************/
        case ProgramState::CONNECT_WIFI:
        {
            WiFiConnectionState wifiState = processWifiConnection();

            if (wifiState == WiFiConnectionState::CONNECTED)
            {
                setIndicatorState(IndicatorState::WIFI_CONNECTED);
                state = ProgramState::CONNECT_MQTT;
            }
            else if (wifiState == WiFiConnectionState::FAILED)
            {
                setIndicatorState(IndicatorState::WIFI_CONNECTING);
                state = ProgramState::BUFFER_DATA;
            }
            else
            {
                setIndicatorState(IndicatorState::WIFI_CONNECTING);
            }
            break;
        }

        /*****************************************************
         * STATE: CONNECT_MQTT
         * MQTT connection handling with retry timing.
         *****************************************************/
        case ProgramState::CONNECT_MQTT:
        {
            if (!getWiFiConnectionStatus())
            {
                resetMqttConnection();
                state = ProgramState::CONNECT_WIFI;
                break;
            }

            MqttConnectionState mqttState = processMqttConnection(getDeviceId());

            if (mqttState == MqttConnectionState::CONNECTED)
            {
                setIndicatorState(IndicatorState::MQTT_ONLINE);
                state = ProgramState::READ_SENSORS;
            }
            else if (mqttState == MqttConnectionState::FAILED)
            {
                setIndicatorState(IndicatorState::MQTT_CONNECTING);
                state = ProgramState::BUFFER_DATA;
            }
            else
            {
                setIndicatorState(IndicatorState::MQTT_CONNECTING);
            }
            break;
        }

        /*****************************************************
         * STATE: READ_SENSORS
         * Acquires the latest sensor and alarm values.
         *****************************************************/
        case ProgramState::READ_SENSORS:
            readBatteryVoltages(data);
            readSHT31(data);
            readAlarms(data);

            if (isAlarmActive())
            {
                setIndicatorState(IndicatorState::ALARM_ACTIVE);
            }

            state = ProgramState::PUBLISH_DATA;
            break;

        /*****************************************************
         * STATE: PUBLISH_DATA
         * Creates and publishes the telemetry payload.
         *****************************************************/
        case ProgramState::PUBLISH_DATA:
            if (!getWiFiConnectionStatus())
            {
                resetMqttConnection();
                state = ProgramState::CONNECT_WIFI;
                break;
            }

            if (!getMqttConnectionStatus())
            {
                state = ProgramState::CONNECT_MQTT;
                break;
            }

            mqttLoop();
            getTelemetry(payload, data);

            if (mqttPublish(getDeviceId(), payload))
            {
                if (!isAlarmActive())
                {
                    setIndicatorState(IndicatorState::MQTT_ONLINE);
                }

                lastTelemetry = millis();
                state = ProgramState::WAIT_NEXT_CYCLE;
            }
            else
            {
                state = ProgramState::CONNECT_MQTT;
            }
            break;

        /*****************************************************
         * STATE: BUFFER_DATA
         * Stores telemetry data locally while network
         * communication is unavailable.
         *****************************************************/
        case ProgramState::BUFFER_DATA:
            /* NO CURRENT IMPLEMENTATION */
            state = ProgramState::WAIT_NEXT_CYCLE;
            break;

        /*****************************************************
         * STATE: WAIT_NEXT_CYCLE
         * Idle state between telemetry transmissions.
         * Maintains active MQTT connections and waits for
         * the next measurement interval.
         *****************************************************/
        case ProgramState::WAIT_NEXT_CYCLE:
            if (!getWiFiConnectionStatus())
            {
                resetMqttConnection();
                state = ProgramState::CONNECT_WIFI;
                break;
            }

            if (!getMqttConnectionStatus())
            {
                state = ProgramState::CONNECT_MQTT;
                break;
            }

            mqttLoop();

            if (!isAlarmActive())
            {
                setIndicatorState(IndicatorState::MQTT_ONLINE);
            }

            if (millis() - lastTelemetry >= TELEMETRY_INTERVAL_MS)
            {
                state = ProgramState::READ_SENSORS;
            }
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
