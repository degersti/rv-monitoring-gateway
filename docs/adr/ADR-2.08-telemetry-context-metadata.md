# ADR-2.08 - Telemetry Context Metadata

## Status

Accepted


## Context

The gateway periodically generates telemetry records with sensor measurements, alarm states, and timing information:

```json
{
  "bootEpochId": 409,
  "timestamp": 1791028922,
  "houseBatteryVoltage": 12.2,
  "engineBatteryVoltage": 4.9,
  "temperature": 23.6,
  "humidity": 61.5,
  "waterAlarm": false,
  "smokeAlarm": false
}
```

The web app requires additional context to interpret this telemetry:

- the network through which the message was transmitted,
- the signal strength of that connection,
- the configured telemetry interval, so that the web app can determine whether a gateway is online and how much time has passed since the last telemetry,
- whether a record was delivered immediately or with a delay.

The gateway operates in a sleep-based cycle (wake-up, send telemetry, deep sleep). Because of this cycle, an MQTT connection state says nothing about whether the gateway is operating normally. Online/offline detection is therefore derived from the received telemetry and the interval at which it is expected.

The required values belong to different points in the telemetry lifecycle:

- The telemetry interval is device configuration and applies when a measurement is **generated**.
- Network type and signal strength describe the connection used when the message is **transmitted**.

This distinction matters because records may be buffered locally without a network and sent later (ADR-2.05). The network connection is also established after the measurement has been created, so it is not known at generation time.

ADR-2.07 introduced `network_manager` as the common abstraction for WiFi and cellular. This extension builds on it so that the telemetry and MQTT layers remain free of network-specific dependencies.


## Decision

The telemetry payload shall be extended with three fields. No separate MQTT topic or device-state message is introduced.

```text
networkType
rssi
telemetryInterval
```

```text
Telemetry Context
│
├── Measurement Context
│   └── telemetryInterval
│
└── Network Context
    ├── networkType
    └── rssi
```

The resulting payload of a record transmitted immediately:

```json
{
  "bootEpochId": 409,
  "timestamp": 1791028922,
  "houseBatteryVoltage": 12.2,
  "engineBatteryVoltage": 4.9,
  "temperature": 23.6,
  "humidity": 61.5,
  "waterAlarm": false,
  "smokeAlarm": false,
  "networkType": "CELLULAR",
  "rssi": 19,
  "telemetryInterval": 10
}
```

The payload of a record that was buffered and transmitted later:

```json
{
  "bootEpochId": 409,
  "timestamp": 1791028312,
  "houseBatteryVoltage": 12.1,
  "engineBatteryVoltage": 4.9,
  "temperature": 23.4,
  "humidity": 61.2,
  "waterAlarm": false,
  "smokeAlarm": false,
  "networkType": "UNAVAILABLE",
  "rssi": 99,
  "telemetryInterval": 10
}
```

All three fields are always present in every telemetry message, including messages sent after an alarm wake-up and messages sent from the buffer.


### Field Definitions

The MQTT payload contains plain values without units. The interpretation of each value is defined by this ADR and, for `rssi`, by `networkType`.

| Field | Type | Meaning |
|---|---|---|
| `networkType` | string | Network used for the first transmission of the record. Values: `WIFI`, `CELLULAR`, `UNAVAILABLE` |
| `rssi` | integer | Signal strength of that network. Interpretation depends on `networkType` (see below) |
| `telemetryInterval` | integer | Configured telemetry interval in seconds |

**`rssi` interpretation**

| `networkType` | Value | Unit |
|---|---|---|
| `WIFI` | negative integer, e.g. `-67` | dBm |
| `CELLULAR` | `0` to `31`, e.g. `19` | modem signal quality (CSQ) |
| `UNAVAILABLE` | `99` | not applicable |
| any | `99` | signal strength unknown |

The value `99` is used whenever no valid signal strength is available. It cannot collide with a valid value of either network type: WiFi values are always negative, and valid cellular values are in the range 0 to 31. The `rssi` field is never omitted. `networkType = UNAVAILABLE` is always combined with `rssi = 99`.

WiFi and cellular values are not directly comparable. The web app shall interpret and display them according to `networkType`.


### Measurement Context

`telemetryInterval` is a configuration value, not a guarantee. The actual spacing between two records may differ, for example because of connection time or because an alarm wake-up triggers an additional telemetry message outside the regular cycle. It shall not be derived from the actually executed sleep duration. Future versions of the firmware may take these variaties into account.


### Network Context

`networkType` and `rssi` describe the network used for the **first transmission** of a record. They are determined when the record is handed to transmission in the cycle in which it was created, and only while the network connection state is `CONNECTED`. Both are obtained exclusively through `network_manager`:

```text
network_manager
├── getActiveNetwork()   → networkType
└── getRSSI()            → rssi
```

`network_manager` delegates `getRSSI()` to the active network backend. The telemetry and MQTT layers shall not access `wifi_manager` or `cellular_manager` directly (ADR-2.07).

Each backend is responsible for providing a valid value or `99`. The WiFi backend reports the current signal strength. The cellular backend may report the value determined when the mobile network connection was established and retain it for the remaining connection, because querying the modem during an active PPP data connection is not guaranteed to be possible. As a result, the cellular value can be older than the transmission itself.

