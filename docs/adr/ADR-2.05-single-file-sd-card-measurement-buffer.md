# ADR-2.05: Single-File SD Card Measurement Buffer

## Status

> Supersedes the previous version of ADR-2.02.

## Context

ADR-2.02 replaced the NVS-based persistent measurement buffer introduced in ADR-1.08 with an SD card-based implementation.

The initial SD card design stored every measurement as an individual file and separated records into two directories:

```text
/buffer/absolute/
/buffer/relative/
```

The directory structure and file names represented the persistent queue state.

While this approach removed the limitations of NVS and avoided separately maintained buffer metadata, it required repeated directory operations and file-system scans to determine the state and chronological order of buffered measurements.

During implementation and testing of Prototype 2, the buffer architecture was therefore revised.

The persistent measurement buffer shall instead use a single binary file containing a small persistent header followed by fixed-size measurement record slots.

The functional requirements remain unchanged:

- measurements shall survive Deep Sleep, resets and power cycles
- measurements shall be retained during temporary communication outages
- absolute measurements shall be transmitted chronologically
- measurements created before time synchronization shall remain distinguishable from measurements with valid Unix timestamps
- obsolete relative measurements shall be removable after creation of a new Boot Epoch
- the communication layer shall remain independent of the underlying storage implementation

---

## Decision

The persistent measurement buffer shall be implemented as a single binary file on the external SD card.

The buffer file shall be located at:

```text
/buffer/measurement.bin
```

The file shall contain:

1. a persistent buffer header
2. a sequence of fixed-size measurement record slots

Conceptually:

```text
/buffer/measurement.bin

+-----------------------------+
| Buffer File Header          |
+-----------------------------+
| Measurement Record Slot 0   |
+-----------------------------+
| Measurement Record Slot 1   |
+-----------------------------+
| Measurement Record Slot 2   |
+-----------------------------+
| ...                         |
+-----------------------------+
```

New measurement records shall be appended to the end of the file.

Records shall not normally be physically removed immediately. Instead, they shall first be marked as removed and their storage shall later be reclaimed through file compaction.

---

## Buffer File Format

The buffer file shall begin with a persistent header containing the information required for efficient buffer access.

The header shall contain:

- file format identifier
- file format version
- number of allocated record slots
- number of active records
- number of active absolute records
- number of active relative records
- slot index of the oldest active absolute record

The file format identifier and version allow the firmware to validate the persistent file before using it.

Each record slot shall contain:

- a record state
- one `MeasurementRecord`

Conceptually:

```text
BufferFileHeader

Record Slot
├── state
└── MeasurementRecord
    ├── timestamp
    ├── Boot Epoch ID
    └── measurement data
```

Record slots shall have a fixed size so that their file position can be calculated directly from their slot index.

---

## Record States

Each stored record shall have a persistent state.

The following states are defined:

```text
ACTIVE
REMOVED
```

An `ACTIVE` record belongs to the current buffer contents.

A `REMOVED` record no longer belongs to the logical buffer but may remain physically present in the file until compaction is performed.

This allows records to be removed without rewriting the complete buffer file after every successful transmission.

---

## Timestamp Classification

Absolute and relative measurements shall no longer be stored in separate directories.

Instead, their type shall be determined from the timestamp stored inside the `MeasurementRecord`.

A timestamp greater than or equal to the configured valid timestamp threshold shall be treated as an absolute Unix timestamp.

A timestamp below this threshold shall be treated as a relative timestamp.

Therefore, absolute and relative records may coexist within the same buffer file.

---

## Buffer Initialization

During startup, the buffer shall be initialized after successful SD card initialization.

If the buffer directory does not exist, it shall be created.

If no buffer file exists, a new empty buffer file with a valid header shall be created.

If an existing buffer file is found, its header shall be loaded and validated.

Validation shall include at least:

- file format identifier
- file format version
- file size consistency
- slot count consistency
- active record count consistency
- absolute and relative record count consistency

If the persistent header is invalid or inconsistent, the firmware shall attempt to reconstruct the header by sequentially scanning the stored record slots.

The complete buffer shall therefore remain recoverable from the stored record slots without relying exclusively on a valid persistent header.

---

## Header Reconstruction

The persistent header is used to avoid unnecessary scans during normal operation but shall not be the sole source of truth for the stored measurements.

If the header cannot be validated, the firmware shall scan the record slots and reconstruct:

- total slot count
- active record count
- absolute record count
- relative record count
- oldest active absolute record slot

The reconstructed header shall subsequently be written back to the buffer file.

This provides recovery from invalid or inconsistent header metadata as long as the underlying record structure remains readable and structurally valid.

