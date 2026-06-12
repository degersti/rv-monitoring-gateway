# ESP32-S3 Prototype Pin Assignment

## Status

Draft

## Purpose

This document defines the initial GPIO pin assignment for the RV Monitoring Gateway prototype.

The goal is to provide a stable wiring reference for the breadboard / prototype carrier board before moving to firmware implementation or a custom PCB.

This pin assignment is intended for the ESP32-S3 DevKitC-1 N16R8.

---

## Pin Assignment Overview

| GPIO | Prototype Assignment | Status | Notes |
|------|----------------------|----------|-------|
| GPIO0 | Not used | Avoid | Boot / strapping related |
| GPIO1 | Free | Available | Can be used if needed |
| GPIO2 | Free | Available | Can be used if needed |
| GPIO3 | Not used | Avoid | Avoid for prototype stability |
| **GPIO4** | **House battery voltage** | **Used** | **ADC input** |
| **GPIO5** | **Engine battery voltage** | **Used** | **ADC input** |
| GPIO6 | Reserved analog input | Reserved | ADC-capable |
| GPIO7 | Reserved analog input | Reserved | ADC-capable |
| **GPIO8** | **I2C SDA** | **Used** | **SHT31 and future I2C sensors** |
| **GPIO9** | **I2C SCL** | **Used** | **SHT31 and future I2C sensors** |
| **GPIO10** | **SD Card CS** | **Reserved** | **SPI interface for SD card module** |
| **GPIO11** | **SD Card MOSI** | **Reserved** | **SPI interface for SD card module** |
| **GPIO12** | **SD Card SCK** | **Reserved** | **SPI interface for SD card module** |
| **GPIO13** | **SD Card MISO** | **Reserved** | **SPI interface for SD card module** |
| **GPIO14** | **Water ingress sensor** | **Used** | **Digital input / interrupt** |
| **GPIO15** | **Smoke alarm input** | **Used** | **Digital input / interrupt** |
| **GPIO16** | **LTE Power Enable** | Reserved | **LTE modem power control** |
| **GPIO17** | **LTE UART TX** | **Reserved** | **ESP32 TX → LTE RX** |
| **GPIO18** | **LTE UART RX** | **Reserved** | **ESP32 RX ← LTE TX** |
| GPIO19 | Free | Available | Future expansion |
| GPIO20 | Free | Available | Future expansion |
| GPIO21 | Free | Available | Future expansion |
| GPIO26 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO27 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO28 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO29 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO30 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO31 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO32 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO33 | Free | Available | Future expansion |
| GPIO34 | Free | Available | Future expansion |
| GPIO35 | Free | Available | Future expansion |
| GPIO36 | Free | Available | Future expansion |
| GPIO37 | Free | Available | Future expansion |
| GPIO38 | Status LED (Board-dependent) | Optional | Some DevKit revisions |
| GPIO39 | Free | Available | Future expansion |
| GPIO40 | Free | Available | Future expansion |
| GPIO41 | Free | Available | Future expansion |
| GPIO42 | Free | Available | Future expansion |
| GPIO43 | USB / Serial | Avoid | Programming / debug interface |
| GPIO44 | USB / Serial | Avoid | Programming / debug interface |
| GPIO45 | Not used | Avoid | Strapping related |
| GPIO46 | Not used | Avoid | Strapping / input-only limitations |
| GPIO47 | Free | Available | Future expansion |
| GPIO48 | Status LED (Board-dependent) | Optional | Some DevKit revisions |

---
## Prototype Wiring Overview

```text
ESP32-S3 DevKitC-1
│
├── I2C Bus
│   ├── GPIO8  -> SDA
│   └── GPIO9  -> SCL
│
├── Analog Inputs
│   ├── GPIO4  -> House battery voltage divider
│   ├── GPIO5  -> Engine battery voltage divider
│   ├── GPIO6  -> Reserved for voltage devider
│   └── GPIO7  -> Reserved for voltage devider
│
├── Digital Inputs
│   ├── GPIO14  -> Water ingress sensor
│   └── GPIO15  -> Smoke alarm input
|  
│
├── Reserved LTE Interface
│   ├── GPIO17 -> LTE UART TX
│   ├── GPIO18 -> LTE UART RX
│   └── GPIO16 -> LTE power enable
│
└── Reserved SD Card Interface
    ├── GPIO10 -> SD CS
    ├── GPIO11 -> SD MOSI
    ├── GPIO12 -> SD SCK
    └── GPIO13 -> SD MISO
```

---

## I2C Bus

The I2C bus is used for low-speed digital sensors.

Initial devices:

- SHT31 temperature and humidity sensor

Possible future devices:

- RTC
- external ADC
- current sensor
- pressure sensor
- additional environmental sensors

```text
ESP32-S3      SHT31
GPIO8   ---> SDA
GPIO9   ---> SCL
3V3     ---> VCC
GND     ---> GND
```

Pull-up resistors may already be present on breakout boards. If not, external pull-ups of approximately 4.7 kΩ to 3.3 V shall be added.

---

## ADC Inputs

Two ADC inputs are reserved for onboard voltage measurement.

```text
GPIO4 -> House battery voltage
GPIO5 -> Engine battery voltage
```

Both ADC inputs shall only be connected through a resistor divider and protection circuitry.

The ESP32-S3 ADC pins must never be connected directly to the 12 V system.

---

## Digital Inputs

Two digital inputs are reserved for basic hazard detection.

```text
GPIO15 -> Water ingress sensor
GPIO17 -> Smoke alarm input
```

The final input circuit depends on the selected sensor type.

For prototype testing, simple dry-contact or logic-level inputs may be used.

For real onboard installation, input protection and possible galvanic isolation should be considered.

---

## Reserved LTE Interface

A complete UART interface is reserved for a future LTE modem.

```text
GPIO17 -> ESP32 TX / LTE RX
GPIO18 -> ESP32 RX / LTE TX
GPIO16 -> LTE power enable
```

These pins shall not be used for other prototype functions.

---

## Reserved SD Card Interface

A complete SPI interface is reserved for a future SD card module.

```text
GPIO10 -> CS
GPIO11 -> MOSI
GPIO12 -> SCK
GPIO13 -> MISO
```

The SD card is not required for the first prototype but may later be used for logging, configuration, or offline buffering.

---

## Pins Intentionally Avoided

The following pins are intentionally avoided for normal prototype functions:

| GPIO | Reason |
|------|--------|
| GPIO0 | Boot / strapping related |
| GPIO3 | Avoid for prototype stability |
| GPIO45 | Strapping related |
| GPIO46 | Strapping / input-only limitations depending on board |

This reduces the risk of boot problems during development.

---

## First Prototype Scope

The first hardware prototype shall only use:

- ESP32-S3 DevKitC-1
- SHT31 temperature and humidity sensor
- house battery voltage input
- engine battery voltage input
- smoke detector input
- water detector input

The initial firmware goal is:

```text
Read SHT31
Read battery voltage ADC
Publish values via MQTT
View data on website
```

LTE modem, SD card and final enclosure design are outside the scope of the first prototype.

---

## Notes

This pin assignment is not final for production hardware.

It is intended to keep the prototype wiring stable while allowing later migration to:

- a soldered prototype board
- a custom carrier board
- a final PCB revision