**Records that cannot be transmitted in their generation cycle**

If a record cannot be transmitted in the cycle in which it was created, it is stored in the buffer with:

```text
networkType = "UNAVAILABLE"
rssi        = 99
```

This applies both when no network connection is available and when the transmission fails despite an established connection. Only records that were actually transmitted carry a real network context.

Buffered records are transmitted later **unchanged**. Their network fields are not updated at that time. The backend can therefore recognize delayed records by `networkType = UNAVAILABLE`.

The network over which a buffered record was eventually delivered is not reported per record. It is known from the current record transmitted in the same connection.


### Telemetry Lifecycle

```text
Measurement record created
   ├── Timestamp
   ├── Sensor values
   ├── Alarm states
   └── Telemetry interval (from configuration)
          │
          ▼
   Network connected?
          │
   ┌──────┴───────┐
  NO             YES
   │              │
   │              ▼
   │      Create payload:
   │      networkType = active network
   │      rssi        = current value
   │              │
   │              ▼
   │      MQTT transmission
   │              │
   │       ┌──────┴──────┐
   │    success       failure
   │       │              │
   │       ▼              │
   │     done             │
   │                      │
   └──────────┬───────────┘
              ▼
            Buffer (measurement data + telemetryInterval,
              |           no network information)
              │
              ▼  (later connection)
      Create payload:
      networkType = UNAVAILABLE
      rssi        = 99
              │
              ▼
      MQTT transmission
```

Telemetry generation and transmission remain separate. A record does not require an active network when it is created.


### Impact on the Measurement Buffer

The measurement record is extended with telemetryInterval. Since the buffer file defined in ADR-2.05 uses fixed-size record slots, this changes the slot size and therefore the buffer file format.

The buffer file format version shall be incremented. A buffer file with an older format version shall not be interpreted using the new record layout.

networkType and rssi are not stored in the buffer. The slot size therefore grows only by telemetryInterval, and the stored records never contain network information that could be outdated or misleading.

This requires that the transmission path can distinguish a fresh telemetry record from a telemetry record read from the buffer when the payload is created.



### Alarm Wake-Ups

Currently there are no dedicated alarm messages. An alarm wakes the gateway and results in a regular telemetry message. The new fields are therefore part of this message as well.

Dedicated alarm messages with a higher priority may be introduced in the future. Such messages are a separate message type, are not covered by this ADR, and are not required to contain networkType, rssi, or telemetryInterval.


### Online Detection in the Backend

The web app determines online/offline state and the time since the last telemetry from the received telemetry. This ADR defines only which information the gateway provides. Evaluation logic, such as thresholds derived from `telemetryInterval`, is the responsibility of the backend and not part of this ADR.

The backend relies on current telemetry being transmitted before buffered telemetry and on requesting the latest data by the newest timestamp. Delayed records are identifiable by `networkType = UNAVAILABLE`.


## Consequences

The payload gives the backend enough context to interpret measurement frequency, connection type, and delivery delay. It builds on ADR-2.07, keeps record generation independent of the network, and stays compatible with buffering. Buffered records can be transmitted without modification.


### Advantages

- The backend can tell whether telemetry was transmitted via WiFi or cellular.
- Delayed records are directly recognizable by `networkType = UNAVAILABLE`.
- Buffered records are sent unchanged; no merging with network information is required at transmission.
- Signal strength is available for monitoring, diagnostics, and analysis.
- Online detection and the time since the last telemetry can be evaluated against the actually configured interval instead of a fixed backend assumption.
- Buffered records keep their original interval.
- `rssi` always contains a numeric value, which keeps the payload schema stable.
- RSSI retrieval stays encapsulated in the network architecture; telemetry and MQTT remain network-agnostic.
- No additional topic, message, or backend request is required.


### Disadvantages

- Messages and buffered records become slightly larger.
- The meaning of `rssi` depends on `networkType`; backend and web app must know both interpretations.
- `networkType` has a third value, `UNAVAILABLE`, which backend and web app must handle.
- For buffered records, the network used for the eventual delivery is not reported per record.
- The cellular `rssi` can be older than the transmission itself.
- WiFi and cellular RSSI values are not directly comparable.
- The buffer file format changes and requires version handling.
- Backend components must support the extended schema.


## Open Points

- Handling of existing buffer files with an older format version: discard or migrate.


## Future Considerations

Possible extensions include cellular operator, connection technology
(LTE-M / NB-IoT), IP/session information, firmware version, configuration
version, buffer/queue status, sequence numbers, and further network quality
indicators. These shall only be added if they provide a concrete benefit.

If the amount of metadata grows significantly, separate message types or MQTT
topics for device state, diagnostics, and telemetry may be reconsidered. For
the current prototype, extending the existing telemetry payload is preferred.

The MQTT Last Will and Testament mechanism is currently not used for online
detection. It may be reconsidered as a future feature.


## Relationship to Other ADRs

- **ADR-2.05:** The measurement record is extended; the buffer file format version is incremented.
- **ADR-2.06 / ADR-2.07:** `networkType` and `rssi` are provided through `network_manager`, which remains the only access path to network information.
