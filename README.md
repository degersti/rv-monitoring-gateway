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

### Prototype 1 – Core Monitoring Prototype 

- [x] Battery voltage monitoring
- [x] Temperature and humidity monitoring
- [x] Water ingress / Smoke alarm detection
- [x] Wi-Fi communication
- [x] Backend connectivity (MQTT)
- [x] Data buffering
- [x] Time management
- [x] Deep Sleep support
- [x] System state machine
- [x] Watchdog
- [x] Firmware integration
- [x] Prototype 1 validation

### Prototype 2 – Enhanced Connectivity

- [x] Migrate data buffering to SD Card
- [ ] Cellular communication (LTE / NB-IoT)
- [ ] MQTT using TLS, QoS 1/2 and Websockets
- [ ] Bluetooth configuration
- [ ] Writing Log and Measurements to SD Card
- [ ] GPS positioning and geofencing



### Optional – Extended Monitoring

- OTA firmware updates
- Bluetooth connection to integrated battery management
- Bluetooth connection to solar charger 
- Shore power monitoring
- CAN-/NMEA-Bus integration
- Additional environmental sensors

---

## Current Status

🚧 Prototype 2 Development

### In Progress

- Buffering on SD-Card

### Next Milestone

**Prototype 2 running unattended on real hardware**

## License

License to be defined.
