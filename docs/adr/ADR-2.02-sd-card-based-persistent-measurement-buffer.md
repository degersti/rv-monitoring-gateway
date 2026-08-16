# ADR-2.02: SD Card-Based Persistent Measurement Buffer

## Status

> Replaced by ADR-2.05

## Context

ADR-1.08 introduced a persistent measurement buffer stored in the internal ESP32 Flash using NVS.

The implementation fulfilled the functional requirements of Prototype 1 by preserving measurements during temporary communication outages and across Deep Sleep, resets and power cycles. Measurements were stored as fixed-size binary records, while the buffer metadata was maintained separately in NVS.

During long-term testing, the NVS implementation produced the following error:

```text
[E][Preferences.cpp:294] putBytes(): nvs_set_blob fail: metadata NOT_ENOUGH_SPACE
[DEBUG] Metadata write: 0/16 bytes
[ERROR] Failed to save measurement buffer metadata after pop
```

Although the metadata itself required only a few bytes, NVS was no longer able to reliably update the metadata blob.

In addition, the available storage capacity of the internal Flash is limited. Increasing the measurement frequency, extending communication outages or buffering additional data such as GNSS positions would further reduce the usable buffering duration.

Prototype 2 introduces an external SD card which provides significantly larger storage capacity and can be replaced independently of the controller hardware.

The communication layer shall remain independent of the underlying storage implementation.

---

## Decision

The persistent measurement buffer shall be moved from the internal ESP32 Flash (NVS) to the external SD card.

The functional behavior defined in ADR-1.08 remains unchanged.

Only the persistent storage medium is replaced.

The Data Manager shall continue to provide a storage-independent interface for storing and retrieving buffered measurements.

Communication modules shall remain unaware of the underlying storage implementation.

NVS shall no longer be used for measurement records or frequently updated buffer metadata.

NVS remains reserved for configuration parameters and other infrequently modified persistent data.

---

## Storage Structure

Buffered measurements shall be stored as individual binary files on the SD card.

Two directories shall be used:

```text
/buffer/absolute/
/buffer/relative/
```

Measurements with a valid Unix timestamp shall be stored in the **absolute** directory.

Example:

```text
/buffer/absolute/1756423762.bin
```

Measurements recorded before a valid absolute time reference is available shall be stored in the **relative** directory.

Example:

```text
/buffer/relative/310.bin
```

The file name of an absolute record shall be its Unix timestamp in seconds.

The file name of a relative record shall be its relative timestamp in seconds.

Since measurements are generated at intervals significantly greater than one second, timestamp collisions are excluded by the system design.

No additional sequence number is required.

The chronological order of buffered measurements shall therefore be represented directly by the file names.

No persistent read index, write index or record counter shall be maintained.

The directory structure of the SD card itself represents the persistent queue.

---

## Buffer Operation

The measurement buffer shall operate according to the following rules:

- creating a measurement creates a new file
- measurements with a valid Unix timestamp shall be stored in the `absolute` directory
- measurements without a valid Unix timestamp shall be stored in the `relative` directory
- buffered measurements shall be transmitted only from the `absolute` directory
- reading the oldest buffered measurement opens the file with the smallest timestamp in the `absolute` directory
- successfully transmitted measurements are removed by deleting the corresponding file
- the number of buffered measurements is determined by counting valid files in both buffer directories
- after startup, the buffer state is reconstructed by scanning the buffer directories

The implementation shall not depend on persistent metadata stored outside the measurement records themselves.
---

## Timestamp Handling

Each measurement record shall continue to contain:

- timestamp
- Boot Epoch ID
- measurement data

The timestamp shall represent either:

- a valid Unix timestamp, or
- a relative timestamp when no valid absolute time reference exists.

The Boot Epoch ID identifies the runtime epoch in which the measurement was created.

It remains part of every measurement record to support timestamp reconstruction and validation.

---

## Relative Timestamp Reconstruction

When a valid time synchronization is obtained during the current Boot Epoch, buffered measurements stored in the relative directory shall be converted into absolute measurements.

For every relative measurement:

1. read the relative record
2. verify that the Boot Epoch ID matches the current Boot Epoch
3. reconstruct the absolute Unix timestamp
4. update the measurement record
5. store the updated record in

```text
/buffer/absolute/<unixTimestamp>.bin
```

6. verify that the absolute record has been stored successfully
7. remove the original relative record

The original relative record shall not be removed before the absolute record has been stored successfully.

---

## Boot Epoch Handling

Relative timestamps can only be reconstructed while the runtime time base of their Boot Epoch remains available.

The following information shall therefore remain stored in RTC memory:

- current Boot Epoch ID
- last synchronized Unix timestamp
- relative system time at synchronization
- RTC validation information

During system startup, the firmware shall determine whether the previous Boot Epoch can be continued.

If the retained RTC information is valid:

- the previous Boot Epoch shall continue
- relative measurements remain valid
- timestamp reconstruction remains possible

If the retained RTC information is no longer valid:

- a new Boot Epoch shall be created
- timestamp reconstruction for previous relative measurements becomes impossible
- all files stored in

```text
/buffer/relative/
```

shall be removed.

Absolute measurements stored in

```text
/buffer/absolute/
```

remain unaffected.

---

## Error Handling

The firmware shall detect and report SD card failures.

Storage operations shall never silently assume success.

If the SD card is unavailable or produces write errors, the corresponding operation shall return an explicit failure status.

The resulting system behavior shall be handled by the state machine.

---

## Consequences

### Advantages

- significantly larger persistent buffer capacity
- supports long communication outages
- suitable for shorter measurement intervals
- suitable for additional data such as GNSS positions
- avoids NVS storage exhaustion
- greatly reduces internal Flash wear
- no persistent buffer metadata required
- simple and deterministic recovery after restart
- storage medium can be replaced independently of the ESP32
- Data Manager interface remains largely unchanged
- communication layer remains storage independent

### Disadvantages

- requires additional hardware
- SD cards may fail or become unavailable
- file system operations are more complex than NVS access
- startup requires scanning the buffer directories
- SD card access consumes additional power
- file operations are generally slower than internal Flash access

---

## Alternatives Considered

### Continue Using NVS

**Pros**

- no additional hardware
- existing implementation retained

**Cons**

- limited storage capacity
- Flash wear
- observed NVS storage exhaustion
- unsuitable for long communication outages

**Rejected**

---

### LittleFS / SPIFFS

**Pros**

- file-based storage
- no external hardware

**Cons**

- still limited by internal Flash
- Flash wear remains
- storage cannot be replaced independently

**Rejected**

---

### External FRAM

**Pros**

- extremely high write endurance
- deterministic write performance

**Cons**

- additional hardware
- higher BOM cost
- limited storage capacity

May be reconsidered for future hardware revisions.

---

## Future Considerations

This ADR only defines the persistent storage of buffered measurements.

Future ADRs may extend the SD card usage for:

- persistent system logging
- diagnostic logging
- user-accessible data logging
- firmware update storage
- additional file management functions

These topics are intentionally excluded from this ADR.

---

## Relationship to ADR-1.08

This ADR supersedes the storage decision defined in ADR-1.08.

The following architectural decisions remain unchanged:

- persistent measurement buffering
- chronological transmission
- transport-independent buffer interface
- ownership by the Data Manager
- timestamp handling
- Boot Epoch concept
- communication-layer independence

Only the persistent storage medium changes from the internal ESP32 Flash (NVS) to the external SD card.
