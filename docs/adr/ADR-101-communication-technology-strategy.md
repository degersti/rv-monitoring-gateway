# ADR-101: Communication Technology Strategy

## Status

Accepted

## Context

The RV Monitoring Gateway is intended to operate as a stand-alone monitoring device for recreational vehicles and boats.

The system shall periodically transmit telemetry data and immediately report alarm events, regardless of whether a local Wi-Fi network is available.

The communication technology shall therefore provide:

- Wide-area network coverage
- Low power consumption
- High reliability
- Low hardware and operating costs
- Minimal installation effort for the end user

The communication layer shall remain independent from the underlying transport technology to support future communication modules.

Several communication technologies were evaluated.

### Technology Comparison

| Technology | Additional Infrastructure | Recurring Costs | Coverage | Power Consumption | Data Rate | Suitability |
|------------|---------------------------|-----------------|----------|-------------------|-----------|-------------|
| Wi-Fi | Wi-Fi Router | None | Local | Medium | High | Prototype / Fixed Installations |
| LTE Cat-M1 / NB-IoT | Integrated SIM / eSIM | Yes | Wide Area | Low | Low | **Recommended** |
| LoRaWAN | LoRaWAN Gateway or Public Network | Optional | Wide Area (limited by infrastructure) | Very Low | Very Low | Local IoT Networks |
| Bluetooth LE | Smartphone | None | Short Range | Very Low | Low | Local Configuration |
| Zigbee / Thread | Border Router | None | Local | Very Low | Low | Home Automation |
| Satellite | Satellite Subscription | High | Global | High | Very Low | Remote / Expedition Use |

### Wi-Fi

**Advantages**

- Integrated into the ESP32
- No recurring communication costs
- High data throughput
- Simple development and debugging

**Disadvantages**

- Requires an existing Wi-Fi infrastructure
- Unsuitable for autonomous operation while travelling

### LTE Cat-M1 / NB-IoT

**Advantages**

- Wide-area network coverage
- Optimized for IoT devices
- Low power consumption
- Reliable connectivity
- Suitable for unattended operation

**Disadvantages**

- Requires cellular connectivity
- Recurring service costs

### LoRa / LoRaWAN

**Advantages**

- Very low power consumption
- Long communication range
- License-free frequency band

**Disadvantages**

- Requires a LoRaWAN gateway or public network
- Public coverage is inconsistent
- Not suitable as the sole communication technology for a mobile stand-alone device

### Satellite Communication

**Advantages**

- Global coverage
- Independent of terrestrial infrastructure

**Disadvantages**

- High hardware costs
- High recurring service costs
- Increased power consumption

### Bluetooth LE

**Advantages**

- No infrastructure required
- Very low power consumption

**Disadvantages**

- Short communication range
- Suitable only for local configuration

## Decision

The communication subsystem shall be implemented as a modular abstraction layer independent of the underlying transport technology.

The selected communication architecture prioritizes:

1. Independence from customer-provided infrastructure
2. Wide-area availability
3. Low power consumption
4. Reliable telemetry and alarm transmission
5. A simple installation process for end users

For the initial prototype, Wi-Fi shall be used as the primary communication technology due to its simplicity and low development cost.

For the commercial product, LTE Cat-M1 or NB-IoT shall be considered the preferred wide-area communication technology, enabling autonomous operation without requiring customer-provided network infrastructure.

The commercial product shall preferably use an integrated eSIM managed through an IoT connectivity provider. This allows end users to operate the device without purchasing, installing or configuring their own SIM card or mobile subscription.

LoRaWAN, satellite communication and other communication technologies are considered future extension options but are not part of the initial product scope.

## Consequences

- The communication interface remains transport-independent.
- MQTT and backend services remain unchanged regardless of communication technology.
- The prototype can be developed and tested using Wi-Fi.
- The production device can migrate to LTE Cat-M1 / NB-IoT with minimal firmware changes.
- Future communication technologies can be integrated without affecting application logic.
