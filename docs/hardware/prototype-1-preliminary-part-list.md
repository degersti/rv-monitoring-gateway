# Prototype 1 Preliminary Part List

## Core Controller

| Qty | Part | Notes |
|---:|---|---|
| 1 | ESP32-S3 DevKitC-1 N16R8 | Main controller, WiFi/BLE, USB programming |

## Power Input and Supply

| Qty | Part | Notes |
|---:|---|---|
| 1 | 12 V input terminal block, 2-pin | Board supply input after external fuse |
| 1 | Fuse holder or inline fuse | Installed close to battery / supply source |
| 1 | Traco TSR 1-2450 or TSR 2-2450 switching regulator | 12 V to 5 V supply for ESP32 DevKit |
| 1 | Reverse polarity protection diode or MOSFET module | Recommended before regulator input |
| 1 | TVS diode for 12 V line | Recommended for onboard supply protection |
| 1 | Electrolytic capacitor, 100 µF / 25 V | Input bulk capacitor on 12 V side |
| 1 | Ceramic capacitor, 100 nF / 50 V | Input high-frequency decoupling near regulator |
| 1 | Electrolytic capacitor, 100 µF / 10 V or 16 V | Output bulk capacitor on 5 V side |
| 1 | Ceramic capacitor, 100 nF / 16 V or higher | Output high-frequency decoupling near regulator / ESP32 |

## Sensors and Inputs

| Qty | Part | Notes |
|---:|---|---|
| 1 | SHT31 breakout board | Temperature and humidity via I2C |
| 1 | Float switch / water ingress sensor | Digital alarm input |
| 1 | Smoke detector with relay output | Prefer dry-contact relay output |
| 2 | 2-pin terminal blocks | Water and smoke inputs |

## Battery Voltage Measurement

For each battery input:

| Qty | Part | Notes |
|---:|---|---|
| 1 | 100 kΩ resistor, 1% | Upper divider resistor |
| 1 | 18 kΩ resistor, 1% | Lower divider resistor |
| 1 | 1 kΩ resistor | ADC series resistor |
| 1 | 100 nF capacitor | ADC filter capacitor |
| 2 | Schottky diodes | Clamp to 3V3 and GND |
| 1 | 2-pin terminal block | Battery voltage input |

For two battery inputs, multiply by 2.

## Digital Input Conditioning

For each dry-contact input:

| Qty | Part | Notes |
|---:|---|---|
| 1 | 10 kΩ resistor | Pull-up to 3.3 V |
| 1 | 1 kΩ resistor | GPIO series protection |
| 1 | 100 nF capacitor | Optional debounce/filter |
| 1 | 2-pin terminal block | Sensor connection |

For water + smoke, multiply by 2.

## Prototype Carrier Board

| Qty | Part | Notes |
|---:|---|---|
| 1 | Perfboard / stripboard / prototype PCB board | Soldered Prototype 1 carrier |
| 2 | Female header strips | For plugging in ESP32-S3 DevKit |
| several | Screw terminals | External wiring |
| several | Pin headers | SHT31, debug, optional expansion |
| several | Jumper wires / hookup wire | Internal wiring |
| several | Heat shrink / cable ties | Mechanical strain relief |

## Reserved for Future

| Qty | Part | Notes |
|---:|---|---|
| 1 | microSD card module | SPI, GPIO10-GPIO13 |
| 1 | LTE modem module | UART, GPIO16-GPIO18 |
| 1 | SIM card / antenna | Only if LTE used |
| 1 | External antenna connector | Future LTE/WiFi considerations |
