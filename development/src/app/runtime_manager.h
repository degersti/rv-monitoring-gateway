#pragma once

#include <stdint.h>

void initRuntimeManager(void);
uint32_t getBootEpochId(void);
bool isAlarmActive(void);
void prepareDeepSleep(void);
void enterDeepSleep(void);
void printWakeupReason(void);

