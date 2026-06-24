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
* can be periodically synchronized with an external reference
* minimizes hardware complexity and cost

An external real-time clock (RTC) module was considered but would increase hardware complexity, firmware complexity, and component count.

## Decision

The gateway shall use the ESP32 internal RTC-backed system clock as its local time source.

Network Time Protocol (NTP) shall be used as the authoritative time source.

After a successful NTP synchronization:

* The ESP32 system clock shall be updated.
* The timestamp of the last successful synchronization shall be stored.
* The internal RTC shall maintain the system time during Deep Sleep.
* The current time shall be obtained from the operating system time functions.

The firmware shall not manually advance timestamps based on the planned Deep Sleep duration.

The same timestamp source shall be used for:

* timer wake-ups
* GPIO wake-ups
* normal operation

The gateway shall perform an NTP synchronization when:

* no valid time reference exists
* the configured re-synchronization interval has elapsed since the last successful synchronization

The re-synchronization interval shall be configurable with a default re-synchronization interval of 24 hours.

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

## Timestamp Availability

The firmware shall maintain an internal time state.

A valid absolute timestamp is available when:

* at least one successful NTP synchronization has occurred since power-up

No valid absolute timestamp is available when:

* the device has never synchronized its clock
* the device has experienced complete power loss and has not yet re-synchronized

If no valid absolute timestamp is available, measurements and alarm events may still be buffered locally using a relative RTC-based time reference.

After a successful NTP synchronization, buffered measurements may be assigned absolute UTC timestamps using the elapsed RTC time between measurement acquisition and synchronization.

The reconstructed timestamp shall be calculated as:

```text
timestamp = ntp_time_at_sync - (rtc_time_at_sync - rtc_time_at_measurement)
```

Only measurements with valid absolute timestamps shall be transmitted to external systems.

## Consequences

### Positive

* No additional hardware required.
* Minimal BOM cost.
* Time survives Deep Sleep cycles.
* Supports offline data buffering.
* Supports accurate timestamping of GPIO alarm events.
* Supports reconstruction of measurements acquired before the first successful NTP synchronization.
* Simple firmware implementation.
* Uses native ESP32 timekeeping mechanisms.
* Automatic correction of RTC drift through periodic NTP synchronization.
* Reduced network traffic and power consumption compared to synchronizing before every transmission.

### Negative

* Time is lost after complete power removal.
* Long-term accuracy depends on RTC drift between NTP synchronizations.
* Reconstruction accuracy depends on the RTC time base.
* Buffered measurements cannot be assigned absolute timestamps until a successful NTP synchronization has occurred.
* Timestamp accuracy degrades gradually as the time since the last synchronization increases.

## Implementation Notes

The implementation shall use standard ESP32 time functions:

```cpp
time(nullptr);
gettimeofday();
```

The Time Manager shall store:

```cpp
lastNtpSyncTimestamp
```

and determine whether re-synchronization is required using:

```cpp
(time(nullptr) - lastNtpSyncTimestamp) >= ntpSyncInterval
```

A dedicated Time Manager module shall be responsible for:

* NTP synchronization
* RTC time tracking
* timestamp reconstruction
* synchronization interval management
* providing timestamps to application modules

The Time Manager shall provide a single interface for obtaining timestamps throughout the firmware.
