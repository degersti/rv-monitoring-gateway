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

## Low-Power Measurement Consideration

Since the resistor divider is connected to the battery, it creates a continuous current path from the battery to ground.

With the planned resistor values:

```text
R1 = 100 kΩ
R2 = 18 kΩ
```

the divider current is approximately:

```text
I = 12 V / (100 kΩ + 18 kΩ)
I ≈ 102 µA
```

This current is small, but it is continuously drawn from the battery, even while the ESP32-S3 is in deep sleep.

For the initial prototype, this may be acceptable. However, for long-term unattended operation, especially in marine or RV installations, the voltage divider should be evaluated as part of the overall power budget.

A future hardware revision may switch the voltage divider using a MOSFET or load switch so that the divider is only active during measurement.

Conceptual switched divider:

```text
12V_BAT / Onboard Supply
   |
 [Switch / MOSFET]
   |
  R1
   |
   +---- Rser ---- ESP32-S3 ADC
   |        |
  R2       C1
   |        |
  GND      GND
```

The firmware would then:

1. enable the measurement circuit
2. wait for the ADC input to settle
3. take the voltage measurement
4. disable the measurement circuit again

This reduces continuous battery drain but increases hardware and firmware complexity.

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

### Switched Voltage Divider

Advantages:

- Reduces continuous battery drain during deep sleep
- Better suited for long-term unattended operation
- Allows the measurement circuit to be powered only when required

Disadvantages:

- Increased circuit complexity
- Requires an additional MOSFET or load switch
- Requires firmware control and ADC settling time
- Additional failure modes compared to a permanently connected divider

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
  
