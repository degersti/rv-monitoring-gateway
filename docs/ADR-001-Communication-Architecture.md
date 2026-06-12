# ADR-001: Communication Architecture

## Status

Accepted

## Date

2026-06-11

## Context

The RV Monitoring Gateway shall transmit telemetry and alarm data from an ESP32-based gateway to a backend system over an internet connection. The gateway is intended to operate in unattended recreational vehicles and vessels, where power consumption, unreliable connectivity, and simple remote access are important design considerations.

Although the initial system may consist of only one gateway and one backend service, the communication architecture should remain extensible for future dashboards, notification services, data storage, and additional consumers of the transmitted data.

## Decision Drivers

- Low communication overhead
- Suitability for low-power embedded devices
- Support for intermittent network connectivity
- Support for event-based alarm transmission
- Secure operation over public internet connections
- Easy integration with backend services and dashboards
- Future expandability without major firmware changes

## Considered Options

| Option | Principle | Advantages | Disadvantages | Assessment |
|---|---|---|---|---|
| MQTT | The gateway publishes telemetry and alarm messages to a broker. Backend services, dashboards, or notification handlers subscribe to relevant topics. | Lightweight protocol; well suited for IoT devices; low communication overhead; supports unreliable networks; supports publish/subscribe architecture; supports retained messages; supports Last Will and Testament for offline detection; easy integration with Node-RED, InfluxDB, Grafana, HiveMQ, Mosquitto and other IoT tools. | Requires an MQTT broker; slightly more infrastructure than a direct HTTP endpoint; topic structure and broker security must be designed properly. | Very suitable for the RV Monitoring Gateway, especially for telemetry, alarms, future dashboards, multiple consumers, and offline detection. |
| HTTP/HTTPS with PHP Backend | The gateway sends data directly to a PHP script or HTTP endpoint, which validates and stores the data in a database. | Simple and familiar web architecture; easy to host on standard web servers; direct database integration; no separate broker required; straightforward for simple one-device-to-server telemetry. | Higher protocol overhead per transmission; request/response model is less suitable for asynchronous telemetry; no built-in retained messages; no native Last Will mechanism; offline detection and alarm routing must be implemented separately; less flexible if multiple consumers are added later. | Suitable for a very simple prototype, but less flexible for a scalable IoT-oriented architecture. |
| REST API | The gateway sends structured HTTP requests to a backend API implemented with a web framework. | Clean backend architecture; scalable; easy integration with web applications, authentication systems, and configuration interfaces. | More backend development effort; still request/response based; offline detection, message retention, and event distribution must be implemented separately. | Suitable for later backend functions such as user management, configuration, and dashboards, but not selected as the primary telemetry transport. |
| OPC UA | The gateway exposes or transmits structured data using an industrial information model. | Strong data modeling; standardized semantics; common in industrial automation; supports structured objects, metadata, engineering units, and access models. | High complexity and overhead compared to MQTT; less suitable for small low-power microcontrollers; more difficult cloud integration; unnecessary for a small number of telemetry values. | Not selected for the ESP32 gateway itself. May be relevant later on the backend side for integration with industrial or fleet-management systems. |

## Decision

MQTT is selected as the primary communication protocol for telemetry and alarm transmission between the RV Monitoring Gateway and the backend infrastructure.

The gateway shall publish measurement data and alarm events to an MQTT broker. Backend services, dashboards, databases, or notification handlers may subscribe to the relevant topics and process the data independently.

## Rationale

MQTT provides a good balance between simplicity, efficiency, and expandability. It is lightweight enough for ESP32-based embedded devices, while still supporting important IoT features such as retained messages, event-driven communication, and Last Will and Testament messages for offline detection.

Although a direct HTTP or PHP-based backend would be sufficient for simple one-directional data upload, MQTT offers a more suitable foundation for the long-term system architecture. It allows additional consumers, such as dashboards, alarm handlers, logging services, or mobile applications, to be added without requiring changes to the embedded gateway firmware.

## Consequences

### Positive Consequences

- Low-overhead telemetry transmission
- Suitable for battery-powered and intermittently connected devices
- Clear separation between device firmware and backend consumers
- Easy integration with common IoT tools and cloud MQTT brokers
- Supports future expansion to dashboards, notification systems, and data storage
- Enables retained status messages and offline detection through Last Will and Testament

### Negative Consequences

- An MQTT broker is required
- Broker authentication and authorization must be configured securely
- Topic structure and payload format must be designed and documented
- Additional backend components are still required for persistent storage, visualization, and user management

## Initial Implementation Approach

For local development and protocol testing, a local Mosquitto broker may be used.

For the first internet-connected prototype, a managed MQTT broker such as HiveMQ Cloud may be used to reduce infrastructure effort and allow early testing over public internet connections.

The initial firmware shall be designed so that broker configuration, credentials, and transport settings can be changed without modifying the core measurement and alarm logic.

## Related Decisions

- ADR-002: Backend Infrastructure
- ADR-003: Hardware Platform
- ADR-004: Sensor Selection
