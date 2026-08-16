# Measurement Buffer Reader

The `buffer_reader.py` tool is used to read the binary `measurements.bin` file created by the IoT gateway on the SD card.

The tool reads the buffer header and the stored measurement records and displays their contents in a human-readable format in the terminal.

## Requirements

Python 3 is required to run the tool.

The installation can be verified with:

```bash
python --version
```

No additional Python packages are required.

## Directory Structure

By default, the reader expects the following directory structure:

```text
tools/
└── measurement_buffer_reader/
    ├── README.md
    ├── buffer_reader.py
    └── data/
        └── measurements.bin
```

Copy the `measurements.bin` file from the gateway SD card:

```text
/buffer/measurements.bin
```

to the local directory:

```text
tools/measurement_buffer_reader/data/
```

## Usage

The reader can be started from the project root directory:

```bash
python tools/measurement_buffer_reader/buffer_reader.py
```

Alternatively, change to the tool directory first:

```bash
cd tools/measurement_buffer_reader
python buffer_reader.py
```

## Reading a Different Buffer File

An alternative path to a `measurements.bin` file can be passed as an optional command-line argument.

Example:

```bash
python tools/measurement_buffer_reader/buffer_reader.py E:/buffer/measurements.bin
```

This allows the buffer to be read directly from an inserted SD card without copying the file to the local `data` directory first.

## Output

The reader first displays information from the buffer header, including:

- File size
- Buffer version
- Number of slots
- Number of active records
- Number of absolute records
- Number of relative records
- Slot containing the oldest absolute record

The individual measurement records are then displayed.

Example:

```text
Slot    0 | ACTIVE

           Boot epoch ID:          3
           Timestamp:              1786878123
           House battery voltage:  12.64 V
           Engine battery voltage: 12.81 V
           Temperature:            23.72 °C
           Humidity:               57.31 %
           Water alarm:            False
           Smoke alarm:            False
```

A slot can have one of the following states:

```text
ACTIVE
REMOVED
```

`ACTIVE` indicates that the measurement record is currently stored in the buffer.

`REMOVED` indicates that the record has already been logically removed from the buffer, while its slot may still physically exist in the file until the buffer is compacted.

## Note

The reader is designed for the current binary measurement buffer format.

If the firmware structure of `MeasurementRecord`, `BufferFileHeader`, or `StoredMeasurementRecord` changes, the reader must be updated accordingly.

