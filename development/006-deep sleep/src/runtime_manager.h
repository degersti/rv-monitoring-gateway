#pragma once

bool isSerialInitialized(void);
bool isDebugModeEnabled(void);
bool isAlarmActive(void);
void prepareDeepSleep(void);
void enterDeepSleep(void);

void initSerialDebugDelayed(uint32_t);
void initSerialDebugNow(void);
void debugPrintWakeupReason(void);

