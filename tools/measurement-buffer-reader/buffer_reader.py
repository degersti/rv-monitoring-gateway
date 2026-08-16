"""
Measurement Buffer Reader

Reads and validates the binary measurement buffer created by the
RV Monitoring Gateway firmware.
"""

from pathlib import Path
import struct
import sys

BUFFER_FILE_MAGIC = 0x4D425546
BUFFER_FILE_VERSION = 1
RECORD_STATE_ACTIVE = 0xA5A5A5A5
RECORD_STATE_REMOVED = 0x00000000

HEADER_FORMAT = "<7I"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

DEFAULT_BUFFER_FILE = (
    Path(__file__).resolve().parent
    / "data"
    / "measurements.bin"
)


def get_buffer_file() -> Path:
    if len(sys.argv) > 2:
        print("Usage: python measurement_buffer_reader.py [measurements.bin]")
        sys.exit(1)

    if len(sys.argv) == 2:
        return Path(sys.argv[1]).expanduser().resolve()

    return DEFAULT_BUFFER_FILE


def read_header(file):
    data = file.read(HEADER_SIZE)

    if len(data) != HEADER_SIZE:
        raise ValueError(
            f"Buffer file is too small ({len(data)} bytes, "
            f"expected at least {HEADER_SIZE})"
        )

    (
        magic,
        version,
        slot_count,
        active_count,
        absolute_count,
        relative_count,
        oldest_absolute_slot,
    ) = struct.unpack(HEADER_FORMAT, data)

    return {
        "magic": magic,
        "version": version,
        "slot_count": slot_count,
        "active_count": active_count,
        "absolute_count": absolute_count,
        "relative_count": relative_count,
        "oldest_absolute_slot": oldest_absolute_slot,
    }


def validate_header(header):
    if header["magic"] != BUFFER_FILE_MAGIC:
        raise ValueError(
            f"Invalid buffer magic: 0x{header['magic']:08X}"
        )

    if header["version"] != BUFFER_FILE_VERSION:
        raise ValueError(
            f"Unsupported buffer version: {header['version']}"
        )

    if (
        header["active_count"]
        != header["absolute_count"] + header["relative_count"]
    ):
        raise ValueError("Invalid record counters in buffer header")

    if header["active_count"] > header["slot_count"]:
        raise ValueError("Active record count exceeds slot count")


def determine_slot_size(file_size, slot_count):
    payload_size = file_size - HEADER_SIZE

    if payload_size < 0:
        raise ValueError("Invalid buffer file size")

    if slot_count == 0:
        if payload_size != 0:
            raise ValueError(
                "Buffer contains payload although slot count is zero"
            )
        return 0

    if payload_size % slot_count != 0:
        raise ValueError(
            "Buffer payload size is not divisible by slot count"
        )

    return payload_size // slot_count


def state_name(state):
    if state == RECORD_STATE_ACTIVE:
        return "ACTIVE"

    if state == RECORD_STATE_REMOVED:
        return "REMOVED"

    return f"UNKNOWN (0x{state:08X})"


def print_header(header, file_size, slot_size):
    oldest_slot = header["oldest_absolute_slot"]

    if oldest_slot == 0xFFFFFFFF:
        oldest_slot_text = "NONE"
    else:
        oldest_slot_text = str(oldest_slot)

    print()
    print("Measurement Buffer")
    print("=" * 50)
    print(f"File size:             {file_size} bytes")
    print(f"Magic:                 0x{header['magic']:08X} (MBUF)")
    print(f"Version:               {header['version']}")
    print(f"Slot size:             {slot_size} bytes")
    print(f"Slots:                 {header['slot_count']}")
    print(f"Active records:        {header['active_count']}")
    print(f"Absolute records:      {header['absolute_count']}")
    print(f"Relative records:      {header['relative_count']}")
    print(f"Oldest absolute slot:  {oldest_slot_text}")
    print()


def print_slots(file, header, slot_size):
    if header["slot_count"] == 0:
        print("Buffer contains no record slots.")
        return

    if slot_size < 4:
        raise ValueError(f"Invalid slot size: {slot_size} bytes")

    measurement_size = slot_size - 4

    print("Records")
    print("=" * 50)
    print(f"MeasurementRecord size: {measurement_size} bytes")
    print()

    for slot in range(header["slot_count"]):
        data = file.read(slot_size)

        if len(data) != slot_size:
            raise ValueError(
                f"Incomplete record data in slot {slot}"
            )

        state = struct.unpack("<I", data[:4])[0]
        record_data = data[4:]

        if len(record_data) != 28:
            raise ValueError(
                "Unexpected MeasurementRecord size: "
                f"{len(record_data)} bytes (expected 28)"
            )

        (
            boot_epoch_id,
            timestamp,
            house_battery_voltage,
            engine_battery_voltage,
            temperature,
            humidity,
            water_alarm,
            smoke_alarm,
        ) = struct.unpack("<IIffff??2x", record_data)

        print(
            f"Slot {slot:4d} | "
            f"{state_name(state):20s}"
        )
        print(f"           Boot epoch ID:          {boot_epoch_id}")
        print(f"           Timestamp:              {timestamp}")
        print(f"           House battery voltage:  {house_battery_voltage:.2f} V")
        print(f"           Engine battery voltage: {engine_battery_voltage:.2f} V")
        print(f"           Temperature:            {temperature:.2f} °C")
        print(f"           Humidity:               {humidity:.2f} %")
        print(f"           Water alarm:            {water_alarm}")
        print(f"           Smoke alarm:            {smoke_alarm}")
        print()


def main():
    buffer_file = get_buffer_file()

    print(f"Buffer file: {buffer_file}")

    if not buffer_file.is_file():
        print()
        print("ERROR: measurements.bin was not found.")
        print()
        print("Copy the SD-card buffer file to:")
        print(f"  {DEFAULT_BUFFER_FILE}")
        print()
        print("Alternatively pass another file path:")
        print(
            "  python measurement_buffer_reader.py "
            "E:/buffer/measurements.bin"
        )
        sys.exit(1)

    file_size = buffer_file.stat().st_size

    try:
        with buffer_file.open("rb") as file:
            header = read_header(file)
            validate_header(header)

            slot_size = determine_slot_size(
                file_size,
                header["slot_count"],
            )

            print_header(
                header,
                file_size,
                slot_size,
            )

            print_slots(
                file,
                header,
                slot_size,
            )

    except (OSError, ValueError, struct.error) as error:
        print()
        print(f"ERROR: {error}")
        sys.exit(1)


if __name__ == "__main__":
    main()
