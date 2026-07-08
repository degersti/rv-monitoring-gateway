#pragma once

#include <stdint.h>

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

bool isSerialInitialized(void);
bool isDebugModeEnabled(void);
void initSerialDebugDelayed(uint32_t startTime);
void initSerialDebugNow(void);
void log(LogLevel level, const char* format, ...);
void logProgress(const char* msg);

#define LOG_DEBUG(...) log(LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...)  log(LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...)  log(LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) log(LogLevel::Error, __VA_ARGS__)
#define LOG_PROGRESS(msg) logProgress(msg)