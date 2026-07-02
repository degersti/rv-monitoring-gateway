# ADR-011: Runtime Watchdog

## Status

Accepted

## Context

The RV Monitoring Gateway is intended to operate unattended for extended periods.

Unexpected software failures may occur due to:

- software bugs
- communication library deadlocks
- memory corruption
- unexpected peripheral behavior
- future firmware extensions

A system that stops executing without recovery could fail to detect alarms or transmit important events.

The gateway therefore requires an automatic recovery mechanism.

## Decision

The firmware shall use the ESP32 hardware watchdog to detect software lockups.

The watchdog shall supervise the main application execution.

The Runtime Manager shall be responsible for initializing and periodically feeding the watchdog.

The application shall reset ("feed") the watchdog only after one complete execution cycle has finished successfully.

If the watchdog is not fed within the configured timeout period, the ESP32 shall automatically reset.

No software attempt shall be made to recover from a watchdog timeout before the reset occurs.

## Rationale

The watchdog provides:

- automatic recovery from deadlocks
- recovery from unexpected infinite loops
- protection against blocked communication libraries
- higher long-term system reliability

Feeding the watchdog only after a successful application cycle ensures that partially blocked execution cannot indefinitely prevent a reset.

Assigning watchdog ownership to the Runtime Manager keeps the responsibility centralized and prevents individual modules from hiding blocked or incomplete system execution by feeding the watchdog independently.

Using the hardware watchdog avoids implementing complex software health monitoring mechanisms.

## Consequences

### Advantages

- Automatic recovery without user intervention.
- Increased system availability.
- Simple implementation.
- Independent of individual software modules.
- Detects failures regardless of their origin.
- Clear ownership through the Runtime Manager.

### Disadvantages

- The root cause of a watchdog reset is not automatically known.
- Incorrect timeout configuration may cause unnecessary resets.
- Long blocking operations must complete within the watchdog timeout.
- Individual modules must not feed the watchdog directly.

## Implementation Notes

The watchdog shall be initialized during system startup by the Runtime Manager.

The timeout shall be chosen to exceed the maximum expected execution time of one application cycle, including:

- sensor acquisition
- Wi-Fi connection
- MQTT communication
- local data storage
- housekeeping tasks

Each completed application cycle shall reset the watchdog timer through the Runtime Manager.

Sensor, communication, storage, and utility modules shall not feed the watchdog directly.

Future modules shall avoid long blocking operations. Operations that may require significant execution time should periodically yield or be restructured into non-blocking state machines where appropriate.

## Future Considerations

Future firmware versions may extend watchdog supervision to individual FreeRTOS tasks if the software architecture evolves from a primarily sequential execution model to multiple independent tasks.

Watchdog reset causes may later be stored in persistent runtime metadata for diagnostics.