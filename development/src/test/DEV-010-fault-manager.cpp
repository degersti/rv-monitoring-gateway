#include <Arduino.h>

#include "app/fault_manager.h"
#include "app/status_indicator.h"
#include "app/runtime_manager.h"

static uint32_t lastStateChangeTime = 0;
static uint8_t testStep = 0;

static const uint32_t STATE_DURATION_MS = 5000;

void setupDevFaultManager()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("--------------------------------");
    Serial.println("DEV-006 Fault Manager Test");
    Serial.println("--------------------------------");

    initStatusIndicator();
    initFaultManager();

    lastStateChangeTime = millis();
}

void loopDevFaultManager()
{
    updateStatusIndicator();

    if (millis() - lastStateChangeTime <
        STATE_DURATION_MS)
    {
        return;
    }

    lastStateChangeTime = millis();

    testStep++;

    clearAllFaults();

    switch (testStep)
    {
        case 1:
            Serial.println("Test 1: No active faults");
            break;

        case 2:
            Serial.println("Test 2: Warning");

            setFault(
                FaultCode::SHT31_READ_FAILED);
            break;

        case 3:
            Serial.println("Test 3: Error");

            setFault(
                FaultCode::SD_INIT_FAILED);
            break;

        case 4:
            Serial.println("Test 4: Critical");

            setFault(
                FaultCode::INVALID_SYSTEM_STATE);
            break;

        case 5:
            Serial.println("Test 5: Warning + Error + Critical");

            setFault(
                FaultCode::SHT31_READ_FAILED);

            setFault(
                FaultCode::SD_INIT_FAILED);

            setFault(
                FaultCode::INVALID_SYSTEM_STATE);
            break;

        case 6:
            Serial.println("Test 6: Warning + Error");

            setFault(
                FaultCode::SHT31_READ_FAILED);

            setFault(
                FaultCode::SD_INIT_FAILED);
            break;

        case 7:
            Serial.println("Test 7: Warning");

            setFault(
                FaultCode::SHT31_READ_FAILED);
            break;

        case 8:
            Serial.println("Test 8: Clear all faults");

            clearAllFaults();
            break;

        default:
            testStep = 0;
            break;
    }

    updateFaultManager();

    Serial.print("Fault active : ");
    Serial.println(
        hasAnyActiveFault() ? "YES" : "NO");

    Serial.print("Critical     : ");
    Serial.println(
        hasCriticalFault() ? "YES" : "NO");

    Serial.print("Severity     : ");

    switch (getHighestActiveFaultSeverity())
    {
        case FaultSeverity::NONE:
            Serial.println("NONE");
            break;

        case FaultSeverity::WARNING:
            Serial.println("WARNING");
            break;

        case FaultSeverity::ERROR:
            Serial.println("ERROR");
            break;

        case FaultSeverity::CRITICAL:
            Serial.println("CRITICAL");
            break;
    }

    Serial.println();
}