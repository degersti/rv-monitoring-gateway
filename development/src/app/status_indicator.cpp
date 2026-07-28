/*************************************************
 * File:        status_indicator.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Implements the system status indicator.
 *
 * Responsibilities:
 * - RGB LED control
 * - Status visualization
 * - Alarm indication
 * - Telemetry activity indication
 *
 *************************************************/

#include "status_indicator.h"
#include <Adafruit_NeoPixel.h>
#include "config.h"

// Onboard RGB NeoPixel LED instance
Adafruit_NeoPixel rgbLed(1, PIN_RGB_LED, NEO_GRB + NEO_KHZ800);

// Current indicator state
static IndicatorState currentState = IndicatorState::BOOT;

// Timing variables for non-blocking blink patterns
static uint32_t lastToggleTime = 0;
static bool blinkOn = false;

// Telemetry flash event
static bool telemetryFlashActive = false;
static uint32_t telemetryLastToggleTime = 0;
static bool telemetryFlashOn = false;
static uint8_t telemetryFlashCount = 0;

// Telemetry flash timing (3 fast green flashes)
static const uint32_t TELEMETRY_FLASH_INTERVAL_MS = 75;
static const uint8_t TELEMETRY_FLASH_COUNT = 3;

/*************************************************
 * Function:    setRgb
 * Description: Sets the RGB LED to the specified
 *              color and updates the LED output.
 * Parameters:  red   - Red intensity (0..255)
 *              green - Green intensity (0..255)
 *              blue  - Blue intensity (0..255)
 * Returns:     None (void function)
 * Notes:       Color values are applied directly
 *              to the onboard NeoPixel LED.
 *************************************************/
static void setRgb(uint8_t red, uint8_t green, uint8_t blue)
{
    rgbLed.setPixelColor(0, rgbLed.Color(red, green, blue));
    rgbLed.show();
}

/*************************************************
 * Function:    setOff
 * Description: Turns the RGB LED off.
 * Parameters:  None
 * Returns:     None (void function)
 * Notes:       Sets all color channels to zero.
 *************************************************/
static void setOff(void)
{
    setRgb(0, 0, 0);
}

/*************************************************
 * Function:    blinkRgb
 * Description: Generates a non-blocking blink
 *              pattern using the specified color
 *              and interval.
 * Parameters:  red        - Red intensity (0..255)
 *              green      - Green intensity (0..255)
 *              blue       - Blue intensity (0..255)
 *              intervalMs - Blink interval in ms
 * Returns:     None (void function)
 * Notes:       Uses millis() for timing.
 *************************************************/
static void blinkRgb(uint8_t red, uint8_t green, uint8_t blue, uint32_t intervalMs)
{
    uint32_t now = millis();

    if (now - lastToggleTime >= intervalMs)
    {
        blinkOn = !blinkOn;
        lastToggleTime = now;

        if (blinkOn)
        {
            setRgb(red, green, blue);
        }
        else
        {
            setOff();
        }
    }
}

/*************************************************
 * Function:    updateTelemetryFlash
 * Description: Executes the telemetry activity
 *              indication pattern.
 * Parameters:  None
 * Returns:     None (void function)
 * Notes:       Displays three short green flashes
 *              after a successful telemetry
 *              transmission.
 *************************************************/
static void updateTelemetryFlash(void)
{
    uint32_t now = millis();

    if (now - telemetryLastToggleTime >= TELEMETRY_FLASH_INTERVAL_MS)
    {
        telemetryLastToggleTime = now;
        telemetryFlashOn = !telemetryFlashOn;

        if (telemetryFlashOn)
        {
            setRgb(0, 255, 0);      // Green
        }
        else
        {
            setOff();
            telemetryFlashCount++;

            if (telemetryFlashCount >= TELEMETRY_FLASH_COUNT)
            {
                telemetryFlashActive = false;
                telemetryFlashCount = 0;
                telemetryFlashOn = false;
            }
        }
    }
}

/*************************************************
 * Function:    initStatusIndicator
 * Description: Initializes the onboard RGB LED
 *              and sets the initial boot state.
 * Parameters:  None
 * Returns:     None (void function)
 * Notes:       Must be called once during system
 *              startup.
 *************************************************/
void initStatusIndicator(void)
{
    rgbLed.begin();
    rgbLed.setBrightness(32);
    rgbLed.show();

    currentState = IndicatorState::OFF;
}

/*************************************************
 * Function:    setIndicatorState
 * Description: Updates the current status
 *              indicator state.
 * Parameters:  state - New indicator state
 * Returns:     None (void function)
 * Notes:       Resets blink timing whenever the
 *              state changes.
 *************************************************/
void setIndicatorState(IndicatorState state)
{
    if (currentState != state)
    {
        currentState = state;
        lastToggleTime = 0;
        blinkOn = false;
    }
}

/*************************************************
 * Function:    triggerTelemetryFlash
 * Description: Starts the telemetry activity
 *              indication sequence.
 * Parameters:  None
 * Returns:     None (void function)
 * Notes:       Triggers three short green flashes
 *              without changing the current
 *              indicator state.
 *************************************************/
void triggerTelemetryFlash(void)
{
    telemetryFlashActive = true;
    telemetryLastToggleTime = 0;
    telemetryFlashOn = false;
    telemetryFlashCount = 0;
}

/*************************************************
 * Function:    updateStatusIndicator
 * Description: Updates the status indicator LED
 *              according to the current system
 *              state.
 * Parameters:  None
 * Returns:     None (void function)
 * Notes:       Must be called cyclically from the
 *              main application loop.
 *************************************************/
void updateStatusIndicator(void)
{
    if (telemetryFlashActive &&
        currentState != IndicatorState::ALARM_ACTIVE &&
        currentState != IndicatorState::ERROR_ACTIVE)
    {
        updateTelemetryFlash();
        return;
    }

    switch (currentState)
    {
        case IndicatorState::BOOT:
            blinkRgb(255, 180, 0, 100);
            break;
             
        case IndicatorState::WIFI_CONNECTING:
            blinkRgb(0, 0, 255, 1000);
            break;

        case IndicatorState::WIFI_CONNECTED:
            setRgb(0, 0, 255);
            break;

        case IndicatorState::MQTT_CONNECTING:
            blinkRgb(0, 255, 0, 1000);
            break;

        case IndicatorState::MQTT_ONLINE:
            setRgb(0, 255, 0);
            break;

        case IndicatorState::ALARM_ACTIVE:
            blinkRgb(255, 0, 0, 1000);
            break;

        case IndicatorState::ERROR_ACTIVE:
            blinkRgb(255, 0, 0, 150);
            break;

        case IndicatorState::OFF:
            setOff();
            break;
    }
}