#include <Arduino.h>
#include "app/status_indicator.h"

static uint32_t lastStateChangeTime = 0;
static uint8_t testStep = 0;

static constexpr uint32_t STATE_DURATION_MS = 5000;

/*************************************************
 * Function:    setAllIndicatorModes
 * Description: Sets the same mode for all status
 *              indicators.
 * Parameters:  mode - Indicator mode to apply
 * Returns:     None
 * Notes:       Internal test helper function.
 *************************************************/
static void setAllIndicatorModes(
    IndicatorMode mode)
{
    setIndicatorMode(
        Indicator::STATUS,
        mode);

    setIndicatorMode(
        Indicator::NETWORK,
        mode);

    setIndicatorMode(
        Indicator::BACKEND,
        mode);

    setIndicatorMode(
        Indicator::ERROR,
        mode);
}

/*************************************************
 * Function:    setupDevStatusIndicator
 * Description: Initializes the status indicator
 *              development test.
 * Parameters:  None
 * Returns:     None
 *************************************************/
void setupDevStatusIndicator()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("--------------------------------");
    Serial.println("DEV-005 Status Indicator Test");
    Serial.println("--------------------------------");

    initStatusIndicator();

    setAllIndicatorModes(
        IndicatorMode::OFF);

    lastStateChangeTime = millis();
}

/*************************************************
 * Function:    loopDevStatusIndicator
 * Description: Executes the automatic status
 *              indicator test sequence.
 * Parameters:  None
 * Returns:     None
 * Notes:       Must be called regularly from the
 *              main program loop.
 *************************************************/
