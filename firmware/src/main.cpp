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
#include "app/runtime_manager.h"
#include "app/hardware_manager.h"
#include "app/status_indicator.h"
#include "state_machine.h"

/*************************************************
 * Function:    setup
 * Description: Initializes the system hardware and
 *              software components after startup.
 * Parameters:  None
 * Returns:     None (void function)
 * Notes:       Called once after system reset or
 *              power-up. Initializes the runtime
 *              context, basic hardware peripherals, 
 *              and the application state machine.
 *************************************************/
void setup()
{
    initRuntimeManager();
    initWatchdog();
    initHardwareManager();
    initStatusIndicator();
    initStateMachine();
}

/*************************************************
 * Function:     loop
 * Description:  Main program loop. Executes the
 *               application state machine and the
 *               status indicator update.
 * Parameters:   None
 * Returns:      None (void function)
 * Notes:        Called repeatedly after setup() to
 *               maintain application operation. The
 *               loop() function is responsible for
 *               executing the state machine and 
 *               updating the status indicator.
 *************************************************/
void loop()
{
    runStateMachine();
    updateStatusIndicator();
    feedRuntimeWatchdog();
}
