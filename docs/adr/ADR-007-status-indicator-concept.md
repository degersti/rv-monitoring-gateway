# ADR-007: Status Indicator Concept

## Status

Accepted

## Context

The RV Monitoring Gateway requires a simple and intuitive method to communicate its operational status during development, testing, installation, and future field operation.

The final product shall provide clear visual feedback regarding:

* Device power and firmware operation
* Network connectivity (Wi-Fi or LTE)
* Cloud connectivity (MQTT)
* Alarm conditions (water ingress, smoke/fire detection, future alarms)
* Internal system faults

The current prototype platform (ESP32-S3 DevKitC-1) includes an onboard RGB LED but does not provide dedicated status LEDs.

The final product is expected to use dedicated status indicators on the front panel.

## Decision

The status indication concept shall be based on four logical status indicators:

| Indicator | Purpose                                     |
| --------- | ------------------------------------------- |
| PWR       | Device powered and firmware running         |
| NET       | Network connection available (Wi-Fi or LTE) |
| CLOUD     | MQTT / cloud connection available           |
| ALARM     | Alarm condition or system fault             |

The firmware shall internally manage these indicators independently of the underlying hardware implementation.

### Prototype Implementation

The ESP32-S3 onboard RGB LED shall be used as a temporary visualization device.

The RGB LED shall display the highest-priority active state according to the following mapping:

| State              | Color  | Pattern    |
| ------------------ | ------ | ---------- |
| Boot               | Yellow | Fast blink |
| Network connecting | Blue   | Slow blink |
| Network connected  | Blue   | Steady on  |
| MQTT connecting    | Green  | Slow blink |
| MQTT connected     | Green  | Steady on  |
| Alarm active       | Red    | Steady on  |
| System fault       | Red    | Fast blink |
| Deep sleep         | Off    | LED off    |

### Priority Order

If multiple states are active simultaneously, the following priority shall be applied:

1. System fault
2. Alarm active
3. MQTT connected / connecting
4. Network connected / connecting
5. Power / boot state

This ensures that critical conditions are always visible.

### Future Product Implementation

The final product shall replace the RGB LED with four dedicated status LEDs:

| LED   | Color         |
| ----- | ------------- |
| PWR   | Green         |
| NET   | Blue          |
| CLOUD | Cyan or Green |
| ALARM | Red           |

The firmware shall remain unchanged except for the hardware abstraction layer responsible for driving the indicators.

## Consequences

### Advantages

* Consistent user experience between prototype and final product.
* Clear separation between logical status indicators and hardware implementation.
* Professional appearance in the final product.
* Easier field diagnostics without requiring serial access.
* Minimal software changes required when migrating to custom hardware.

### Disadvantages

* The prototype can only display one state at a time due to the limitations of a single RGB LED.
* Simultaneous visualization of all logical indicators is not possible until dedicated LEDs are implemented.

## Notes

The status indicator system shall be implemented through a dedicated indicator abstraction layer.

Application code shall only set logical indicator states and shall not directly control LED hardware.
