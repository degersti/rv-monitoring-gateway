# ADR-006: Power Converter Module Selection

## Status

Accepted

## Context

The RV Monitoring Gateway shall be powered from a 12 V onboard electrical system.

The system is intended to operate unattended for extended periods aboard a recreational vehicle or vessel. Therefore, the power supply design must be reliable, simple to assemble, and suitable for a soldered Prototype 1 carrier board.

Prototype 1 is not intended to use a custom PCB. Instead, it shall be built as a soldered carrier board using an ESP32-S3 DevKitC-1 and external modules.

The ESP32-S3 DevKitC-1 requires a stable 5 V supply when powered through its 5 V / VIN input.

A custom buck converter design would require additional component selection, layout considerations, compensation, thermal validation, and testing. This would add unnecessary complexity at the current project stage.

## Decision

Prototype 1 shall use a ready-made 12 V to 5 V switching regulator module.

For the first implementation, a Traco TSR Series switching regulator is selected as the preferred solution.

The regulator shall provide a regulated 5 V output for the ESP32-S3 DevKitC-1 and connected peripherals.

The power input path shall follow this structure:

```text
12 V onboard supply
   |
Fuse
   |
Input connector
   |
Reverse polarity protection
   |
Transient voltage protection
   |
Traco TSR switching regulator
   |
5 V supply to ESP32-S3 DevKitC-1
```

## Rationale

The Traco TSR Series regulators provide a robust and compact industrial-grade solution for generating a stable 5 V supply from the onboard electrical system.

Advantages include:

- high efficiency
- compact SIP package
- minimal external circuitry
- industrial quality and reliability
- simple integration on a soldered carrier board
- reduced design risk compared to a custom switching regulator

The regulator can be treated as a functional building block within the prototype and does not require detailed switching regulator design.

This allows development effort to focus on gateway functionality:

- sensor acquisition
- battery voltage measurement
- alarm input handling
- WiFi communication
- MQTT telemetry
- unattended operation

## Alternatives Considered

### MP1584 Module

MP1584-based modules are inexpensive, compact, and widely available.

They were not selected because quality varies significantly between manufacturers and long-term reliability is difficult to assess.

### LM2596 Module

LM2596 modules are inexpensive and widely available.

They were not selected because they are physically larger and generally less attractive for a compact installed system.

### Custom Buck Converter Circuit

A custom buck converter circuit was rejected for Prototype 1.

Although it may be appropriate for a later custom PCB, it would add unnecessary complexity at this stage and would require careful layout, component selection, and validation.

## Consequences

The switching regulator is treated as a module-level component in Prototype 1.

The schematic does not need to include the internal regulator IC, inductor, feedback divider, or compensation network of the regulator.

Only the external power path and protection components are documented.

Future hardware revisions may replace the regulator with:

- a lower-power switching regulator
- an automotive-qualified regulator
- a custom PCB-integrated power supply design

## Open Points

The exact Traco TSR regulator variant shall be selected before hardware assembly.

The following parameters shall be verified:

- input voltage range
- output voltage (5 V)
- output current capability
- efficiency
- thermal behavior
- quiescent current
- suitability for long-term unattended operation

The final installed device shall be tested with the actual onboard electrical system before being left unattended.
