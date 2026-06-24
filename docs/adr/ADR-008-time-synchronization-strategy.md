# ADR-008: Time Synchronization Strategy

## Status

Accepted

## Context

The monitoring gateway records sensor data and alarm events that may either be transmitted immediately or stored locally when network connectivity is unavailable.

Reliable timestamping is required for:

* sensor measurements
* alarm events
* offline data buffering
* future historical analysis

The gateway is expected to spend most of its lifetime in Deep Sleep and periodically wake up either by:

* scheduled timer wake-ups
* asynchronous GPIO wake-ups triggered by alarm inputs

The system requires a time source that:

* survives Deep Sleep cycles
* supports offline operation
* automatically synchronizes when Internet connectivity becomes available
* minimizes hardware complexity and cost

An external real-time clock (RTC) module was considered but would increase hardware complexity, firmware complexity, and component count.

## Decision

The gateway shall use the ESP32 internal RTC-backed system clock as its local time source.

Network Time Protocol (NTP) shall be used as the authoritative time source whenever Internet connectivity is available.

After a successful NTP synchronization:

1. The ESP32 system clock shall be updated.
2. The internal RTC shall maintain the system time during Deep Sleep.
3. The current time shall be obtained from the operating system time functions.

The firmware shall not manually advance timestamps based on the planned Deep Sleep duration.

The same timestamp source shall be used for:

* timer wake-ups
* GPIO wake-ups
* normal operation

No external RTC module shall be used in Prototype 1.

If the gateway experiences a complete power loss or reset before a successful NTP synchronization has occurred, the system time shall be considered invalid.

## Time Representation

All timestamps shall be represented internally as Unix timestamps in Coordinated Universal Time (UTC).

The firmware shall not store, process, or convert:

* local time zones
* daylight saving time (DST)
* regional time settings

Timezone conversion shall be performed only by external systems such as:

* MQTT consumers
* databases
* dashboards
* visualization tools

Example:

```json
{
  "timestamp": 1782212400
}
```

## Time Synchronization

The gateway shall attempt NTP synchronization whenever:

* a network connection becomes available
* the current system time is invalid
* periodic re-synchronization is required to correct RTC drift

If NTP synchronization succeeds:

* the system time shall be updated
* the time validity flag shall be set

If NTP synchronization fails:

* the gateway shall continue using the last valid RTC-maintained system time
* the time validity state shall remain unchanged

## Deep Sleep Behavior

During Deep Sleep:

* the ESP32 internal RTC continues running
* the system time continues advancing automatically

After wake-up:

* the current time shall be read from the system clock
* no manual timestamp correction shall be performed
* no Deep Sleep duration calculations shall be performed

This behavior applies equally to:

* timer wake-ups
* GPIO wake-ups

## Timestamp Validity

The firmware shall maintain a time validity state.

A timestamp is considered valid when:

* at least one successful NTP synchronization has occurred since power-up

A timestamp is considered invalid when:

* the device has never synchronized its clock
* the device has experienced complete power loss and has not yet re-synchronized

When timestamps are invalid, measurements and alarm events may still be recorded and transmitted, but shall be marked accordingly.

Example:

```json
{
  "timestamp": 0,
  "timestampValid": false
}
```

or

```json
{
  "timestamp": 1782212400,
  "timestampValid": false
}
```

depending on implementation preference.

## Consequences

### Positive

* No additional hardware required.
* Minimal BOM cost.
* Time survives Deep Sleep cycles.
* Supports offline data buffering.
* Supports accurate timestamping of GPIO alarm events.
* Simple firmware implementation.
* Uses native ESP32 timekeeping mechanisms.
* Automatic correction of RTC drift through periodic NTP synchronization.

### Negative

* Time is lost after complete power removal.
* Long-term accuracy depends on RTC drift between NTP synchronizations.
* Offline operation after power loss cannot provide valid timestamps until network access is restored.

## Alternatives Considered

### External RTC Module (DS3231)

Advantages:

* Maintains time during complete power loss.
* Higher long-term accuracy.
* Battery-backed operation.

Disadvantages:

* Additional hardware cost.
* Increased PCB complexity.
* Additional firmware maintenance.
* Not required for Prototype 1.

### Manual Timestamp Advancement

Approach:

* Store timestamp before Deep Sleep.
* Store planned sleep duration.
* Add sleep duration after wake-up.

Advantages:

* Independent of RTC-based system time.

Disadvantages:

* Unnecessary because the ESP32 RTC maintains system time during Deep Sleep.
* Incorrect for asynchronous GPIO wake-ups.
* More complex implementation.
* Higher risk of timestamp errors.

## Implementation Notes

The implementation shall use standard ESP32 time functions:

```cpp
time(nullptr);
gettimeofday();
```

A dedicated Time Manager module shall be responsible for:

* NTP synchronization
* time validity tracking
* providing timestamps to application modules

The Time Manager shall provide a single interface for obtaining timestamps throughout the firmware.
