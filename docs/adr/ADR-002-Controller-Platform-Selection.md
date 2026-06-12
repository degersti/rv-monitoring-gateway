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

## Decision

The **ESP32-S3 DevKitC-1 N16R8** has been selected as the development platform.

## Rationale

Compared to the classic ESP32 DevKit V1, the ESP32-S3 provides several advantages:

- More advanced BLE capabilities
- 16 MB Flash memory
- 8 MB PSRAM
- Additional resources for OTA updates, web interfaces, MQTT, JSON processing, and data logging
- Strong support through both ESP-IDF and Arduino frameworks
- Better foundation for a future custom PCB based on the ESP32-S3-WROOM-1-N16R8 module

## Consequences

A carrier board will be designed specifically for the ESP32-S3 DevKitC-1 N16R8.

The existing ESP32 DevKit V1 will only be used as a test platform and will no longer serve as the basis for the project hardware design.

All pin assignments shall be defined centrally within a dedicated board configuration file.
