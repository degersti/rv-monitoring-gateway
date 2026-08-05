#include "fault_manager.h"
#include "app/status_indicator.h"
#include "app/runtime_manager.h"

static constexpr size_t FAULT_COUNT =
    static_cast<size_t>(FaultCode::COUNT);

static bool activeFaults[FAULT_COUNT] = {};

/*************************************************
 * Function:    isValidFaultCode
 * Description: Checks whether the specified
 *              fault code is valid.
 * Parameters:  code - Fault code to validate.
 * Returns:     true if the fault code is valid,
 *              otherwise false.
 * Notes:       FaultCode::NONE is not considered
 *              a valid runtime fault.
 *************************************************/
static bool isValidFaultCode(FaultCode code)
{
    const size_t index = static_cast<size_t>(code);

    return code != FaultCode::NONE &&
           index < FAULT_COUNT;
}

/*************************************************
 * Function:    faultIndex
 * Description: Converts a fault code into the
 *              corresponding array index.
 * Parameters:  code - Fault code.
 * Returns:     Array index of the specified
 *              fault code.
 * Notes:       The caller is responsible for
 *              providing a valid fault code.
 *************************************************/
static size_t faultIndex(FaultCode code)
{
    return static_cast<size_t>(code);
}

/*************************************************
 * Function:    initFaultManager
 * Description: Initializes the fault manager and
 *              clears all active faults.
 * Parameters:  None
 * Returns:     None
 * Notes:       Updates the ERROR indicator to
 *              reflect the current fault state.
 *************************************************/
void initFaultManager()
{
    clearAllFaults();
    updateFaultManager();
}

/*************************************************
 * Function:    setFault
 * Description: Marks the specified fault as
 *              active.
 * Parameters:  code - Fault code to activate.
 * Returns:     None
 * Notes:       Repeated calls for an already
 *              active fault have no additional
 *              effect.
 *************************************************/
void setFault(FaultCode code)
{
    if (!isValidFaultCode(code))
    {
        return;
    }

    activeFaults[faultIndex(code)] = true;
}

/*************************************************
 * Function:    clearFault
 * Description: Clears the specified fault.
 * Parameters:  code - Fault code to clear.
 * Returns:     None
 * Notes:       Invalid fault codes are ignored.
 *************************************************/
void clearFault(FaultCode code)
{
    if (!isValidFaultCode(code))
    {
        return;
    }

    activeFaults[faultIndex(code)] = false;
}

/*************************************************
 * Function:    clearAllFaults
 * Description: Clears all currently active
 *              faults.
 * Parameters:  None
 * Returns:     None
 * Notes:       Used during initialization to
 *              reset the complete fault state.
 *************************************************/
void clearAllFaults()
{
    for (size_t i = 0; i < FAULT_COUNT; ++i)
    {
        activeFaults[i] = false;
    }
}

/*************************************************
 * Function:    isFaultActive
 * Description: Checks whether the specified
 *              fault is currently active.
 * Parameters:  code - Fault code to check.
 * Returns:     true if the fault is active,
 *              otherwise false.
 * Notes:       Invalid fault codes always return
 *              false.
 *************************************************/
bool isFaultActive(FaultCode code)
{
    if (!isValidFaultCode(code))
    {
        return false;
    }

    return activeFaults[faultIndex(code)];
}

/*************************************************
 * Function:    hasAnyActiveFault
 * Description: Checks whether any fault is
 *              currently active.
 * Parameters:  None
 * Returns:     true if at least one fault is
 *              active, otherwise false.
 * Notes:       FaultCode::NONE is excluded from
 *              the evaluation.
 *************************************************/
bool hasAnyActiveFault()
{
    for (size_t i = 1; i < FAULT_COUNT; ++i)
    {
        if (activeFaults[i])
        {
            return true;
        }
    }

    return false;
}

