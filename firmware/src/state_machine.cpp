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
#include "sensor_manager.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "data_manager.h"

// Timestamp of last telemetry transmission
static unsigned long lastTelemetry = 0;

// Temporary JSON payload buffer
static  char payload[TELEMETRY_PAYLOAD_SIZE];

// Current sensor and alarm values
static SensorData data = {
    .houseBatteryVoltage = -999.0f,
    .engineBatteryVoltage = -999.0f,
    .temperature = -999.0f,
    .humidity = -999.0f,
    .waterAlarm = false,
    .smokeAlarm = false
};

// Application starts in BOOT state after power-up
static ProgramState state = ProgramState::BOOT;

void runStateMachine() {
    switch (state) {
       /*****************************************************
        * STATE: BOOT
        * Entry state after reset or power-up.
        * Performs basic startup handling before hardware 
        * initialization.
        *****************************************************/
        case ProgramState::BOOT:
            Serial.println("BOOT");
            state = ProgramState::INIT_HARDWARE;
            break;
        /*****************************************************
        * STATE: INIT_HARDWARE
        * Initializes all hardware peripherals and sensors 
        * required by the application.
        *******************************************************/
        case ProgramState::INIT_HARDWARE:
            if (initHardware()) {
                state = ProgramState::CONNECT_WIFI;   
            } else {
                state = ProgramState::ERROR;
            }
            break;
        /*****************************************************
        * STATE: CONNECT_WIFI
        * Establishes a WiFi connection if required. 
        * Falls back to local buffering if no network connection 
        * is available.
        *******************************************************/
        case ProgramState::CONNECT_WIFI:
            if (getWiFiConnectionStatus())
            {
                state = ProgramState::CONNECT_MQTT;
            }
            else if (connectWifi())
            {
                state = ProgramState::CONNECT_MQTT;
            }
            else
            {
                state = ProgramState::BUFFER_DATA;
            }
            break;
        /*****************************************************
        * STATE: CONNECT_MQTT
        * Establishes a connection to the MQTT broker.
        * Falls back to local buffering if the broker
        * cannot be reached.
        *******************************************************/
        case ProgramState::CONNECT_MQTT:
            if (getMqttConnectionStatus())
            {
                state = ProgramState::READ_SENSORS;
            }
            else if (!getMqttConnectionStatus())
            {   
                initMqtt(getWifiClient()); 
                if(mqttConnect())
                {
                state = ProgramState::READ_SENSORS;
                }
                else    
                {
                    state = ProgramState::BUFFER_DATA;
                }
            }
            break; 
        /*****************************************************
        * STATE: READ_SENSORS
        * Acquires the latest sensor and alarm values.
        ********************************************************/
        case ProgramState::READ_SENSORS:
            // Read all sensor data
            readBatteryVoltages(data);
            readSHT31(data);
            readAlarms(data);
            state = ProgramState::PUBLISH_DATA;
            break;     
        /*****************************************************
         * STATE: PUBLISH_DATA
         * Creates and publishes the telemetry payload.
         ********************************************************/
        case ProgramState::PUBLISH_DATA:
            if (!getWiFiConnectionStatus())
            {
                state = ProgramState::CONNECT_WIFI;
                break;
            }

            if (!getMqttConnectionStatus())
            {
                state = ProgramState::CONNECT_MQTT;
                break;
            }

            getTelemetry(payload, data);

            if (mqttPublish(payload))
            {
                state = ProgramState::WAIT_NEXT_CYCLE;
            }
            else
            {
                state = ProgramState::CONNECT_MQTT;
            }
            break;
        /*****************************************************
        * STATE: BUFFER_DATA
        *  Stores telemetry data locally while network
        * communication is unavailable.
        *******************************************************/
        case ProgramState::BUFFER_DATA:
            /* NO CURRENT IMPLEMENTATION */
            state = ProgramState::WAIT_NEXT_CYCLE;
            break;
        /*****************************************************
         * STATE: WAIT_NEXT_CYCLE
        * Idle state between telemetry transmissions.
        * Maintains active MQTT connections and waits
        * for the next measurement interval.
         ********************************************************/
        case ProgramState::WAIT_NEXT_CYCLE:
            if (getWiFiConnectionStatus() && getMqttConnectionStatus())
            {
                mqttLoop();
            }
            if (millis() - lastTelemetry >= TELEMETRY_INTERVAL_MS) {
                lastTelemetry = millis();
                state = ProgramState::READ_SENSORS;
            }         
            break;
    }
};