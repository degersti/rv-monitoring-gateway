# ESP32-S3 Prototype 2 - Pin Assignment

## Status

Accepted

## Purpose

This document defines the GPIO pin assignment for Prototype 2 of the RV Monitoring Gateway.

The goal is to provide a stable hardware interface definition for firmware development, breadboard verification, and the Prototype 2 carrier board before transitioning to the custom PCB.

This pin assignment is intended for the ESP32-S3 DevKitC-1 N16R8.

---

## Pin Assignment Overview

| GPIO | Prototype Assignment | Status | Notes |
|------|----------------------|--------|-------|
| GPIO0 | Not used | Avoid | Boot / strapping related |
| **GPIO1** | **Serial Debug Enable** | **Used** | **Active LOW digital input for enabling serial debug output** |
| **GPIO2** | **Configuration Button** | **Used** | **Bluetooth configuration, factory reset and Deep Sleep wake-up** |
| GPIO3 | Not used | Avoid | Avoid for prototype stability |
| **GPIO4** | **House battery voltage** | **Used** | **ADC input** |
| **GPIO5** | **Engine battery voltage** | **Used** | **ADC input** |
| GPIO6 | Reserved analog input | Reserved | ADC-capable |
| GPIO7 | Reserved analog input | Reserved | ADC-capable |
| **GPIO8** | **LTE Power Enable** | **Used** | **Controls LTE high-side switch** |
| **GPIO9** | **STATUS LED** | **Used** | **External operating status indicator** |
| **GPIO10** | **NETWORK LED** | **Used** | **External Wi-Fi / LTE connection indicator** |
| **GPIO11** | **BACKEND LED** | **Used** | **External MQTT / backend connection indicator** |
| **GPIO12** | **ERROR LED** | **Used** | **External alarm and system fault indicator** |
| **GPIO13** | **Water ingress sensor** | **Used** | **Digital input / wake-up source** |
| **GPIO14** | **Smoke alarm input** | **Used** | **Digital input / wake-up source** |
| **GPIO15** | **LTE PWR** | **Used** | **???** |
| **GPIO16** | **LTE DTR** | **Used** | **LTE sleep / wake control** |
| **GPIO17** | **LTE UART TX** | **Used** | **ESP32 TX → LTE RXD** |
| **GPIO18** | **LTE UART RX** | **Used** | **ESP32 RX ← LTE TXD** |
| GPIO19 | USB D- | Avoid | Native USB |
| GPIO20 | USB D+ | Avoid | Native USB |
| **GPIO21** | **SD Card Power Enable** | **Reserved** | **Controls future SD high-side power switch** |
| GPIO26 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO27 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO28 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO29 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO30 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO31 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO32 | Not used | Avoid | Internal Flash / PSRAM |
| GPIO33 | Free | Available | Future expansion |
| GPIO34 | Free | Available | Future expansion |
| **GPIO35** | **SD Card SCK** | **Used** | **SPI interface** |
| **GPIO36** | **SD Card MOSI** | **Used** | **SPI interface** |
| **GPIO37** | **SD Card CS** | **Used** | **SPI interface** |
| GPIO38 | Status LED | Optional | External status LED |
| GPIO39 | Free | Available | Future expansion |
| **GPIO40** | **I2C SCL** | **Used** | **SHT31 and future I2C devices** |
| **GPIO41** | **I2C SDA** | **Used** | **SHT31 and future I2C devices** |
| GPIO42 | Free | Available | Future expansion |
| GPIO43 | USB / Serial TX | Avoid | Programming / debug |
| GPIO44 | USB / Serial RX | Avoid | Programming / debug |
| GPIO45 | Not used | Avoid | Strapping related |
| GPIO46 | Not used | Avoid | Strapping related |
| **GPIO47** | **SD Card MISO** | **Used** | **SPI interface** |
| GPIO48 | RGB Status LED | Optional | Onboard RGB LED |

---

## Prototype Wiring Overview