/*************************************************
 * Function:    hasCriticalFault
 * Description: Checks whether any active fault
 *              has a critical severity.
 * Parameters:  None
 * Returns:     true if at least one critical
 *              fault is active, otherwise false.
 * Notes:       Severity is determined by the
 *              configured fault classification.
 *************************************************/
bool hasCriticalFault()
{
    for (size_t i = 1; i < FAULT_COUNT; ++i)
    {
        const FaultCode code =
            static_cast<FaultCode>(i);

        if (activeFaults[i] &&
            getFaultSeverity(code) ==
                FaultSeverity::CRITICAL)
        {
            return true;
        }
    }

    return false;
}

/*************************************************
 * Function:    getFaultSeverity
 * Description: Returns the configured severity
 *              of the specified fault.
 * Parameters:  code - Fault code.
 * Returns:     Fault severity.
 * Notes:       The severity classification is
 *              defined centrally within the
 *              fault manager.
 *************************************************/
FaultSeverity getFaultSeverity(FaultCode code)
{
    switch (code)
    {
        // Warning:
        // The gateway can continue operating with reduced functionality.
        case FaultCode::SHT31_READ_FAILED:
        case FaultCode::INPUT_ALARM_ACTIVE:
            return FaultSeverity::WARNING;

        // Error:
        // An important subsystem or operation has failed.
        case FaultCode::SD_INIT_FAILED:
        case FaultCode::SD_READ_FAILED:
        case FaultCode::SD_WRITE_FAILED:
            return FaultSeverity::ERROR;

        // Critical:
        // Normal system operation is no longer reliable.
        case FaultCode::CONFIG_INVALID:
        case FaultCode::SHT31_INIT_FAILED:
        case FaultCode::INVALID_SYSTEM_STATE:
            return FaultSeverity::CRITICAL;

        case FaultCode::NONE:
        case FaultCode::COUNT:
        default:
            return FaultSeverity::NONE;
    }
}

/*************************************************
 * Function:    getHighestActiveFaultSeverity
 * Description: Determines the highest severity
 *              of all currently active faults.
 * Parameters:  None
 * Returns:     Highest active fault severity or
 *              FaultSeverity::NONE if no faults
 *              are active.
 * Notes:       Used to determine the aggregated
 *              system fault state.
 *************************************************/
FaultSeverity getHighestActiveFaultSeverity()
{
    FaultSeverity highestSeverity =
        FaultSeverity::NONE;

    for (size_t i = 1; i < FAULT_COUNT; ++i)
    {
        if (!activeFaults[i])
        {
            continue;
        }

        const FaultSeverity severity =
            getFaultSeverity(
                static_cast<FaultCode>(i));

        if (static_cast<uint8_t>(severity) >
            static_cast<uint8_t>(highestSeverity))
        {
            highestSeverity = severity;
        }
    }

    return highestSeverity;
}

/*************************************************
 * Function:    updateErrorIndicator
 * Description: Updates the dedicated ERROR
 *              indicator according to the
 *              current fault and alarm state.
 * Parameters:  None
 * Returns:     None
 * Notes:       The indicator is switched off
 *              only when no active faults and
 *              no active alarms are present.
 *************************************************/
void updateFaultManager()
{
    if (!hasAnyActiveFault)
    {
        setIndicatorMode(
            Indicator::ERROR,
            IndicatorMode::OFF);

        return;
    }

    switch (getHighestActiveFaultSeverity())
    {
        case FaultSeverity::CRITICAL:
            setIndicatorMode(
                Indicator::ERROR,
                IndicatorMode::BLINK_FAST);
            break;

        case FaultSeverity::ERROR:
            setIndicatorMode(
                Indicator::ERROR,
                IndicatorMode::BLINK_SLOW);
            break;

        case FaultSeverity::WARNING:
            setIndicatorMode(
                Indicator::ERROR,
                IndicatorMode::ON);
            break;

        case FaultSeverity::NONE:
        default:
            // No system fault is active, but an
            // alarm is currently active.
            setIndicatorMode(
                Indicator::ERROR,
                IndicatorMode::OFF);
            break;
    }
}