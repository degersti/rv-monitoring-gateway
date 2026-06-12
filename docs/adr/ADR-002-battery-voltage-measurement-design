# ADR-002: Battery Voltage Measurement Design

## Status

Accepted

## Context

The system shall measure the battery voltage of a 12 V of both, the house and the engine electrical system.

The measurement range shall cover at least up to 18 V in order to monitor battery state, charging voltages, and typical voltage peaks within the expected operating range.

The ESP32-S3 ADC must not be connected directly to the onboard voltage and shall be protected against overvoltage conditions.

## Decision

The onboard voltage shall be measured using a resistor divider combined with ADC protection circuitry.

Planned circuit:

```text
12V_BAT / Onboard Supply
   |
  R1 = 100 kΩ, 1 %
   |
   +---- Rser = 1 kΩ ---- ESP32-S3 ADC
   |              |
  R2 = 18 kΩ, 1 % C1 = 100 nF
   |              |
  GND            GND
```

Additionally, the ADC input shall be protected using Schottky clamp diodes:

```text
ADC ---- Schottky ---- 3V3
ADC ---- Schottky ---- GND
```

Example component:

```text
BAT54S
```

## Design Calculation

The resistor divider ratio is:

```text
U_ADC = U_BAT × R2 / (R1 + R2)
```

With:

```text
R1 = 100 kΩ
R2 = 18 kΩ
```

the resulting ADC voltages are:

| Battery Voltage | ADC Voltage |
|----------------|------------|
| 12.0 V | ~1.83 V |
| 14.4 V | ~2.20 V |
| 18.0 V | ~2.75 V |

This ensures that the ADC input voltage remains safely below the ESP32-S3 maximum input voltage of 3.3 V.

## Protection Measures

The design includes:

- Series resistor for ADC current limiting
- Capacitor for noise filtering and signal smoothing
- Schottky clamp diodes to 3V3 and GND
- Optional TVS diode on the battery input side
- Optional fuse or resettable PTC fuse ahead of the measurement circuit

## Consequences

The voltage measurement circuitry shall be integrated directly onto the carrier board.

Voltage conversion shall be performed in software using the known divider ratio:

```cpp
batteryVoltage = adcVoltage * (100000.0 + 18000.0) / 18000.0;
```

For improved accuracy, a calibration procedure shall be implemented during a later project phase.

## Future Hardware Considerations

The initial prototype focuses on basic voltage measurement and ADC protection.

If the project evolves into a long-term marine or RV product, a more comprehensive input protection concept should be evaluated, including:

- Reverse polarity protection
- Automotive-/marine-grade TVS suppression
- Fuse or electronic overcurrent protection
- Protection against load dump and transient events
- Improved EMC robustness
- ESD protection for external interfaces

These measures are not required for the first prototype but should be considered before productization.

## Alternatives Considered

### Direct ADC Connection

Advantages:

- Lowest component count

Disadvantages:

- Unsafe for the ESP32-S3 ADC
- No overvoltage protection
- Not suitable for automotive or marine environments

### Dedicated Voltage Monitoring IC

Advantages:

- Higher measurement accuracy
- Additional diagnostic features may be available

Disadvantages:

- Increased cost
- Additional PCB complexity
- Not required for the initial prototype phase
