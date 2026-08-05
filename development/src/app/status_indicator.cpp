/*************************************************
 * File:        status_indicator.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Implements four independent status indicators.
 *
 * Responsibilities:
 * - Status LED control
 * - Network LED control
 * - Backend LED control
 * - Error LED control
 * - Independent non-blocking blink patterns
 * - Temporary non-blocking flash sequences
 *
 *************************************************/

#include "status_indicator.h"
#include "config.h"

// Runtime state of a single status indicator LED
struct IndicatorRuntime
{
    uint8_t pin;
    IndicatorMode mode;

    uint32_t lastToggleTime;
    uint32_t modeChangeTime;

    bool outputOn;
    bool flashActive;

    uint8_t remainingFlashes;
};

// Runtime state of all indicators
static IndicatorRuntime indicators[] =
{
    {
        PIN_LED_STATUS,
        IndicatorMode::OFF,
        0,
        false,
        false,
        0
    },
    {
        PIN_LED_NETWORK,
        IndicatorMode::OFF,
        0,
        false,
        false,
        0
    },
    {
        PIN_LED_BACKEND,
        IndicatorMode::OFF,
        0,
        false,
        false,
        0
    },
    {
        PIN_LED_ERROR,
        IndicatorMode::OFF,
        0,
        false,
        false,
        0
    }
};

// Indicator timing
static constexpr uint32_t BLINK_SLOW_INTERVAL_MS = 1000;
static constexpr uint32_t BLINK_FAST_INTERVAL_MS = 150;
static constexpr uint32_t FLASH_INTERVAL_MS = 75;
static constexpr uint32_t MIN_MODE_DISPLAY_TIME_MS = 5000;

/*************************************************
 * Function:    getIndicatorRuntime
 * Description: Returns the runtime data associated
 *              with the selected indicator.
 * Parameters:  indicator - Indicator enum value
 * Returns:     Reference to the corresponding
 *              IndicatorRuntime object
 * Notes:       Internal helper function.
 *************************************************/
static IndicatorRuntime& getIndicatorRuntime(
    Indicator indicator)
{
    return indicators[static_cast<uint8_t>(indicator)];
}

/*************************************************
 * Function:    writeIndicator
 * Description: Sets the output state of the
 *              specified indicator LED and updates
 *              its runtime state.
 * Parameters:  indicator - Indicator runtime data
 *              enabled   - Desired LED state
 *                          (true = ON,
 *                           false = OFF)
 * Returns:     None
 * Notes:       Internal helper function.
 *************************************************/
static void writeIndicator(
    IndicatorRuntime& indicator,
    bool enabled)
{
    indicator.outputOn = enabled;

    digitalWrite(
        indicator.pin,
        enabled ? HIGH : LOW);
}

/*************************************************
 * Function:    updateBlink
 * Description: Updates a non-blocking blink pattern
 *              for the specified indicator.
 * Parameters:  indicator   - Indicator runtime data
 *              currentTime - Current system time
 *                            in milliseconds
 *              intervalMs  - Blink interval in
 *                            milliseconds
 * Returns:     None
 * Notes:       Internal helper function.
 *************************************************/
static void updateBlink(
    IndicatorRuntime& indicator,
    uint32_t currentTime,
    uint32_t intervalMs)
{
    if (currentTime - indicator.lastToggleTime <
        intervalMs)
    {
        return;
    }

    indicator.lastToggleTime = currentTime;

    writeIndicator(
        indicator,
        !indicator.outputOn);
}

/*************************************************
 * Function:    updateFlash
 * Description: Updates an active non-blocking flash
 *              sequence for the specified indicator.
 * Parameters:  indicator   - Indicator runtime data
 *              currentTime - Current system time
 *                            in milliseconds
 * Returns:     None
 * Notes:       A flash consists of one ON phase and
 *              one OFF phase. The normal indicator
 *              mode resumes after the final OFF
 *              phase has completed.
 *************************************************/
static void updateFlash(
    IndicatorRuntime& indicator,
    uint32_t currentTime)
{
    if (currentTime - indicator.lastToggleTime <
        FLASH_INTERVAL_MS)
    {
        return;
    }

    indicator.lastToggleTime = currentTime;

    if (indicator.outputOn)
    {
        // Complete the current flash by switching
        // the indicator off.
        writeIndicator(indicator, false);
        return;
    }

    if (indicator.remainingFlashes == 0)
    {
        // The final OFF phase has completed.
        indicator.flashActive = false;
        return;
    }

    // Start the next flash.
    indicator.remainingFlashes--;

    writeIndicator(indicator, true);
}

