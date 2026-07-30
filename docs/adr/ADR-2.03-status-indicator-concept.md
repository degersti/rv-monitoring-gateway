# ADR-2.03: Status Indicator Concept

## Status

Accepted

> Supersedes the previous version of ADR-1.05.

---

## Context

The RV Monitoring Gateway requires a simple and intuitive method to communicate its current operating state during development, installation, commissioning, and future field operation.

The status indication shall provide visual feedback regarding:

- current operating mode,
- communication status,
- backend connectivity,
- active alarms,
- internal system faults.

The original Prototype 1 implementation used the single onboard RGB LED of the ESP32-S3 DevKitC-1.

Because only one indicator was available, all system states had to be merged into a single prioritized state machine.

With the introduction of SD-card-based persistent storage, the gateway may continue normal operation while one or more non-fatal system faults are active.

Example:

- SD card unavailable
- communication via Wi-Fi or LTE available
- MQTT connected
- measurements successfully transmitted

A single prioritized RGB indicator can no longer represent all relevant information simultaneously.

Prototype 2 shall therefore introduce dedicated status indicators.

---

## Decision

Prototype 2 shall provide four independent logical status indicators.

| Indicator | Purpose |
|------------|---------|
| STATUS | Current operating mode |
| NETWORK | Network connection |
| BACKEND | MQTT / backend connection |
| ERROR | Active alarms and system faults |

Each indicator represents an independent aspect of the system.

The indicators shall operate simultaneously.

---

# STATUS Indicator

The STATUS indicator represents the current operating mode of the gateway.

Typical operating modes include:

- Boot
- Serial debug enabled
- Normal operation
- Bluetooth configuration mode
- Firmware update (future operation mode)

The STATUS indicator shall be switched off during Deep Sleep.

### Blink Codes

| State | Pattern |
|--------|---------|
| Boot | Slow blink |
| Serial debug enabled | Fast blink |
| Normal operation | Steady on |
| Bluetooth configuration | Long blink |
| Deep Sleep | Off |

---

# NETWORK Indicator

The NETWORK indicator represents the availability of an external network connection.

This may be either Wi-Fi or LTE.

The communication technology is independent of the logical indicator.

### Blink Codes

| State | Pattern |
|--------|---------|
| Disconnected | Off |
| Connecting | Slow blink |
| Connected | Steady on |

Future firmware versions may distinguish Wi-Fi and LTE by different blink patterns without requiring hardware changes.

---

# BACKEND Indicator

The BACKEND indicator represents the application-level connection to the configured backend.

Initially, the BACKEND indicator represents the MQTT connection state. 

In a future version, the backend shall periodically send an acknowledgment message. The gateway shall use this acknowledgment to verify not only that the MQTT connection is established, but also that the backend application is operational and actively receiving messages.

### Blink Codes

| State | Pattern |
|--------|---------|
| Disconnected | Off |
| Connecting | Slow blink |
| Connected | Steady on |
| Data transmission | Short flash |

---

# ERROR Indicator

The ERROR indicator represents situations requiring user attention.

This includes both:

- system faults
- alarm conditions

Examples include:

- water ingress alarm
- smoke alarm
- SD card unavailable
- SD write error
- sensor failure
- internal initialization failure
- future fault conditions

### Blink Codes

| State | Pattern |
|--------|---------|
| No active error | Off |
| Active alarm | Slow blink |
| Active system fault | Error blink code |

The assignment of individual error blink codes shall be defined in a separate Architecture Decision Record covering the system error management concept.

If multiple errors are active simultaneously, the corresponding error codes shall be displayed sequentially.

The sequencing and prioritization of active errors is outside the responsibility of the status indicator module.

---

## Firmware Architecture

The status indication shall continue to be implemented by the `status_indicator.cpp` module.

Unlike the previous implementation, the module shall support multiple independent indication categories simultaneously.

The module shall no longer operate as a single prioritized system state machine.

Instead, each logical indicator shall maintain its own independent indication state.

The `status_indicator.cpp` module is responsible for:

- LED control
- blink timing

The module is not responsible for:

- collecting active system faults,
- prioritizing multiple faults,
- determining the order of displayed error codes.

These responsibilities belong to a dedicated system error management component.

---

## Consequences

### Advantages

- Multiple independent indication states can be displayed simultaneously.
- Alarm conditions remain visible independently of communication status.
- The architecture is scalable for future operating modes.
- Additional communication technologies can be added without hardware modifications.
- Error handling remains separated from visual indication.

### Disadvantages

- Prototype 2 requires four additional external LEDs.
- Additional GPIOs are required.

---

## Notes

This ADR defines the logical status indication concept only.

The following topics are intentionally excluded and shall be defined separately:

- system error management,
- error prioritization,
- persistent error storage,
- communication path selection,
- Wi-Fi/LTE fallback strategy.
