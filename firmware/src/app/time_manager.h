#pragma once

#include "stdint.h"

void initTimeManager();
bool isTimeAvailable();
bool forceTimeSync();
bool isTimeSyncRequired();
uint32_t getCurrentTimestamp();
uint32_t getLastNtpSyncTimestamp();
uint32_t reconstructTimestamp(uint32_t storedTimestamp);
