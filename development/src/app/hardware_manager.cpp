/*************************************************
 * File:        hardware_manager.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Initializes the basic hardware configuration
 * of the ESP32 board.
 *
 * Responsibilities:
 * - GPIO initialization
 * - Digital input configuration
 * - ADC pin configuration
 *
 *************************************************/
#include <Arduino.h>
#include "config.h"

/*************************************************
 * Function:    initHardwareManager
 * Description: Configures all GPIOs required by
 *              the application.
 * Parameters:  None
 * Returns:     None
 * Notes:       Initializes only the basic pin
 *              configuration. Peripheral-specific
 *              initialization (ADC, I2C, sensors,
 *              WiFi, etc.) is performed by the
 *              corresponding modules.
 *************************************************/
void initHardwareManager()
{
    // Debug enable input
    pinMode(PIN_SERIAL_DEBUG_ENABLE, INPUT_PULLUP);
    // Initialize alarm inputs
    pinMode(PIN_WATER_ALARM, INPUT);
    pinMode(PIN_SMOKE_ALARM, INPUT);
    // ADC inputs
    pinMode(PIN_HOUSE_ADC, INPUT);
    pinMode(PIN_ENGINE_ADC, INPUT);
}