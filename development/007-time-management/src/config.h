#include "stdint.h"

constexpr char NTP_SERVER[] = "pool.ntp.org";
constexpr uint32_t NTP_SYNC_INTERVAL_SECONDS = 86400; //24h
constexpr uint32_t NTP_SYNC_TIMEOUT_MS = 10000; //10sec