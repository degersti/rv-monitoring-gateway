# Prototype 1 Preliminary Part List

## Core Controller

| Qty | Part | Notes |
|---:|---|---|
| 1 | ESP32-S3 DevKitC-1 N16R8 | Main controller, WiFi/BLE, USB programming |

## Sensors and Inputs

| Qty | Part | Notes |
|---:|---|---|
| 1 | SHT31 breakout board | Temperature and humidity via I2C |
| 1 | Float switch / water ingress sensor | Digital alarm input |
| 1 | Smoke detector with relay output | Prefer dry-contact relay output |

## Connctors

| Qty | Part | Notes |
|---:|---|---|
| 1 | Terminal block, 6-pin | +12V supply, sensor supply input, house/engine voltage sense, GND |
| 1 | Terminal block, 2-pin | Water detector loop |
| 1 | Terminal block, 2-pin | Smoke / fire detector loop |
| 4 | Fuse holder | Gateway supply, sensor supply, house sense, engine sense |
| 3 | TVS diode, unidirectional, 12 V line | Transient protection on onboard supply input |

## Power Input and Supply

| Qty | Part | Notes |
|---:|---|---|
| 1 | Traco TSR 1-2450 or TSR 2-2450 switching regulator | 12 V to 5 V supply for ESP32 DevKit; TSR 2-2450 if LTE reserve is desired |
| 1 | P-MOSFET, e.g. FQP27P06 | Reverse polarity protection before regulator input |
| 1 | 100 kΩ resistor | MOSFET gate pull-down resistor |
| 1 | Electrolytic capacitor, 100 µF / 50 V | Input bulk capacitor before TSR regulator |
| 1 | Electrolytic capacitor, 100 µF / 10 V or 16 V | Output bulk capacitor on 5 V rail |
| 1 | Ceramic capacitor, 100 nF / 50 V | Input high-frequency decoupling near TSR regulator |
| 1 | Ceramic capacitor, 100 nF / 16 V or higher | Output high-frequency decoupling near ESP32 / 5 V rail |

## Battery Voltage Measurement

For each battery input:

| Qty | Part | Notes |
|---:|---|---|
| 1 | 100 kΩ resistor, 1% | Upper divider resistor |
| 1 | 18 kΩ resistor, 1% | Lower divider resistor |
| 1 | 1 kΩ resistor | ADC series resistor |
| 1 | 100 nF capacitor | ADC filter capacitor |
| 2 | 1N5819 Schottky diode | Clamped to 3V3 and GND |

For two battery inputs, multiply by 2.

## Digital Input Conditioning

For each dry-contact input:

| Qty | Part | Notes |
|---:|---|---|
| 1 | 47 kΩ resistor, 1% | Upper divider resistor |
| 1 | 10 kΩ resistor, 1% | Lower divider resistor |
| 1 | 1 kΩ resistor | GPIO series protection |
| 1 | 100 nF ceramic capacitor | Optional input debounce / noise filter from GPIO input to GND |
| 2 | 1N5819 Schottky diode | Clamped to 3V3 and GND |

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
