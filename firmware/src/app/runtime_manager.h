#pragma once

#include <stdint.h>
void initWatchdog(void);
void initRuntimeManager(void);
void feedRuntimeWatchdog(void);
uint32_t getBootEpochId(void);
const char* getDeviceId(void);
bool isAlarmActive(void);
void prepareDeepSleep(void);
void enterDeepSleep(void);
void printWakeupReason(void);

