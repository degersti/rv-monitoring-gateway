# ADR-009: Runtime Debug and Deep Sleep Strategy
## Status

Accepted

## Context

The monitoring gateway is designed to operate autonomously for extended periods with minimal power consumption. Deep sleep is therefore considered part of the normal runtime behavior rather than a special operating mode.

During development and installation, however, access to serial debug output is required. Since the ESP32 immediately starts executing after reset, there is normally insufficient time to connect a serial monitor before the boot messages are printed.

An early design considered using a dedicated GPIO to disable deep sleep entirely during debugging. This approach was rejected because it changes the runtime behavior and makes testing different from normal operation.

## Decision

The firmware shall always follow the same runtime behavior regardless of whether debugging is enabled.

A dedicated debug input (SERIAL_DEBUG_PIN) is sampled during boot.

If the pin is active:

* Serial communication is initialized.
* The firmware waits for a configurable delay (SERIAL_MONITOR_WAIT_MS) before printing any debug messages.
* Normal program execution continues afterwards.
* Deep sleep remains fully enabled.

If the pin is inactive:

* Serial communication is not initialized.
* No debug output is generated.
* The firmware immediately continues normal execution.

Deep sleep is therefore never disabled by the debug input.

Wake-up sources are configured independently of the debug mode and currently consist of:

* Timer wake-up
* External interrupt wake-up (GPIO12 / GPIO13)

## Consequences

### Advantages

Normal runtime behavior is identical in production and debug mode.
No additional hardware or software path is required to disable deep sleep.
Serial logging is available during installation and development.
No serial interface is initialized during normal operation, reducing startup overhead and power consumption.
The runtime behavior remains deterministic and easy to test.

### Disadvantages

Boot time is increased by the configured delay when debug mode is enabled.
Debugging requires the dedicated debug input to be asserted during boot.

## Future Considerations

Future firmware versions may introduce a dedicated Configuration Mode (e.g. via Bluetooth Low Energy).

Configuration Mode may intentionally keep the controller awake to allow runtime parameter modification such as:

* Deep sleep enable/disable
* Wake-up interval
* Network configuration
* Device settings

This mode is considered independent from the serial debug mechanism and will be specified in a separate ADR.
