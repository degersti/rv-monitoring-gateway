#include "config.h"
#include "state_machine.h"
#include "status_indicator.h"
#include "runtime_manager.h"

// Current program state
ProgramState state; 
// Timestamp of current state entry
uint32_t stateStartTime;

/*************************************************
 * Function:    initStateMachine
 * Description: Initializes the program state
 *              machine and enters the BOOT state.
 *************************************************/
void initStateMachine()
{
    state = ProgramState::BOOT;
    stateStartTime = millis();
}
/*************************************************
 * Function:    setState
 * Description: Changes the current program state
 *              and updates the state entry
 *              timestamp if the state changed.
 *************************************************/
void setState(ProgramState newState)
{
    if (state != newState)
    {
        state = newState;
        stateStartTime = millis();
    }
}
/*************************************************
 * Function:    runStateMachine
 * Description: Executes the current program state
 *              and performs state transitions.
 *************************************************/
void runStateMachine()
{
    switch (state)
    {
         /*****************************************************
         * STATE: BOOT
         * Entry state after reset or power-up.
         *****************************************************/
        case ProgramState::BOOT:
        {   
            setIndicatorState(IndicatorState::BOOT);
            if(isDebugModeEnabled() && !isSerialInitialized())
            {                 
                initSerialDebugDelayed(stateStartTime);  
                break;   
            } 
            if (isDebugModeEnabled())
            {
                debugPrintWakeupReason();
            }
            setState(ProgramState::PROCESS);
            break;
        }
        /*****************************************************
         * STATE: PROCESS
         * Placeholder for later firmware states
         *****************************************************/
        case ProgramState::PROCESS:
        {
            if(isDebugModeEnabled())
            {
                Serial.println("Processing some Data");

            }
            setState(ProgramState::POWER_DECISION);
            break;
        }
        /*****************************************************
         * STATE: POWER_DECISION
         * Decision staying awake or sleep until next cycle
         *****************************************************/
        case ProgramState::POWER_DECISION:
        {
            if(isAlarmActive())
            {
                setState(ProgramState::IDLE);
            }else
            {
                prepareDeepSleep();
                setState(ProgramState::DEEP_SLEEP);
            }
            break;
        }
        /*****************************************************
         * STATE: IDLE
         * Staying awake until next cycle
         *****************************************************/
        case ProgramState::IDLE:
        {
            setIndicatorState(IndicatorState::WIFI_CONNECTED);
            if (millis() - stateStartTime >= (CYCLE_INTERVAL_SEC *1000ULL))
            {
                setState(ProgramState::PROCESS);
                if(isDebugModeEnabled() && !isSerialInitialized())
                {  
                    initSerialDebugNow();
                }
            }
            break;
        }
          /*****************************************************
         * STATE: DEEP_SLEEP
         * Going to sleep until next 
         *****************************************************/
        case ProgramState::DEEP_SLEEP:
        {
            setIndicatorState(IndicatorState::OFF);
            updateStatusIndicator();
            enterDeepSleep();
            break;
        }
    }
}