#pragma once

#include <Arduino.h>

/*************************************************
 * Available physical status indicators.
 *
 * The enum order must match the order of the
 * runtime entries in status_indicator.cpp.
 *************************************************/
enum class Indicator : uint8_t
{
    STATUS,
    NETWORK,
    BACKEND,
    ERROR
};

/*************************************************
 * Available indication modes.
 *
 * These modes describe only the physical LED
 * behavior and contain no application-specific
 * meaning.
 *************************************************/
enum class IndicatorMode : uint8_t
{
    OFF,
    ON,
    BLINK_SLOW,
    BLINK_FAST
};

void initStatusIndicator(void);
bool isStatusIndicatorBusy(void);
void setIndicatorMode(Indicator indicator,IndicatorMode mode);
void triggerIndicatorFlash(Indicator indicator, uint8_t flashCount = 1);
void updateStatusIndicator(void);