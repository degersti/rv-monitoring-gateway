# RV Monitoring Gateway

A lightweight and modular ESP32-based IoT platform for remote monitoring of recreational vehicles and vessels.

## Overview

The RV Monitoring Gateway is intended to provide location-independent access to essential operational data from recreational vehicles and vessels while minimizing installation effort, power consumption, and dependency on proprietary ecosystems.

The project is currently in an early development phase and follows an incremental development approach. Individual hardware and software components are validated through dedicated proof-of-concept projects before being integrated into the final gateway firmware.

## Repository Structure

- `docs/` – Whitepaper and project documentation
- `development/` – Individual proof-of-concept projects
- `firmware/` – Integrated gateway firmware

## Development Roadmap

### Phase 1 – Core Monitoring Prototype

- [x] Battery voltage monitoring
- [x] Temperature and humidity monitoring
- [x] Water ingress detection
- [x] Smoke alarm integration
- [x] Wi-Fi communication
- [x] Backend connectivity (MQTT)
- [x] Historical data storage
- [x] Time management
- [x] Deep Sleep support
- [x] System state machine
- [x] Watchdog
- [ ] Firmware integration
- [ ] Prototype validation

### Phase 2 – Enhanced Connectivity

- Cellular communication (LTE / NB-IoT)
- MQTT using TLS, QoS 1/2 and Websockets
- Bluetooth configuration
- Improved device management
- OTA firmware updates

### Phase 3 – Extended Monitoring

- GPS positioning and geofencing
- Writing Log and Measurements to SD Card
- Battery management system integration
- Solar charge controller integration
- Shore power monitoring
- Additional environmental sensors

---

## Current Status

🚧 Prototype 1 Development

### Completed

- Whitepaper created
- GitHub repository established
- ESP32 development environment configured
- PlatformIO project setup
- Modular firmware architecture
- Logging module
- Runtime manager
- Time manager
- Deep Sleep manager
- Wi-Fi manager
- MQTT manager
- Persistent measurement buffer
- Sensor drivers
- Hardware abstraction completed
- Multiple Architecture Decision Records (ADRs)

### In Progress

- Firmware integration

### Next Milestone

**Prototype 1 running unattended on real hardware**

Features:

- Periodic sensor measurements
- MQTT data transmission
- Automatic buffering during communication outages
- Deep Sleep power optimization
- Wake-up on alarm events
- Automatic recovery after reset or power loss

## License

License to be defined.