---

## Adding Measurements

New measurements shall be appended as fixed-size record slots.

Before a record is appended, sufficient SD card storage shall be ensured.

The new slot shall initially be stored with state:

```text
ACTIVE
```

After the record has been written successfully, the buffer metadata shall be updated accordingly.

Depending on its timestamp, the record shall increment either:

- the absolute record count, or
- the relative record count

The total active record count and slot count shall also be updated.

If the new record is the first absolute record in the buffer, its slot index shall become the oldest absolute record slot.

---

## Reading Buffered Measurements

Only records with valid absolute timestamps shall be made available for transmission.

The persistent header shall maintain the slot index of the oldest active absolute record.

This allows the oldest transmissible record to be accessed directly without scanning the complete buffer file.

Relative records shall remain stored but shall not be returned as transmissible measurements until their timestamps have been converted or otherwise resolved.

---

## Removing Transmitted Measurements

After successful transmission, the oldest absolute record shall be logically removed.

The corresponding record state shall be changed from:

```text
ACTIVE
```

to:

```text
REMOVED
```

The slot shall initially remain physically present in the buffer file.

The buffer counters shall then be updated.

If additional absolute records exist, the firmware shall determine the next active absolute slot and store its index in the buffer header.

If no absolute records remain, the oldest absolute slot shall be marked as invalid.

Physical storage occupied by removed slots shall be reclaimed later through compaction.

---

## Buffer Compaction

Logical removal does not reduce the physical size of the buffer file.

The buffer shall therefore support compaction.

During compaction:

1. a temporary buffer file shall be created
2. a new buffer header shall be initialized
3. the existing buffer file shall be scanned sequentially
4. only `ACTIVE` records shall be copied to the temporary file
5. the new header shall be reconstructed while records are copied
6. the new header shall be written to the temporary file
7. the original buffer file shall be replaced by the compacted file

Removed record slots are discarded during this process.

After successful compaction:

```text
slotCount == activeCount
```

and all physically stored slots represent active records.

Compaction shall not intentionally remove active measurement records.

---

## Boot Epoch Handling

Relative timestamps are only meaningful while the corresponding Boot Epoch remains valid.

During startup, the firmware shall determine whether a new Boot Epoch has been created.

If the previous Boot Epoch remains valid:

- existing relative records shall remain stored

If a new Boot Epoch has been created:

- existing active relative records from the previous Boot Epoch can no longer be reconstructed reliably
- these records shall be logically removed
- absolute records shall remain unaffected
- the buffer file shall subsequently be compacted to physically remove the obsolete relative records

The distinction between relative and absolute records shall be made from the timestamp contained in each measurement record.

No separate relative-record directory is required.

---

## Storage Capacity Management

The buffer shall not use a fixed compile-time maximum number of measurement records.

Its effective capacity shall primarily be determined by the available SD card storage.

Before appending a new record, the firmware shall verify that a configured minimum amount of free SD card storage remains available.

If sufficient free space exists, the record may be appended normally.

If storage becomes limited, the buffer shall first compact already removed slots.

If the required free-space reserve still cannot be maintained, the oldest active buffered records shall be removed until sufficient storage is available.

Because logical removal does not release file-system storage, compaction shall be performed when physical storage must be reclaimed.

This behavior provides ring-buffer-like storage management without requiring a fixed number of preallocated record slots.

---

## Chronological Ordering

Measurement records are appended to the buffer in the order in which they are created.

The slot order therefore represents the chronological insertion order of measurements.

The oldest transmissible absolute measurement is identified by the stored `oldestAbsoluteSlot`.

After removal of this record, the following slots are searched sequentially until the next active absolute record is found.

Relative records may exist between absolute records and shall be skipped during this search.

This avoids directory scans and timestamp-based file-name searches during normal buffer operation.

---

## Persistent Metadata

Unlike the initial ADR-2.02 design, the single-file implementation contains persistent buffer metadata.

This metadata is stored directly inside the buffer file header rather than separately in NVS.

The metadata exists primarily as an optimization for efficient access.

It shall not make the stored measurement records unrecoverable if the header becomes invalid.

The firmware shall therefore be capable of reconstructing the header from the stored record slots.

NVS shall not be used for measurement buffer metadata.

---

## Error Handling

All relevant SD card and buffer operations shall report failure explicitly.

Relevant failures include:

- SD card unavailable
- buffer directory creation failure
- buffer file creation failure
- header read or write failure
- invalid buffer file structure
- record read failure
- record write failure
- record state update failure
- buffer reconstruction failure
- compaction failure
- insufficient storage that cannot be recovered