/*************************************************
 * Function:    initStatusIndicator
 * Description: Initializes all indicator GPIO pins
 *              and resets their runtime state.
 * Parameters:  None
 * Returns:     None
 * Notes:       All indicators are switched off
 *              during initialization.
 *************************************************/
void initStatusIndicator(void)
{
    for (IndicatorRuntime& indicator : indicators)
    {
        pinMode(indicator.pin, OUTPUT);

        indicator.mode = IndicatorMode::OFF;
        indicator.lastToggleTime = 0;
        indicator.modeChangeTime = millis();
        indicator.flashActive = false;
        indicator.remainingFlashes = 0;

        writeIndicator(indicator, false);
    }
}
/*************************************************
 * Function:    isStatusIndicatorBusy
 * Description: Checks whether the status indicator
 *              module is currently processing a
 *              temporary indication sequence or
 *              minimum mode display period.
 * Parameters:  None
 * Returns:     true  - An indication is still active
 *              false - The module is idle
 * Notes:       Can be used to prevent deep sleep
 *              until every changed indicator mode
 *              has been visible for at least the
 *              configured minimum duration.
 *************************************************/
bool isStatusIndicatorBusy(void)
{
    const uint32_t currentTime = millis();

    for (const IndicatorRuntime& indicator : indicators)
    {
        if (indicator.flashActive)
        {
            return true;
        }

        if (indicator.mode != IndicatorMode::OFF &&
            currentTime - indicator.modeChangeTime <
            MIN_MODE_DISPLAY_TIME_MS)
        {
            return true;
        }
    }

    return false;
}

/*************************************************
 * Function:    setIndicatorMode
 * Description: Sets the operating mode of the
 *              selected indicator.
 * Parameters:  indicator - Indicator to update
 *              mode      - New indicator mode
 * Returns:     None
 * Notes:       The mode change timestamp is reset
 *              only when the requested mode differs
 *              from the currently active mode.
 *************************************************/
void setIndicatorMode(
    Indicator indicator,
    IndicatorMode mode)
{
    IndicatorRuntime& runtime =
        getIndicatorRuntime(indicator);

    if (runtime.mode == mode)
    {
        return;
    }

    runtime.mode = mode;
    runtime.modeChangeTime = millis();

    if (runtime.flashActive)
    {
        return;
    }

    runtime.lastToggleTime = millis();

    // Start every new mode from a defined OFF state.
    writeIndicator(runtime, false);
}

/*************************************************
 * Function:    triggerIndicatorFlash
 * Description: Starts a temporary non-blocking flash
 *              sequence on the selected indicator.
 * Parameters:  indicator  - Indicator enum value
 *              flashCount - Number of flashes
 * Returns:     None
 * Notes:       The flash sequence temporarily
 *              overrides the normal indicator mode.
 *
 *              Calling this function again while a
 *              flash sequence is active restarts
 *              the sequence with the new count.
 *************************************************/
void triggerIndicatorFlash(
    Indicator indicator,
    uint8_t flashCount)
{
    if (flashCount == 0)
    {
        return;
    }

    IndicatorRuntime& runtime =
        getIndicatorRuntime(indicator);

    runtime.flashActive = true;
    runtime.remainingFlashes = flashCount;

    // Start the flash sequence from a defined
    // OFF state.
    writeIndicator(runtime, false);

    // Allow the first flash to start during the
    // next update cycle without an initial delay.
    runtime.lastToggleTime =
        millis() - FLASH_INTERVAL_MS;
}

/*************************************************
 * Function:    updateStatusIndicator
 * Description: Updates all status indicators
 *              according to their current modes.
 * Parameters:  None
 * Returns:     None
 * Notes:       Must be called regularly from the
 *              main program loop.
 *
 *              Active flash sequences temporarily
 *              override the normal indicator mode.
 *************************************************/
void updateStatusIndicator(void)
{
    // Read the current system time once so that all
    // indicators use the same timestamp during this
    // update cycle.
    const uint32_t currentTime = millis();

    // Process all four indicators one after another.
    for (IndicatorRuntime& runtime : indicators)
    {
        if (runtime.flashActive)
        {
            updateFlash(
                runtime,
                currentTime);

            continue;
        }

        switch (runtime.mode)
        {
            case IndicatorMode::OFF:
                writeIndicator(runtime, false);
                break;

            case IndicatorMode::ON:
                writeIndicator(runtime, true);
                break;

            case IndicatorMode::BLINK_SLOW:
                updateBlink(
                    runtime,
                    currentTime,
                    BLINK_SLOW_INTERVAL_MS);
                break;

            case IndicatorMode::BLINK_FAST:
                updateBlink(
                    runtime,
                    currentTime,
                    BLINK_FAST_INTERVAL_MS);
                break;
        }
    }
}