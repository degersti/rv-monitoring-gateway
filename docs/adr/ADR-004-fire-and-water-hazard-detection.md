# ADR-004: Fire and Water Hazard Detection

## Status

Accepted

## Context

The monitoring gateway shall provide early warning of critical safety-related conditions aboard recreational vehicles and vessels.

Two hazards have been identified as primary monitoring targets:

- Smoke and fire detection
- Water ingress and bilge flooding

The gateway itself is intended to act as a monitoring and notification device and shall not perform safety-critical fire detection algorithms.

Sensor installations may be distributed throughout the vehicle or vessel, resulting in several meters of cabling between sensors and the gateway.

Therefore, the detection concept must prioritize reliability, low power consumption, ease of installation, and compatibility with commercially available safety devices.

## Decision

### Smoke and Fire Detection

Smoke and fire detection shall be performed using an external certified smoke detector equipped with a relay output or dry contact alarm output.

The gateway shall monitor the detector's alarm contact and generate notifications when an alarm condition is detected.

Low-cost gas sensors such as MQ135, MQ2, MQ5, MQ7, MQ9, or similar devices shall not be used as the primary fire detection mechanism.

### Water Ingress Detection

Water ingress detection shall be performed using an external float switch or equivalent water level switch.

The gateway shall monitor the switch state and generate an alarm when water exceeds the configured threshold level.

### Alarm Inputs

The alarm input circuitry required for wake-up detection shall remain powered during ESP32 deep-sleep operation to allow immediate wake-up upon an alarm event.

The gateway shall provide dedicated opto-isolated alarm inputs designed for dry-contact devices such as:

- Smoke detector relay output
- Float switch

The opto-isolation shall provide electrical separation between the external alarm wiring and the ESP32 controller circuitry, improving robustness against noise, wiring faults, and ground potential differences.

For the initial prototype, alarm inputs shall use normally-open (NO) contacts.

In the normal state, no current shall flow through the external sensor wiring. 
When an alarm condition is detected, the contact closure activates the opto-isolated alarm input, which in turn drives a wake-up capable GPIO input, allowing the ESP32 to wake immediately from deep sleep and process the alarm event.

## Rationale

### Why Use Certified Smoke Detectors

The gateway is intended to monitor and report alarm conditions rather than perform fire detection itself.

Certified smoke detectors are specifically designed, tested, and approved for fire detection applications. They provide significantly higher reliability than low-cost gas sensors and are designed to minimize false alarms while maintaining sensitivity to real fire events.

Using a certified detector allows the gateway to remain independent of the fire detection technology while focusing on alarm handling, communication, and notification.

### Why Not Use MQ-Series Gas Sensors

MQ-series sensors are designed primarily for gas concentration detection and air quality monitoring.

Although these sensors may react to smoke, combustion products, or volatile organic compounds, they have several limitations:

- No fire-detection certification
- Significant sensitivity to humidity and temperature
- Sensor drift over time
- Calibration requirements
- Long warm-up times
- Relatively high power consumption
- Potential false alarms caused by cleaning products, fuel vapors, cooking fumes, or exhaust gases
- Inability to reliably distinguish between harmless gases and actual fire conditions

For these reasons, MQ-series sensors are not considered suitable as the primary fire detection mechanism.

Such sensors may be supported in the future for environmental monitoring or trend analysis but shall not replace certified smoke detectors.

### Why Use Relay Contacts and Float Switches

Relay contacts and mechanical switches provide:

- Electrical simplicity
- High reliability
- Long cable compatibility
- Vendor independence
- Easy replacement of field devices
- Minimal processing requirements
- Low standby power consumption

The gateway remains independent of the specific detector manufacturer and can support a wide range of commercially available products.

### Why Use Normally-Open Contacts in the Prototype

A fail-safe design using normally-closed (NC) contacts was considered.

NC contacts offer the advantage that a cable break can be detected as a fault condition. However, this approach requires a permanently energized sensing circuit and continuous current flow through the external wiring.

For the initial prototype, the focus is on:

- Simplicity
- Low power consumption
- Reliable wake-up from deep sleep
- Reduced hardware complexity

Using normally-open (NO) contacts eliminates the need for continuous current flow through the external alarm wiring during normal operation. Current is only drawn by the alarm input circuit when an alarm condition causes the contact to close.

This approach allows immediate wake-up from ESP32 deep sleep while keeping standby power consumption and hardware complexity low.

The inability to detect cable breaks or disconnected sensors is accepted for the prototype phase and may be revisited in a future hardware revision.

## Consequences

### Advantages

- Robust and proven fire detection using certified detectors
- Simple integration of water level switches
- Low standby power consumption
- No continuous current through external alarm wiring
- Simple ESP32 deep-sleep wake-up implementation
- Vendor-independent field devices
- Reduced software complexity
- Suitable foundation for future product development
- Galvanic isolation between field wiring and controller electronics
- Improved immunity to electrical noise and transients
- Increased robustness in marine and RV environments

### Disadvantages

- Additional external detector hardware required
- Cable breaks cannot be detected with NO contacts
- Slightly more wiring required than integrated sensors
- No direct smoke concentration measurement

## Future Considerations

Future hardware revisions may evaluate:

- Support for both NO and NC alarm contacts
- Normally-closed (NC) fail-safe alarm loops
- Cable-break detection
- Supervised alarm circuits
- Additional environmental gas sensors for trend monitoring
- Integration with commercial marine and RV alarm systems

Any future gas or air-quality sensors shall be considered supplementary information sources and shall not replace certified smoke detectors for primary fire detection.
