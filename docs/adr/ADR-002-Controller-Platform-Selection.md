# ADR-002: Controller Platform Selection

## Status

Accepted

## Context

A stable hardware platform is required for the boat monitoring project that supports both Phase 1 and Phase 2 without requiring major hardware redesigns between development stages.

Planned functionality includes:

- BLE communication with the Liontron BMS
- Wi-Fi-based communication in Phase 1
- Future LTE connectivity via UART
- Sensor interfaces via I²C
- microSD card for configuration, logging, and diagnostic data
- OTA (Over-The-Air) firmware updates
- Potential future product version based on a custom PCB

Several controller platforms were considered, including Arduino-based boards, Raspberry Pi systems, STM32 microcontrollers, and ESP32 variants.

## Decision

The **ESP32-S3 DevKitC-1 N16R8** has been selected as the development platform.

## Rationale

### Why ESP32?

The ESP32 family provides an attractive balance of performance, connectivity, power consumption, development effort, and cost.

Compared to alternative platforms:

- Integrated Wi-Fi and BLE eliminate the need for additional communication modules.
- Significantly lower power consumption than Raspberry Pi-based solutions.
- Lower hardware cost and reduced system complexity.
- Mature software ecosystem with strong support from both ESP-IDF and Arduino frameworks.
- Large community and extensive documentation.
- Suitable for battery-powered and permanently installed embedded applications.
- Available both as development boards and production-ready modules, simplifying the transition from prototype to custom hardware.

### Why ESP32-S3?

Compared to the existing ESP32 DevKit V1, the ESP32-S3 provides several advantages:

- More advanced BLE capabilities.
- 16 MB Flash memory.
- 8 MB PSRAM.
- Additional resources for OTA updates, web interfaces, MQTT, JSON processing, and data logging.
- Better long-term support for future software extensions.
- Better foundation for a future custom PCB based on the ESP32-S3-WROOM-1-N16R8 module.

## Consequences

A carrier board will be designed specifically for the ESP32-S3 DevKitC-1 N16R8.

The existing ESP32 DevKit V1 will only be used as a test platform and will no longer serve as the basis for the project hardware design.

All pin assignments shall be defined centrally within a dedicated board configuration file.

Future hardware revisions should remain compatible with the ESP32-S3 module family to minimize software migration effort.
