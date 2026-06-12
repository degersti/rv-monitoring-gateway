# ADR-003: Temperature and Humidity Sensor Selection

## Status

Accepted

## Context

The rv monitoring project requires measurement of temperature and humidity.

Considered sensors:

- AM2302 / DHT22
- BMP180
- SHT31

The sensor solution should be reliable, easy to integrate, and suitable for future PCB integration.

## Decision

The **SHT31** shall be used as the primary temperature and humidity sensor.

## Rationale

The SHT31 is better suited for this project than the AM2302/DHT22 because it:

- communicates via I2C
- provides a more reliable and modern interface
- integrates naturally into a shared sensor bus architecture
- measures both temperature and relative humidity
- is better suited for clean PCB integration and future productization

## Consequences

The carrier board shall provide an I2C sensor interface.

Planned signals:

- 3V3
- GND
- I2C_SDA
- I2C_SCL

Multiple I2C sensors may be connected to the same bus, provided that address conflicts are avoided.

## Alternatives Considered

### AM2302 / DHT22

Advantages:

- Widely available
- Simple to use

Disadvantages:

- Proprietary single-wire communication protocol
- Lower reliability compared to modern I2C sensors
- Less suitable for multi-sensor architectures
- Less attractive for future PCB integration

### BMP180

Advantages:

- Provides barometric pressure measurements
- Uses I2C

Disadvantages:

- No humidity measurement
- Temperature measurement is primarily intended for pressure compensation
- Does not fulfill the project's humidity sensing requirements