void loopDevStatusIndicator()
{
    updateStatusIndicator();

    const uint32_t currentTime = millis();

    if (currentTime - lastStateChangeTime <
        STATE_DURATION_MS)
    {
        return;
    }

    lastStateChangeTime = currentTime;
    testStep++;

    switch (testStep)
    {
        // -------------------------------------------------
        // STATUS indicator
        // -------------------------------------------------

        case 1:
            Serial.println("Status: BLINK_SLOW");

            setIndicatorMode(
                Indicator::STATUS,
                IndicatorMode::BLINK_SLOW);
            break;

        case 2:
            Serial.println("Status: BLINK_FAST");

            setIndicatorMode(
                Indicator::STATUS,
                IndicatorMode::BLINK_FAST);
            break;

        case 3:
            Serial.println("Status: ON");

            setIndicatorMode(
                Indicator::STATUS,
                IndicatorMode::ON);
            break;

        case 4:
            Serial.println(
                "Status: SINGLE FLASH while mode is ON");

            triggerIndicatorFlash(
                Indicator::STATUS);
            break;

        case 5:
            Serial.println(
                "Status: TRIPLE FLASH while mode is ON");

            triggerIndicatorFlash(
                Indicator::STATUS,
                3);
            break;

        case 6:
            Serial.println("Status: OFF");

            setIndicatorMode(
                Indicator::STATUS,
                IndicatorMode::OFF);
            break;

        // -------------------------------------------------
        // NETWORK indicator
        // -------------------------------------------------

        case 7:
            Serial.println("Network: BLINK_SLOW");

            setIndicatorMode(
                Indicator::NETWORK,
                IndicatorMode::BLINK_SLOW);
            break;

        case 8:
            Serial.println("Network: BLINK_FAST");

            setIndicatorMode(
                Indicator::NETWORK,
                IndicatorMode::BLINK_FAST);
            break;

        case 9:
            Serial.println("Network: ON");

            setIndicatorMode(
                Indicator::NETWORK,
                IndicatorMode::ON);
            break;

        case 10:
            Serial.println(
                "Network: SINGLE FLASH while mode is ON");

            triggerIndicatorFlash(
                Indicator::NETWORK);
            break;

        case 11:
            Serial.println(
                "Network: TRIPLE FLASH while mode is ON");

            triggerIndicatorFlash(
                Indicator::NETWORK,
                3);
            break;

        case 12:
            Serial.println("Network: OFF");

            setIndicatorMode(
                Indicator::NETWORK,
                IndicatorMode::OFF);
            break;

        // -------------------------------------------------
        // BACKEND indicator
        // -------------------------------------------------

        case 13:
            Serial.println("Backend: BLINK_SLOW");

            setIndicatorMode(
                Indicator::BACKEND,
                IndicatorMode::BLINK_SLOW);
            break;

        case 14:
            Serial.println("Backend: BLINK_FAST");

            setIndicatorMode(
                Indicator::BACKEND,
                IndicatorMode::BLINK_FAST);
            break;

        case 15:
            Serial.println("Backend: ON");

            setIndicatorMode(
                Indicator::BACKEND,
                IndicatorMode::ON);
            break;

        case 16:
            Serial.println(
                "Backend: SINGLE FLASH while mode is ON");

            triggerIndicatorFlash(
                Indicator::BACKEND);
            break;

        case 17:
            Serial.println(
                "Backend: TRIPLE FLASH while mode is ON");

            triggerIndicatorFlash(
                Indicator::BACKEND,
                3);
            break;

        case 18:
            Serial.println("Backend: OFF");

            setIndicatorMode(
                Indicator::BACKEND,
                IndicatorMode::OFF);
            break;

        // -------------------------------------------------
        // ERROR indicator
        // -------------------------------------------------

        case 19:
            Serial.println("Error: BLINK_SLOW");

            setIndicatorMode(
                Indicator::ERROR,
                IndicatorMode::BLINK_SLOW);
            break;

        case 20:
            Serial.println("Error: BLINK_FAST");

            setIndicatorMode(
                Indicator::ERROR,
                IndicatorMode::BLINK_FAST);
            break;

        case 21:
            Serial.println("Error: ON");

            setIndicatorMode(
                Indicator::ERROR,
                IndicatorMode::ON);
            break;

        case 22:
            Serial.println(
                "Error: SINGLE FLASH while mode is ON");

            triggerIndicatorFlash(
                Indicator::ERROR);
            break;

        case 23:
            Serial.println(
                "Error: TRIPLE FLASH while mode is ON");

            triggerIndicatorFlash(
                Indicator::ERROR,
                3);
            break;

        case 24:
            Serial.println("Error: OFF");

            setIndicatorMode(
                Indicator::ERROR,
                IndicatorMode::OFF);
            break;

        // -------------------------------------------------
        // Combined indicator tests
        // -------------------------------------------------

        case 25:
            Serial.println("All indicators: BLINK_SLOW");

            setAllIndicatorModes(
                IndicatorMode::BLINK_SLOW);
            break;

        case 26:
            Serial.println("All indicators: BLINK_FAST");

            setAllIndicatorModes(
                IndicatorMode::BLINK_FAST);
            break;

        case 27:
            Serial.println("All indicators: ON");

            setAllIndicatorModes(
                IndicatorMode::ON);
            break;

        case 28:
            Serial.println(
                "All indicators: SINGLE FLASH while mode is ON");

            triggerIndicatorFlash(
                Indicator::STATUS);

            triggerIndicatorFlash(
                Indicator::NETWORK);

            triggerIndicatorFlash(
                Indicator::BACKEND);

            triggerIndicatorFlash(
                Indicator::ERROR);
            break;

        case 29:
            Serial.println(
                "All indicators: TRIPLE FLASH while mode is ON");

            triggerIndicatorFlash(
                Indicator::STATUS,
                3);

            triggerIndicatorFlash(
                Indicator::NETWORK,
                3);

            triggerIndicatorFlash(
                Indicator::BACKEND,
                3);

            triggerIndicatorFlash(
                Indicator::ERROR,
                3);
            break;

        case 30:
            Serial.println("All indicators: OFF");

            setAllIndicatorModes(
                IndicatorMode::OFF);
            break;

        // -------------------------------------------------
        // Mixed indicator tests
        // -------------------------------------------------

        case 31:
            Serial.println("Mixed indicator modes");

            setIndicatorMode(
                Indicator::STATUS,
                IndicatorMode::ON);

            setIndicatorMode(
                Indicator::NETWORK,
                IndicatorMode::BLINK_SLOW);

            setIndicatorMode(
                Indicator::BACKEND,
                IndicatorMode::BLINK_FAST);

            setIndicatorMode(
                Indicator::ERROR,
                IndicatorMode::OFF);
            break;

        case 32:
            Serial.println(
                "Mixed modes with different flash counts");

            triggerIndicatorFlash(
                Indicator::STATUS,
                1);

            triggerIndicatorFlash(
                Indicator::NETWORK,
                2);

            triggerIndicatorFlash(
                Indicator::BACKEND,
                3);

            triggerIndicatorFlash(
                Indicator::ERROR,
                4);
            break;

        case 33:
            Serial.println("All indicators: OFF");

            setAllIndicatorModes(
                IndicatorMode::OFF);
            break;

        // -------------------------------------------------
        // Reset test sequence
        // -------------------------------------------------

        default:
            Serial.println("Restart test sequence");
            Serial.println("--------------------------------");

            testStep = 0;

            setAllIndicatorModes(
                IndicatorMode::OFF);
            break;
    }
}