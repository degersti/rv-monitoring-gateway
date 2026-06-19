#include <Arduino.h>
#include "state_machine.h"

/*************************************************
 * Function:    setup
 * Description: Initializes the system hardware and software components after startup.
 * Parameters:  None
 * Returns:     None (void function)
 * Notes:       Called once after reset or power-up.
 *************************************************/
void setup()
{
    Serial.begin(115200);
    delay(2000);
}

/*************************************************
* Function:     loop
* Description:  Main program loop. Executes the
* application   state machine.
* Parameters:   None
* Returns:      None (void function)
* Notes:        Called repeatedly by the Arduino framework.
*************************************************/
void loop()
{
    runStateMachine();
}

