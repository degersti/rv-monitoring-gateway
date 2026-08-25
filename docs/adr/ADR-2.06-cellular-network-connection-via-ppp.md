# ADR-2.06 – Cellular Network Connection via PPP

## Status

Accepted

## Context

## Context

The gateway shall support cellular connectivity in addition to WiFi. This allows the gateway to communicate with the backend even when no WiFi network is available.

A **SIM7000E cellular modem** is available for this purpose.

The cellular connection should provide standard IP connectivity so that services such as MQTT can use it without depending on modem-specific communication methods.


## Decision

Cellular IP connectivity shall be established using **PPP (Point-to-Point Protocol)** between the ESP32 and the cellular modem.

Similar to the existing `wifi_manager`, a dedicated `cellular_manager` shall handle the cellular connection. Both network types can then be managed through a common network interface.

The resulting architecture is:

```text
Application Services
        │
        ▼
 Network Manager 
        │
        ▼
 Cellular Manager
        │
        ▼
       PPP
        │
        ▼
 Cellular Modem
        │
        ▼
 Mobile Network
```

## Rationale

PPP allows the ESP32 to use the cellular connection as a standard IP connection. This avoids modem-specific communication in higher-level services and allows the cellular connection to be handled in the same way as WiFi.

## Alternatives Considered

### Modem-Controlled Network Connections

The cellular modem could manage application network connections directly.

This approach was rejected because it would couple higher-level communication services to modem-specific functionality and create a separate communication path for cellular connectivity.

### Application-Specific Cellular Connectivity

Individual services such as MQTT could establish and manage their own cellular connection.

This approach was rejected because network connectivity and application communication are separate architectural responsibilities. It would also duplicate network-related logic across application services.

### Separate Cellular Application Interface

Cellular connectivity could expose a dedicated interface independent of the WiFi networking architecture.

This approach was rejected because higher-level services should not need to distinguish between available network technologies.

## Consequences

### Positive

* Higher-level services remain independent of modem-specific communication.
* Cellular connectivity can use the same network abstraction as WiFi.
* Cellular-specific responsibilities remain contained within the cellular_manager.
* Changes to the cellular modem or cellular technology have limited impact on higher-level components.
### Negative

* Establishing a cellular connection generally requires more time and energy than establishing a WiFi connection.
* The cellular modem, mobile network connection, and PPP connection must be managed together.
* Connection failures can originate from different parts of the cellular communication path.

## Scope

This ADR defines the architectural strategy for providing **cellular IP connectivity through PPP**.

It does not define:

* WiFi connectivity,
* network selection or switching,
* fallback policies,
* application protocol behavior,
* retry timing,
* modem initialization sequences,
* modem command handling,
* or detailed connection state machines.

Selection and coordination between WiFi and cellular network transports are defined separately in **ADR-2.07 – Unified Network Management and WiFi/Cellular Switching**.