Storage operations shall never silently assume success.

Corresponding faults shall be reported to the application's fault management.

Higher-level application logic and the state machine remain responsible for determining the resulting system behavior.

---

## Separation of Responsibilities

The measurement buffer shall remain independent of the communication transport.

The SD Manager shall provide generic SD card functionality such as:

- SD card initialization
- SD card availability
- directory creation
- storage capacity information

The Measurement Buffer shall be responsible for:

- buffer file management
- persistent buffer metadata
- record storage
- record retrieval
- logical record removal
- Boot Epoch-related record cleanup
- buffer reconstruction
- storage capacity handling
- file compaction

Higher-level application components shall access buffered measurements through the buffer or Data Manager interface and shall not depend on the physical file layout.

Communication modules shall not access the buffer file directly.

---

## Consequences

### Advantages

- only one persistent measurement file is required
- substantially fewer file-system objects than the previous one-file-per-measurement design
- no directory scans during normal buffer operation
- direct record access using fixed-size slots
- efficient retrieval of the oldest absolute record
- logical deletion avoids rewriting the complete file after every transmission
- removed storage can be reclaimed through controlled compaction
- absolute and relative records can coexist in one buffer
- buffer capacity is primarily determined by available SD card storage
- persistent metadata can be reconstructed from stored records
- no frequently updated NVS metadata is required
- communication layer remains storage independent

### Disadvantages

- buffer file format is more complex
- persistent header metadata must be kept consistent with record operations
- logical deletion temporarily leaves unused slots in the file
- compaction requires rewriting all remaining active records
- interrupted compaction must be handled carefully
- corruption of the single buffer file can affect multiple measurements
- format changes require explicit buffer file version handling

---

## Alternatives Considered

### One File per Measurement

This was the architecture originally defined by ADR-2.02.

**Pros**

- simple individual record files
- no central buffer file
- no persistent buffer header required
- corruption of one file affects only one measurement

**Cons**

- potentially large number of files
- repeated directory operations
- directory scans required to reconstruct buffer state
- chronological lookup depends on file-system enumeration and file names
- less efficient for frequent measurement buffering

**Rejected / Superseded**

---

### NVS-Based Ring Buffer

This was the architecture originally defined by ADR-1.08.

**Pros**

- no external storage required
- direct indexed record access

**Cons**

- limited internal Flash capacity
- Flash wear
- observed NVS storage exhaustion
- unsuitable for long communication outages and larger datasets

**Rejected**

---

## Changes Compared to ADR-2.02

ADR-2.05 supersedes the buffer organization defined by ADR-2.02.

The following major changes are introduced:

### Storage Organization

ADR-2.02:

```text
/buffer/absolute/<timestamp>.bin
/buffer/relative/<timestamp>.bin
```

ADR-2.05:

```text
/buffer/measurement.bin
```

### Record Storage

ADR-2.02 stored one measurement per file.

ADR-2.05 stores all measurements as fixed-size slots inside a single binary file.

### Absolute and Relative Measurements

ADR-2.02 separated them using different directories.

ADR-2.05 stores them together and determines their type from the timestamp contained in each record.

### Persistent Metadata

ADR-2.02 intentionally avoided persistent buffer metadata.

ADR-2.05 introduces a persistent buffer header for efficient operation while retaining the ability to reconstruct this metadata from the stored record slots.

### Record Removal

ADR-2.02 physically deleted individual measurement files.

ADR-2.05 logically marks slots as removed and later reclaims their storage through compaction.

### Buffer Reconstruction

ADR-2.02 reconstructed buffer state by scanning directories and file names.

ADR-2.05 reconstructs buffer metadata by sequentially scanning the record slots inside the binary buffer file.

### Storage Capacity

ADR-2.05 retains the dynamic SD card capacity concept but explicitly manages low-storage situations through logical removal and compaction.

---

## Relationship to Previous ADRs

### ADR-1.08

ADR-1.08 established the original persistent measurement buffer and its functional requirements.

Its NVS-based storage implementation remains superseded.

### ADR-2.02

ADR-2.02 established the decision to move persistent measurement buffering to the external SD card.

That fundamental decision remains valid.

However, ADR-2.05 supersedes the file-system organization and persistence model defined by ADR-2.02.

The following principles remain unchanged:

- external SD card as persistent measurement storage
- persistent buffering across Deep Sleep, resets and power cycles
- chronological transmission
- distinction between relative and absolute timestamps
- Boot Epoch handling
- storage-independent communication layer
- no measurement buffer metadata stored in NVS

ADR-2.02 shall therefore be marked:

```text
Superseded by ADR-2.05
```