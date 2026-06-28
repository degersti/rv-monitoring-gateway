# ADR-010: Persistent Measurement Buffer

## Status

Accepted

## Context

The RV Monitoring Gateway periodically records sensor measurements.

Under normal operating conditions, measurements are transmitted immediately to the backend using MQTT.

However, network connectivity cannot always be guaranteed. Possible reasons include:

- Wi-Fi unavailable
- MQTT broker unreachable
- Future LTE, NB-IoT or LoRaWAN connection unavailable
- Temporary backend outages

To prevent data loss, measurements shall be stored locally until successful transmission becomes possible.

The storage mechanism shall:

- survive Deep Sleep
- survive power cycles
- tolerate unexpected resets
- minimize Flash wear
- support chronological transmission after reconnection
- remain independent of the communication technology (Wi-Fi, LTE, NB-IoT, LoRaWAN, ...)

## Decision

The gateway shall implement a persistent ring buffer stored in the ESP32 Flash.

Each measurement shall be written as one fixed-size record.

The buffer operates as a circular queue.

Two indices are maintained:

- write index
- read index

Operation:

1. New measurements are always written at the current write index.
2. After writing, the write index advances.
3. Successfully transmitted records advance the read index.
4. When the write index reaches the end of the buffer, it wraps to the beginning.
5. If the buffer becomes full, the oldest unsent measurements are overwritten.

The buffer metadata (indices and record count) shall be stored persistently.

The Data Manager shall own the persistent buffer and provide a technology-independent interface for storing and retrieving measurements.

Communication modules shall access buffered measurements exclusively through the Data Manager interface and shall remain unaware of the underlying storage implementation.

## Timestamp Handling

Each measurement record shall contain:

- Timestamp
- Boot Epoch ID

The timestamp shall either represent:

- a valid Unix timestamp, or
- a relative timestamp when no valid UTC reference is available.

The `bootEpochId` identifies the runtime epoch in which the measurement was recorded.

It shall be provided by the Runtime Manager and stored together with each measurement.

The combination of timestamp and `bootEpochId` allows the firmware or backend to determine whether a relative timestamp can later be reconstructed.

## Record Structure

Each measurement record shall contain:

- Timestamp
- Boot Epoch ID
- House battery voltage
- Engine battery voltage
- Temperature
- Humidity
- Water alarm state
- Smoke alarm state

The exact binary layout is implementation-specific but shall remain version controlled.

## Consequences

### Advantages

- Measurements survive Deep Sleep and power cycles.
- Data loss during temporary communication outages is minimized.
- Flash wear remains predictable due to sequential writes.
- Constant memory usage.
- Simple deterministic implementation.
- Communication layer remains storage independent.
- Future communication technologies require no buffer redesign.
- Relative timestamps can be reconstructed whenever a valid time reference becomes available during the same runtime epoch.
- Power interruptions become traceable through the `bootEpochId`.

### Disadvantages

- Flash erase cycles limit total lifetime.
- Very long outages may overwrite the oldest measurements.
- Requires persistent metadata management.
- Timestamp reconstruction is only possible within the same runtime epoch.

## Alternatives Considered

### RAM Buffer

**Pros**

- Very fast
- No Flash wear

**Cons**

- Data lost during reset
- Data lost during Deep Sleep

**Rejected**

### RTC Memory

**Pros**

- Survives Deep Sleep
- Fast access

**Cons**

- Lost after power failure
- Limited capacity
- Not suitable for long communication outages

**Rejected**

### File System (LittleFS / SPIFFS)

**Pros**

- Flexible
- Human-readable during debugging

**Cons**

- Higher complexity
- Additional filesystem overhead
- Less deterministic memory layout

**Rejected for Prototype 1**

### External FRAM

**Pros**

- Extremely high write endurance
- Fast writes

**Cons**

- Additional hardware
- Increased BOM cost
- Additional PCB space

May be reconsidered for future product generations.

## Future Considerations

The buffer architecture shall remain independent of the transport layer.

Future communication methods such as:

- LTE
- NB-IoT
- LoRaWAN
- Ethernet

shall retrieve buffered measurements through the same Data Manager API.

Possible future extensions include:

- Record versioning
- CRC per record
- Acknowledged transmission states
- Priority records for alarm events
- Buffer statistics
- Configurable buffer size
- Compression of buffered records
