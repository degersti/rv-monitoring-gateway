#pragma once

#include <Arduino.h>

bool initSdCard();
bool isSdCardAvailable();
bool createSdDirectory(const char* path);
uint64_t getSdTotalBytes();
uint64_t getSdUsedBytes();
uint64_t getSdFreeBytes();