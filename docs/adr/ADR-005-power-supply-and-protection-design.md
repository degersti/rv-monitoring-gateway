# ADR-005: Power Supply and Protection Design

## Status

In Progress

## Context

The monitoring gateway requires a reliable power supply while operating in RV and marine environments.

The system shall be capable of measuring the onboard battery voltage and continue operating even when major vehicle or vessel systems are switched off.

The primary use case is installation on a 12 V house battery system, where the gateway continuously monitors battery voltage and periodically transmits telemetry data.

During development, testing, and future installation variants, the gateway may also be powered from an external 5 V source such as USB.

The initial prototype shall remain simple while preserving flexibility for future installations and product variants.

## Decision

The gateway shall support two power supply scenarios:

### Scenario 1: House Battery Powered Operation

The default installation shall use the house battery connection both as:

1. the battery voltage measurement input
2. the primary power source for the gateway

In this configuration, the gateway power supply is derived from the battery connection through an onboard power conversion stage.

Conceptual structure:

```text
HOUSE_BAT+
     |
     +---- Battery Voltage Measurement Circuit
     |
     +---- Power Protection and Conversion Circuit
                 |
                 +---- Gateway Electronics
```

### Scenario 2: External 5 V Powered Operation

The gateway shall also support operation from an external 5 V power source, such as USB.

In this configuration, the house battery connection remains available for voltage measurement but is not required as the primary power source.

Conceptual structure:

```text
HOUSE_BAT+
     |
     +---- Battery Voltage Measurement Circuit

USB / EXT_5V
     |
     +---- Gateway Electronics
```

The exact implementation of power-path management between battery-powered and externally powered operation is not part of this ADR and shall be defined during hardware design.

Possible implementation approaches may include:

- direct development-only powering via USB
- jumper selection
- diode OR-ing
- ideal diode circuits
- dedicated power-path management circuitry

## Power Conversion from 12V

The gateway shall be designed for operation from nominal 12 V battery systems.

Several power conversion approaches were considered, including:

- direct linear regulation (LDO)
- switching buck conversion
- multi-stage conversion (e.g. buck converter followed by local regulation)

Since the gateway is intended for long-term battery-powered operation, power efficiency is considered an important design criterion.

Linear regulators provide a simple implementation and low output noise but may result in significant power dissipation when converting from 12 V battery systems to the operating voltage required by the ESP32.

Switching regulators offer significantly higher efficiency and are therefore considered the preferred solution for future hardware revisions.

The final regulator topology and component selection shall be validated during hardware implementation and testing.

## Protection Measures

The power input design shall consider the following protection measures:

- reverse polarity protection
- overvoltage protection
- input filtering
- transient suppression suitable for RV and marine environments

The exact protection circuit and component selection shall be defined during schematic design and validated during hardware testing.

## Consequences

### Advantages

- Simple installation using only the house battery connection in the default configuration
- Supports both battery-powered and externally powered operation
- Preserves flexibility for future hardware revisions and product variants
- Allows battery voltage measurement independent of the selected power source
- Supports development and testing through standard USB power sources
- Enables future support for auxiliary batteries, solar-powered installations, or dedicated power supplies without major hardware redesign

### Disadvantages

- Additional flexibility increases design complexity compared to a single power-source architecture
- Power-path management between multiple power sources requires additional design consideration
- External power scenarios require additional validation and documentation