```text
3V3                  ----|  ESP32 |--- GND
3V3                  ----|        |--- U0TXD / GPIO43
RST                  ----|        |--- U0RXD / GPIO44
GPIO04 / House ADC   ----|        |--- GPIO01 / Serial Debug Enable
GPIO05 / Engine ADC  ----|        |--- GPIO02 / Config Button
GPIO06 / free        ----|        |--- GPIO42
GPIO07 / free        ----|        |--- GPIO41 / I2C SDA
GPIO15 / LTE PWR     ----|        |--- GPIO40 / I2C SCL
GPIO16 / LTE DTR     ----|        |--- GPIO39
GPIO17 / LTE TX      ----|        |--- GPIO38 / Status LED
GPIO18 / LTE RX      ----|        |--- GPIO37 / SD CS
GPIO08 / LTE PWR EN  ----|        |--- GPIO36 / SD MOSI
GPIO03 / avoid       ----|        |--- GPIO35 / SD SCK
GPIO46 / avoid       ----|        |--- GPIO00 / BOOT
GPIO09 / Status LED  ----|        |--- GPIO45 / avoid
GPIO10 / Network LED ----|        |--- GPIO48 / RGB LED unused
GPIO11 / Backend LED ----|        |--- GPIO47 / SD MISO
GPIO12 / Error LED   ----|        |--- GPIO21 / SD PWR EN
GPIO13 / Water       ----|        |--- GPIO20 / USB D+
GPIO14 / Smoke       ----|        |--- GPIO19 / USB D-
5V0                  ----|        |--- GND
GND                  ----|        |--- GND
```

---

## I2C Bus

The I2C bus is used for low-speed digital peripherals.

Current devices:

- SHT31 temperature and humidity sensor

Future devices may include:

- RTC
- External ADC
- Current sensor
- Pressure sensor
- Additional environmental sensors

```text
ESP32-S3      SHT31
GPIO41  ---> SDA
GPIO40  ---> SCL
3V3     ---> VCC
GND     ---> GND
```

External pull-up resistors (approximately 4.7 kΩ) shall be added if they are not already present on the connected modules.

---

## ADC Inputs

Two ADC inputs are used for battery voltage monitoring.

```text
GPIO4 -> House battery voltage
GPIO5 -> Engine battery voltage
```

Both inputs shall be connected through resistor dividers and protection circuitry.

---

## Digital Inputs

Two digital inputs are used for alarm detection and Deep Sleep wake-up.

```text
GPIO12 -> Water ingress sensor
GPIO13 -> Smoke alarm input
```

Input protection and galvanic isolation should be considered for permanent installation.

---

## Serial Debug Enable

GPIO1 is connected to a jumper for enabling serial debug output.

```text
GPIO1 -> Serial debug enable
```

---

## Configuration Button

Prototype 2 includes a multifunction configuration button connected to GPIO2.

```text
GPIO2 -> Configuration button
```

Functions:

- Wake-up from Deep Sleep
- Activate Bluetooth provisioning
- Enter configuration mode
- Request factory reset

---

## LTE Interface

Prototype 2 includes a complete LTE interface.

```text
GPIO15 -> LTE PWR
GPIO16 -> LTE DTR
GPIO17 -> LTE TX
GPIO18 -> LTE RX
GPIO8  -> LTE sower switch
```

GPIO8 controls the external high-side power switch for the LTE module.

---

## SD Card Interface

Prototype 2 includes an SPI interface for an SD card.

```text
GPIO37 -> SD_CS
GPIO36 -> SD_MOSI
GPIO35 -> SD_SCK
GPIO47 -> SD_MISO
```

The SD card supply is routed through a removable jumper.

This allows future replacement of the jumper by an electronic power switch without modifying the remaining hardware.

GPIO21 is reserved for future SD power control.

---

## Status LEDs

Prototype 2 uses four dedicated external status LEDs.

```text
GPIO9  -> STATUS LED
GPIO10 -> NETWORK LED
GPIO11 -> BACKEND LED
GPIO12 -> ERROR LED
```

Each LED shall be connected through an appropriate current-limiting resistor.

The onboard RGB LED of the ESP32-S3 DevKitC-1 is not used by Prototype 2.

The logical behavior, operating modes, and blink patterns of the status indicators are defined in ADR-2.03 *Status Indicator Concept*.

---

## Pins Intentionally Avoided

| GPIO | Reason |
|------|--------|
| GPIO0 | Boot / strapping related |
| GPIO3 | Prototype stability |
| GPIO19 | Native USB D- |
| GPIO20 | Native USB D+ |
| GPIO43 | USB / Serial TX |
| GPIO44 | USB / Serial RX |
| GPIO45 | Strapping related |
| GPIO46 | Strapping related |

---

## Prototype 2 Scope

Prototype 2 includes:

- ESP32-S3 DevKitC-1
- SHT31 temperature and humidity sensor
- House battery voltage measurement
- Engine battery voltage measurement
- Water ingress detection
- Smoke alarm input
- LTE modem
- SD card
- LTE power switching
- SD power switch preparation
- Configuration button
- Factory reset
- Serial debug enable
- Deep Sleep operation
- MQTT communication
- Offline buffering

The prototype validates the complete hardware and software architecture before the custom PCB is designed.

---

## Notes

This pin assignment defines the stable hardware interface for Prototype 2.

Future PCB revisions are expected to preserve the logical signal assignments wherever practical while replacing the prototype wiring with an integrated PCB implementation.
