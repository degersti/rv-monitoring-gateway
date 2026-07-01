#include <Arduino.h>

#ifdef DEV_001_SENSOR_MANAGER
    #include "test/DEV-001-sensor-manager.h"
#endif

#ifdef DEV_004_WIFI_AND_MQTT
    #include <WiFi.h>
    #include "test/DEV-004-wifi-and-mqtt.h"
#endif

#ifdef DEV_005_STATUS_INDICATOR
    #include "test/DEV-005-status-indicator.h"
#endif

#ifdef DEV_006_DEEP_SLEEP
    #include "test/DEV-006-deep-sleep.h"
#endif

#ifdef DEV_007_TIME_MANAGEMENT
    #include <WiFi.h>
    #include "test/DEV-007-time-menagement.h"
#endif

#ifdef DEV_008_MEASUREMENT_BUFFER
    #include "test/DEV-008-measurement-buffer.h"
#endif

void setup()
{
    #ifdef DEV_001_SENSOR_MANAGER
        setupDevSensorManager();
    #endif
    #ifdef DEV_004_WIFI_AND_MQTT
        setupDevWifiAndMqtt();
    #endif
    #ifdef DEV_005_STATUS_INDICATOR
        setupDevStatusIndicator();
    #endif
    #ifdef DEV_006_DEEP_SLEEP
        setupDevDeepSleep();
    #endif
    #ifdef DEV_007_TIME_MANAGEMENT
        setupDevTimeManagement();
    #endif
    #ifdef DEV_008_MEASUREMENT_BUFFER
        setupDevMeasurementBuffer();
    #endif
}
void loop()
{
    #ifdef DEV_001_SENSOR_MANAGER
        loopDevSensorManager();
    #endif
    #ifdef DEV_004_WIFI_AND_MQTT
        loopDevWifiAndMqtt();
    #endif
    #ifdef DEV_005_STATUS_INDICATOR
        loopDevStatusIndicator();
    #endif
    #ifdef DEV_006_DEEP_SLEEP
        loopDevDeepSleep();
    #endif
    #ifdef DEV_007_TIME_MANAGEMENT
        loopDevTimeManagement();
    #endif
    #ifdef DEV_008_MEASUREMENT_BUFFER
        loopDevMeasurementBuffer();
    #endif
}