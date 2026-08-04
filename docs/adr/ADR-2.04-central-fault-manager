# ADR-2.04 Central Fault Management

## Status

Accepted

## Context

Prototype 2 consists of several independent hardware and software modules, including:

- sensor acquisition
- SD card storage
- Wi-Fi communication
- LTE communication
- MQTT backend communication
- configuration handling
- system state management
- status indication

Each module can detect errors within its own area of responsibility.

Without a central fault management concept, fault handling would become distributed across the firmware. Individual modules could start their own behavior, severity evaluation, logging decisions or system reactions.

This would lead to:

- duplicated fault handling logic
- inconsistent fault classification
- direct dependencies between functional modules and status indicators
- reduced readability of the main state machine
- unclear ownership of system-wide fault reactions
- increasing complexity as additional modules are introduced

Prototype 2 requires a simple and clearly separated fault management concept. Persistent diagnostics and extensive fault history are not required at this stage.

## Decision

A central fault manager shall be introduced.

Individual modules remain responsible for detecting faults within their own scope.

Detected faults shall be reported to the fault manager. The fault manager shall maintain a centralized representation of the currently active system faults.

The fault manager shall be responsible for:

- collecting active system faults
- centrally classifying fault severity
- providing an aggregated system fault state
- determining the highest active fault severity
- controlling the ERROR indicator based on the current fault and alarm state

The fault manager shall not be responsible for:

- detecting hardware or communication faults itself
- controlling the STATUS, NETWORK or BACKEND indicators
- restarting the system
- entering deep sleep
- blocking deep sleep
- recovering failed modules
- publishing diagnostic messages
- writing persistent fault history
- managing monitored external alarms

## Separation of Responsibilities

The functional modules shall detect their own faults.

Examples include:

- the storage module detecting an SD card failure
- the sensor module detecting an invalid sensor reading
- the network module detecting a connection failure
- the backend module detecting a failed data transmission
- the configuration module detecting invalid configuration data

These modules shall report the detected condition to the fault manager but shall not independently implement global fault reactions.

The fault manager shall evaluate the combined system fault state.

The status indicator module shall remain responsible only for executing the requested LED behavior.

This results in the following dependency direction:

```text
functional modules
        |
        v
 fault manager
        |
        v
status indicator
```

The status indicator module shall not depend on the fault manager.

## Fault Scope

The fault manager shall manage internal system faults.

Examples include:

- sensor failures
- storage failures
- network failures
- backend communication failures
- configuration failures
- invalid internal system states

Monitored external conditions such as smoke or water ingress are alarms rather than system faults.

These alarms shall not be stored as fault entries.

The existing alarm evaluation shall remain unchanged.

However, the summarized alarm state shall be considered when controlling the ERROR indicator.

## Fault State

For Prototype 2, each fault shall have only two runtime states:

- inactive
- active

The fault manager shall represent only the current fault state.

Repeated reporting of an already active fault shall have no additional effect.

A fault remains active until:

- the responsible module reports that the fault is no longer present, or
- the fault manager is initialized again

Prototype 2 shall not maintain:

- occurrence counters
- first-occurrence timestamps
- last-occurrence timestamps
- transition history
- latched fault states
- persistent fault records

## Fault Severity

Each system fault shall be assigned one fixed severity.

The following severity levels shall be used:

| Severity | Meaning |
|----------|---------|
| Warning | A function is degraded, but the gateway can continue operating |
| Error | An important subsystem or operation has failed |
| Critical | Reliable normal system operation is no longer possible |

The severity assignment shall be defined centrally.

Individual modules shall not decide how their faults are displayed.

If multiple faults are active, the highest active severity shall represent the aggregated system fault state.

## Error Indicator

The dedicated ERROR indicator shall represent both:

- internal system faults
- the existing summarized alarm state

The ERROR indicator shall be switched off only when:

- no system fault is active, and
- no monitored alarm is active

If one or more system faults are active, the highest active fault severity shall determine the indicator state.

If no system fault is active but an alarm is active, the ERROR indicator shall still indicate an active error or alarm condition.

The detailed mapping between severities and indicator patterns shall remain part of the status indicator concept.

## Fault Lifetime

Faults shall initially be stored only for the current execution cycle.

The fault state shall not be retained persistently.

All faults shall be cleared after:

- a power cycle
- a hardware reset
- a software restart
- a wakeup from deep sleep

After initialization or wakeup, each module shall evaluate its current state again.

If a fault is still present, the responsible module shall report it again.

This behavior is accepted for Prototype 2 because the fault manager is intended to represent the current system state rather than provide historical diagnostics.

## Deep Sleep

An active fault shall not automatically prevent deep sleep.

Many fault conditions do not justify keeping the gateway awake, for example:

- unavailable Wi-Fi
- unavailable LTE communication
- unavailable backend communication
- a sensor failure
- an SD card failure

Keeping the gateway awake because of such faults could unnecessarily increase power consumption without improving system recovery.

Deep-sleep eligibility shall therefore remain a separate system decision.

Temporary conditions that prevent safe entry into deep sleep, such as:

- active storage operations
- unfinished communication
- active indicator flash sequences

shall be handled by the responsible modules or by the central power management logic.

The fault manager may provide information required for future fault-specific power decisions, but it shall not automatically control deep sleep in the initial implementation.

## Consequences

### Positive

- fault handling is centralized
- functional modules remain focused on their own responsibilities
- the main state machine remains readable
- severity classification is consistent
- the ERROR indicator has one defined control source
- no persistent storage is required
- no additional flash or SD card writes are introduced
- repeated fault reports remain simple and deterministic
- the concept can be extended later without changing module responsibilities

### Negative

- fault information is lost after reset or deep sleep
- short-lived faults may no longer be visible after the next wakeup
- no fault history is available
- recurring faults cannot be distinguished from first-time faults
- diagnostics depend on the normal logging system
- modules must evaluate their fault state again after every initialization

These limitations are accepted for Prototype 2.

## Future Extensions

Possible later extensions include:

- persistent fault history
- occurrence counters
- fault timestamps
- latched faults
- fault event logging
- diagnostic backend messages
- automatic recovery strategies
- fault-specific system reactions
- reset-cause diagnostics
- fault-specific deep-sleep behavior

These features are explicitly excluded from the initial Prototype 2 implementation.

## Decision Summary

Prototype 2 shall use a simple central fault manager.

Functional modules detect and report their own faults. The fault manager collects and evaluates the currently active system faults and controls the ERROR indicator based on the aggregated fault and alarm state.

Faults shall not be retained across reset, power loss or deep sleep.

The initial implementation shall represent only the current system fault state and shall not include persistent diagnostics, counters, timestamps or automatic recovery behavior.
