# Prototype 2 Preliminary Part List

> Extends: Prototype 1 Preliminary Part List

Prototype 2 reuses all components from Prototype 1 and adds the following hardware.

---

## LTE Communication

| Qty | Part | Notes |
|---:|---|---|
| 1 | Waveshare SIM7000G CAT-M/NB-IoT/GNSS HAT | LTE modem for MQTT communication |
| 1 | LTE antenna | Matching antenna for the selected LTE bands |
| 1 | GNSS antenna | Required for later GNSS functionality |
| 1 | Nano SIM card | Activated IoT/mobile data SIM |

---

## LTE Power Switching

| Qty | Part | Notes |
|---:|---|---|
| 1 | IRF4905 P-channel MOSFET | High-side power switch for LTE supply |
| 1 | BC547B NPN transistor | MOSFET gate driver |
| 1 | 100 kΩ resistor | MOSFET gate pull-up |
| 1 | 470 Ω resistor | MOSFET gate resistor |
| 1 | 4.7 kΩ resistor | Base resistor |
| 1 | 100 kΩ resistor | Base pull-down |

---

## SD Card Storage

| Qty | Part | Notes |
|---:|---|---|
| 1 | MicroSD card module (3.3 V SPI) | Offline buffering and logging |
| 1 | MicroSD card | Capacity as required |
| 1 | 2-pin jumper header | SD card power jumper |
| 1 | Jumper shunt | Connects +3.3 V to +3V3_SD |

---

## Debug Interface

| Qty | Part | Notes |
|---:|---|---|
| 1 | 2-pin jumper header | Serial debug enable |
| 1 | Jumper shunt | Pulls GPIO1 LOW to enable serial debug output |
| 1 | 10 kΩ resistor | External pull-up for GPIO1 |

---

## Configuration Interface

| Qty | Part | Notes |
|---:|---|---|
| 1 | Momentary push button (NO) | Configuration button (Bluetooth provisioning / factory reset) |
| 1 | 10 kΩ resistor | External pull-up for configuration button |
| 1 | 100 nF ceramic capacitor *(optional)* | Hardware debounce / EMI suppression |

