#pragma once

#include <Arduino.h>

enum class ProgramState
{
    BOOT,
    PROCESS,
    IDLE,
    POWER_DECISION,
    DEEP_SLEEP
};

void initStateMachine();
void runStateMachine();