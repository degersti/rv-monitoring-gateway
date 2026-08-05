#pragma once

#include <Arduino.h>

enum class FaultCode : uint8_t
{
    NONE = 0,
    // Configuration
    CONFIG_LOAD_FAILED,
    CONFIG_INVALID,
    // Sensors
    SHT31_INIT_FAILED,
    SHT31_READ_FAILED,
    // Storage
    SD_INIT_FAILED,
    SD_READ_FAILED,
    SD_WRITE_FAILED,
    // External 
    INPUT_ALARM_ACTIVE,
    // Internal
    INVALID_SYSTEM_STATE,
    // Sentinel value to indicate the number of fault codes
    COUNT
};

enum class FaultSeverity : uint8_t
{
    NONE = 0,
    WARNING,
    ERROR,
    CRITICAL
};

void initFaultManager();
void setFault(FaultCode code);
void clearFault(FaultCode code);
void clearAllFaults();
bool isFaultActive(FaultCode code);
bool hasAnyActiveFault();
bool hasCriticalFault();
FaultSeverity getFaultSeverity(FaultCode code);
FaultSeverity getHighestActiveFaultSeverity();
void updateFaultManager();