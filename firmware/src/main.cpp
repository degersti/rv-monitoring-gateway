/*************************************************
 * File:        main.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Main application entry point.
 *
 * Responsibilities:
 * - System initialization
 * - Main application loop
 * - State machine execution
 * - Cyclic service updates
 *
 *************************************************/

#include <Arduino.h>
#include "state_machine.h"
#include "status_indicator.h"

/*************************************************
 * Function:    setup
 * Description: Initializes the system hardware and
 *              software components after startup.
 *************************************************/
void setup()
{
    Serial.begin(115200);
    delay(2000);
    initStatusIndicator();
}

/*************************************************
 * Function:     loop
 * Description:  Main program loop. Executes the
 *               application state machine and the
 *               status indicator update.
 *************************************************/
void loop()
{
    runStateMachine();
    updateStatusIndicator();
}
