#pragma once

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

void log(LogLevel level, const char* format, ...);
void logProgress(const char* msg);

#define LOG_DEBUG(...) log(LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...)  log(LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...)  log(LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) log(LogLevel::Error, __VA_ARGS__)
#define LOG_PROGRESS(msg) logProgress(msg)