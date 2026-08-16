/*************************************************
 * File:        measurement_buffer.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Persistent measurement buffer implementation
 * using one binary file on the external SD card.
 *
 * Responsibilities:
 * - Store all measurement records in one file
 * - Keep absolute and relative record counters
 * - Retrieve absolute records chronologically
 * - Remove records without directory scans
 * - Compact the buffer file when required
 *
 *******************************************#******/
#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include "app/measurement_buffer.h"
#include "app/measurement_record.h"
#include "app/sd_manager.h"
#include "app/debug_logger.h"
#include "app/fault_manager.h"


//--------------------------------------------------
// Buffer file format
//--------------------------------------------------

static constexpr uint32_t BUFFER_FILE_MAGIC =
    0x4D425546UL; // "MBUF"

static constexpr uint32_t BUFFER_FILE_VERSION = 1;

static constexpr uint32_t RECORD_STATE_ACTIVE =
    0xA5A5A5A5UL;

static constexpr uint32_t RECORD_STATE_REMOVED =
    0x00000000UL;

struct BufferFileHeader
{
    uint32_t magic;
    uint32_t version;
    uint32_t slotCount;
    uint32_t activeCount;
    uint32_t absoluteCount;
    uint32_t relativeCount;
    uint32_t oldestAbsoluteSlot;
};

struct StoredMeasurementRecord
{
    uint32_t state;
    MeasurementRecord record;
};

static BufferFileHeader bufferHeader = {};
static bool bufferInitialized = false;


//--------------------------------------------------
// Helper functions
//--------------------------------------------------

/*************************************************
 * Function:    isAbsoluteTimestamp
 * Description: Determines whether a timestamp is
 *              treated as a valid Unix timestamp.
 * Parameters:  timestamp - Timestamp to evaluate
 * Returns:     true  - Absolute Unix timestamp
 *              false - Relative timestamp
 * Notes:       Relative runtime timestamps are
 *              expected to remain below the valid
 *              Unix timestamp range.
 *************************************************/
static bool isAbsoluteTimestamp(uint32_t timestamp)
{
    return timestamp >= VALID_TIMESTAMP_VALUE;
}

/*************************************************
 * Function:    createEmptyBufferFile
 * Description: Creates a new measurement buffer
 *              file with an initialized header.
 * Parameters:  None
 * Returns:     true  - Buffer file created
 *              false - File creation failed
 * Notes:       An existing buffer file is removed
 *              before the new file is created.
 *************************************************/
static bool createEmptyBufferFile()
{
    if (SD.exists(BUFFER_FILE_PATH))
    {
        if (!SD.remove(BUFFER_FILE_PATH))
        {
            LOG_ERROR(
                "Failed to remove existing buffer file");

            setFault(
                FaultCode::SD_WRITE_FAILED);

            return false;
        }
    }

    File file =
        SD.open(BUFFER_FILE_PATH, FILE_WRITE);

    if (!file)
    {
        LOG_ERROR(
            "Failed to create measurement buffer file");

        setFault(
            FaultCode::SD_WRITE_FAILED);

        return false;
    }

    bufferHeader.magic = BUFFER_FILE_MAGIC;
    bufferHeader.version = BUFFER_FILE_VERSION;
    bufferHeader.slotCount = 0;
    bufferHeader.activeCount = 0;
    bufferHeader.absoluteCount = 0;
    bufferHeader.relativeCount = 0;
    bufferHeader.oldestAbsoluteSlot = UINT32_MAX;

    const size_t writtenBytes =
        file.write(
            reinterpret_cast<const uint8_t*>(
                &bufferHeader),
            sizeof(bufferHeader));

    file.flush();
    file.close();

    if (writtenBytes != sizeof(bufferHeader))
    {
        LOG_ERROR(
            "Failed to write measurement buffer header");

        setFault(
            FaultCode::SD_WRITE_FAILED);

        return false;
    }

    return true;
}

/*************************************************
 * Function:    writeBufferHeader
 * Description: Writes the current in-memory buffer
 *              header to the buffer file.
 * Parameters:  None
 * Returns:     true  - Header written successfully
 *              false - Header write failed
 * Notes:       The header is stored at the beginning
 *              of the measurement buffer file.
 *************************************************/
static bool writeBufferHeader()
{
    File file =
        SD.open(BUFFER_FILE_PATH, "r+");

    if (!file)
    {
        LOG_ERROR(
            "Failed to open measurement buffer header for writing");

        setFault(
            FaultCode::SD_WRITE_FAILED);

        return false;
    }

    if (!file.seek(0))
    {
        file.close();

        LOG_ERROR(
            "Failed to seek measurement buffer header");

        setFault(
            FaultCode::SD_WRITE_FAILED);

        return false;
    }

    const size_t writtenBytes =
        file.write(
            reinterpret_cast<const uint8_t*>(
                &bufferHeader),
            sizeof(bufferHeader));

    file.flush();
    file.close();

    if (writtenBytes != sizeof(bufferHeader))
    {
        LOG_ERROR(
            "Incomplete measurement buffer header write");

        setFault(
            FaultCode::SD_WRITE_FAILED);

        return false;
    }

    return true;
}

/*************************************************
 * Function:    loadBufferHeader
 * Description: Loads and validates the persistent
 *              measurement buffer header.
 * Parameters:  None
 * Returns:     true  - Valid header loaded
 *              false - Header invalid or unreadable
 * Notes:       File size, format version and stored
 *              record counters are validated.
 *************************************************/
static bool loadBufferHeader()
{
    File file =
        SD.open(BUFFER_FILE_PATH, FILE_READ);

    if (!file)
    {
        return false;
    }

    if (file.size() < sizeof(BufferFileHeader))
    {
        file.close();
        return false;
    }

    const size_t readBytes =
        file.read(
            reinterpret_cast<uint8_t*>(
                &bufferHeader),
            sizeof(bufferHeader));

    const size_t expectedSize =
        sizeof(BufferFileHeader) +
        static_cast<size_t>(bufferHeader.slotCount) *
            sizeof(StoredMeasurementRecord);

    const size_t actualSize = file.size();

    file.close();

    if (readBytes != sizeof(bufferHeader) ||
        bufferHeader.magic != BUFFER_FILE_MAGIC ||
        bufferHeader.version != BUFFER_FILE_VERSION ||
        actualSize != expectedSize ||
        bufferHeader.activeCount !=
            bufferHeader.absoluteCount +
            bufferHeader.relativeCount ||
        bufferHeader.activeCount >
            bufferHeader.slotCount)
    {
        return false;
    }

    return true;
}

/*************************************************
 * Function:    getRecordOffset
 * Description: Calculates the byte offset of a
 *              stored measurement record slot.
 * Parameters:  slot - Record slot index
 * Returns:     Byte offset inside the buffer file
 * Notes:       Record slots have a fixed size and
 *              follow directly after the header.
 *************************************************/
static size_t getRecordOffset(uint32_t slot)
{
    return sizeof(BufferFileHeader) +
           static_cast<size_t>(slot) *
               sizeof(StoredMeasurementRecord);
}

/*************************************************
 * Function:    readStoredRecord
 * Description: Reads a stored measurement record
 *              from a specific buffer slot.
 * Parameters:  slot         - Record slot index
 *              storedRecord - Destination record
 * Returns:     true  - Record read successfully
 *              false - Record read failed
 * Notes:       The record is accessed directly by
 *              its calculated file offset.
 *************************************************/
static bool readStoredRecord(
    uint32_t slot,
    StoredMeasurementRecord& storedRecord)
{
    if (slot >= bufferHeader.slotCount)
    {
        return false;
    }

    File file =
        SD.open(BUFFER_FILE_PATH, FILE_READ);

    if (!file)
    {
        setFault(
            FaultCode::SD_READ_FAILED);
        return false;
    }

    if (!file.seek(getRecordOffset(slot)))
    {
        file.close();
        setFault(
            FaultCode::SD_READ_FAILED);
        return false;
    }

    const size_t readBytes =
        file.read(
            reinterpret_cast<uint8_t*>(
                &storedRecord),
            sizeof(storedRecord));

    file.close();

    if (readBytes != sizeof(storedRecord))
    {
        setFault(
            FaultCode::SD_READ_FAILED);
        return false;
    }

    return true;
}

/*************************************************
 * Function:    writeRecordState
 * Description: Updates the state of a stored
 *              measurement record slot.
 * Parameters:  slot  - Record slot index
 *              state - New record state
 * Returns:     true  - State updated successfully
 *              false - State update failed
 * Notes:       Used to logically remove records
 *              without immediately rewriting the
 *              complete buffer file.
 *************************************************/
static bool writeRecordState(
    uint32_t slot,
    uint32_t state)
{
    File file =
        SD.open(BUFFER_FILE_PATH, "r+");

    if (!file)
    {
        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (!file.seek(getRecordOffset(slot)))
    {
        file.close();
        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    const size_t writtenBytes =
        file.write(
            reinterpret_cast<const uint8_t*>(
                &state),
            sizeof(state));

    file.flush();
    file.close();

    if (writtenBytes != sizeof(state))
    {
        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    return true;
}

/*************************************************
 * Function:    findNextAbsoluteSlot
 * Description: Finds the next active record with an
 *              absolute timestamp.
 * Parameters:  startSlot - First slot to inspect
 *              slot      - Resulting slot index
 * Returns:     true  - Absolute record found
 *              false - No absolute record found
 * Notes:       Scans the binary buffer sequentially
 *              starting at the specified slot.
 *************************************************/
static bool findNextAbsoluteSlot(
    uint32_t startSlot,
    uint32_t& slot)
{
    if (startSlot >= bufferHeader.slotCount)
    {
        return false;
    }

    File file =
        SD.open(BUFFER_FILE_PATH, FILE_READ);

    if (!file)
    {
        setFault(
            FaultCode::SD_READ_FAILED);
        return false;
    }

    if (!file.seek(getRecordOffset(startSlot)))
    {
        file.close();
        setFault(
            FaultCode::SD_READ_FAILED);
        return false;
    }

    StoredMeasurementRecord storedRecord;

    for (uint32_t currentSlot = startSlot;
         currentSlot < bufferHeader.slotCount;
         ++currentSlot)
    {
        const size_t readBytes =
            file.read(
                reinterpret_cast<uint8_t*>(
                    &storedRecord),
                sizeof(storedRecord));

        if (readBytes != sizeof(storedRecord))
        {
            file.close();
            setFault(
                FaultCode::SD_READ_FAILED);
            return false;
        }

        if (storedRecord.state == RECORD_STATE_ACTIVE &&
            isAbsoluteTimestamp(
                storedRecord.record.timestamp))
        {
            slot = currentSlot;
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

/*************************************************
 * Function:    rebuildBufferHeader
 * Description: Reconstructs the measurement buffer
 *              metadata from stored record slots.
 * Parameters:  None
 * Returns:     true  - Header rebuilt successfully
 *              false - Header rebuild failed
 * Notes:       Performs one sequential file scan and
 *              is only required if the persistent
 *              header is invalid or inconsistent.
 *************************************************/
static bool rebuildBufferHeader()
{
    File file =
        SD.open(BUFFER_FILE_PATH, FILE_READ);

    if (!file ||
        file.size() < sizeof(BufferFileHeader))
    {
        if (file)
        {
            file.close();
        }
        return false;
    }

    const size_t payloadSize =
        file.size() - sizeof(BufferFileHeader);

    if (payloadSize %
            sizeof(StoredMeasurementRecord) != 0)
    {
        file.close();

        LOG_ERROR(
            "Measurement buffer file has invalid size");

        setFault(
            FaultCode::SD_READ_FAILED);

        return false;
    }

    bufferHeader.magic = BUFFER_FILE_MAGIC;
    bufferHeader.version = BUFFER_FILE_VERSION;
    bufferHeader.slotCount =
        payloadSize /
        sizeof(StoredMeasurementRecord);
    bufferHeader.activeCount = 0;
    bufferHeader.absoluteCount = 0;
    bufferHeader.relativeCount = 0;
    bufferHeader.oldestAbsoluteSlot = UINT32_MAX;

    if (!file.seek(sizeof(BufferFileHeader)))
    {
        file.close();
        return false;
    }

    StoredMeasurementRecord storedRecord;

    for (uint32_t slot = 0;
         slot < bufferHeader.slotCount;
         ++slot)
    {
        const size_t readBytes =
            file.read(
                reinterpret_cast<uint8_t*>(
                    &storedRecord),
                sizeof(storedRecord));

        if (readBytes != sizeof(storedRecord))
        {
            file.close();
            return false;
        }

        if (storedRecord.state != RECORD_STATE_ACTIVE)
        {
            continue;
        }

        bufferHeader.activeCount++;

        if (isAbsoluteTimestamp(
                storedRecord.record.timestamp))
        {
            bufferHeader.absoluteCount++;

            if (bufferHeader.oldestAbsoluteSlot ==
                UINT32_MAX)
            {
                bufferHeader.oldestAbsoluteSlot = slot;
            }
        }
        else
        {
            bufferHeader.relativeCount++;
        }
    }

    file.close();

    return writeBufferHeader();
}

/*************************************************
 * Function:    compactBufferFile
 * Description: Rewrites active records into a new
 *              compact measurement buffer file.
 * Parameters:  None
 * Returns:     true  - Buffer compacted successfully
 *              false - Compaction failed
 * Notes:       Removed slots are discarded so their
 *              SD card storage can be reclaimed.
 *************************************************/
static bool compactBufferFile()
{
    File source =
        SD.open(BUFFER_FILE_PATH, FILE_READ);

    if (!source)
    {
        setFault(
            FaultCode::SD_READ_FAILED);
        return false;
    }

    if (SD.exists(BUFFER_TEMP_FILE_PATH))
    {
        SD.remove(BUFFER_TEMP_FILE_PATH);
    }

    File target =
        SD.open(BUFFER_TEMP_FILE_PATH, FILE_WRITE);

    if (!target)
    {
        source.close();
        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    BufferFileHeader newHeader = {};
    newHeader.magic = BUFFER_FILE_MAGIC;
    newHeader.version = BUFFER_FILE_VERSION;
    newHeader.oldestAbsoluteSlot = UINT32_MAX;

    if (target.write(
            reinterpret_cast<const uint8_t*>(
                &newHeader),
            sizeof(newHeader)) != sizeof(newHeader))
    {
        source.close();
        target.close();
        SD.remove(BUFFER_TEMP_FILE_PATH);
        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (!source.seek(sizeof(BufferFileHeader)))
    {
        source.close();
        target.close();
        SD.remove(BUFFER_TEMP_FILE_PATH);
        return false;
    }

    StoredMeasurementRecord storedRecord;

    for (uint32_t slot = 0;
         slot < bufferHeader.slotCount;
         ++slot)
    {
        const size_t readBytes =
            source.read(
                reinterpret_cast<uint8_t*>(
                    &storedRecord),
                sizeof(storedRecord));

        if (readBytes != sizeof(storedRecord))
        {
            source.close();
            target.close();
            SD.remove(BUFFER_TEMP_FILE_PATH);
            setFault(
                FaultCode::SD_READ_FAILED);
            return false;
        }

        if (storedRecord.state != RECORD_STATE_ACTIVE)
        {
            continue;
        }

        if (target.write(
                reinterpret_cast<const uint8_t*>(
                    &storedRecord),
                sizeof(storedRecord)) !=
            sizeof(storedRecord))
        {
            source.close();
            target.close();
            SD.remove(BUFFER_TEMP_FILE_PATH);
            setFault(
                FaultCode::SD_WRITE_FAILED);
            return false;
        }

        if (isAbsoluteTimestamp(
                storedRecord.record.timestamp))
        {
            if (newHeader.oldestAbsoluteSlot ==
                UINT32_MAX)
            {
                newHeader.oldestAbsoluteSlot =
                    newHeader.slotCount;
            }

            newHeader.absoluteCount++;
        }
        else
        {
            newHeader.relativeCount++;
        }

        newHeader.slotCount++;
        newHeader.activeCount++;
    }

    source.close();

    if (!target.seek(0) ||
        target.write(
            reinterpret_cast<const uint8_t*>(
                &newHeader),
            sizeof(newHeader)) != sizeof(newHeader))
    {
        target.close();
        SD.remove(BUFFER_TEMP_FILE_PATH);
        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    target.flush();
    target.close();

    if (!SD.remove(BUFFER_FILE_PATH) ||
        !SD.rename(
            BUFFER_TEMP_FILE_PATH,
            BUFFER_FILE_PATH))
    {
        LOG_ERROR(
            "Failed to replace compacted measurement buffer file");

        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    bufferHeader = newHeader;

    LOG_INFO(
        "Measurement buffer compacted: records=%lu",
        static_cast<unsigned long>(
            bufferHeader.activeCount));

    return true;
}

/*************************************************
 * Function:    clearRelativeRecords
 * Description: Removes all active measurement
 *              records with relative timestamps.
 * Parameters:  None
 * Returns:     true  - Relative records removed
 *              false - Cleanup failed
 * Notes:       Called after creation of a new boot
 *              epoch because old relative timestamps
 *              can no longer be reconstructed.
 *************************************************/
static bool clearRelativeRecords()
{
    if (bufferHeader.relativeCount == 0)
    {
        return true;
    }

    File file =
        SD.open(BUFFER_FILE_PATH, "r+");

    if (!file)
    {
        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (!file.seek(sizeof(BufferFileHeader)))
    {
        file.close();
        return false;
    }

    StoredMeasurementRecord storedRecord;

    for (uint32_t slot = 0;
         slot < bufferHeader.slotCount;
         ++slot)
    {
        const size_t readBytes =
            file.read(
                reinterpret_cast<uint8_t*>(
                    &storedRecord),
                sizeof(storedRecord));

        if (readBytes != sizeof(storedRecord))
        {
            file.close();
            setFault(
                FaultCode::SD_READ_FAILED);
            return false;
        }

        if (storedRecord.state == RECORD_STATE_ACTIVE &&
            !isAbsoluteTimestamp(
                storedRecord.record.timestamp))
        {
            const size_t stateOffset =
                getRecordOffset(slot);

            if (!file.seek(stateOffset))
            {
                file.close();
                return false;
            }

            const uint32_t removedState =
                RECORD_STATE_REMOVED;

            if (file.write(
                    reinterpret_cast<const uint8_t*>(
                        &removedState),
                    sizeof(removedState)) !=
                sizeof(removedState))
            {
                file.close();
                setFault(
                    FaultCode::SD_WRITE_FAILED);
                return false;
            }

            // Continue with the next complete slot.
            if (!file.seek(
                    getRecordOffset(slot + 1)))
            {
                file.close();
                return false;
            }
        }
    }

    file.flush();
    file.close();

    bufferHeader.activeCount -=
        bufferHeader.relativeCount;
    bufferHeader.relativeCount = 0;

    if (!writeBufferHeader())
    {
        return false;
    }

    LOG_INFO(
        "Relative measurement records cleared");

    return compactBufferFile();
}

/*************************************************
 * Function:    removeOldestStoredRecord
 * Description: Removes the oldest active record from
 *              the persistent measurement buffer.
 * Parameters:  None
 * Returns:     true  - Record removed successfully
 *              false - No record removed
 * Notes:       Relative records are removed before
 *              absolute records when available.
 *************************************************/
static bool removeOldestStoredRecord()
{
    if (bufferHeader.activeCount == 0)
    {
        return false;
    }

    File file =
        SD.open(BUFFER_FILE_PATH, FILE_READ);

    if (!file)
    {
        setFault(
            FaultCode::SD_READ_FAILED);
        return false;
    }

    if (!file.seek(sizeof(BufferFileHeader)))
    {
        file.close();
        return false;
    }

    StoredMeasurementRecord storedRecord;
    uint32_t selectedSlot = UINT32_MAX;

    for (uint32_t slot = 0;
         slot < bufferHeader.slotCount;
         ++slot)
    {
        if (file.read(
                reinterpret_cast<uint8_t*>(
                    &storedRecord),
                sizeof(storedRecord)) !=
            sizeof(storedRecord))
        {
            file.close();
            return false;
        }

        if (storedRecord.state == RECORD_STATE_ACTIVE)
        {
            selectedSlot = slot;
            break;
        }
    }

    file.close();

    if (selectedSlot == UINT32_MAX)
    {
        return false;
    }

    if (!writeRecordState(
            selectedSlot,
            RECORD_STATE_REMOVED))
    {
        return false;
    }

    bufferHeader.activeCount--;

    if (isAbsoluteTimestamp(
            storedRecord.record.timestamp))
    {
        bufferHeader.absoluteCount--;

        if (selectedSlot ==
            bufferHeader.oldestAbsoluteSlot)
        {
            uint32_t nextSlot = UINT32_MAX;

            if (bufferHeader.absoluteCount > 0 &&
                findNextAbsoluteSlot(
                    selectedSlot + 1,
                    nextSlot))
            {
                bufferHeader.oldestAbsoluteSlot =
                    nextSlot;
            }
            else
            {
                bufferHeader.oldestAbsoluteSlot =
                    UINT32_MAX;
            }
        }
    }
    else
    {
        bufferHeader.relativeCount--;
    }

    return writeBufferHeader();
}

/*************************************************
 * Function:    ensureStorageSpace
 * Description: Ensures sufficient free SD card space
 *              before storing a new record.
 * Parameters:  None
 * Returns:     true  - Sufficient space available
 *              false - Space could not be freed
 * Notes:       Removed slots are compacted first. If
 *              required, oldest records are removed
 *              until the configured reserve exists.
 *************************************************/
static bool ensureStorageSpace()
{
    if (!isSdCardAvailable())
    {
        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (getSdFreeBytes() >= MIN_FREE_SPACE_BYTES)
    {
        return true;
    }

    // First reclaim slots that are already logically
    // removed from the queue.
    if (bufferHeader.slotCount >
        bufferHeader.activeCount)
    {
        if (!compactBufferFile())
        {
            return false;
        }
    }

    while (getSdFreeBytes() <
           MIN_FREE_SPACE_BYTES)
    {
        LOG_WARN(
            "SD card storage low: removing oldest buffered record");

        if (!removeOldestStoredRecord())
        {
            LOG_ERROR(
                "Unable to free SD card storage");

            setFault(
                FaultCode::SD_WRITE_FAILED);
            return false;
        }

        // Logical removal does not release FAT space;
        // compaction performs the physical reclaim.
        if (!compactBufferFile())
        {
            return false;
        }
    }

    return true;
}


//--------------------------------------------------
// Public buffer API
//--------------------------------------------------

/*************************************************
 * Function:    initBuffer
 * Description: Initializes the persistent single-file
 *              measurement buffer.
 * Parameters:  hasNewBootEpoch - Indicates whether
 *              a new boot epoch was created
 * Returns:     true  - Initialization successful
 *              false - Initialization failed
 * Notes:       Creates or validates the buffer file
 *              and removes relative records after a
 *              new boot epoch.
 *************************************************/
bool initBuffer(bool hasNewBootEpoch)
{
    LOG_INFO(
        "Initializing measurement buffer...");

    bufferInitialized = false;

    if (!isSdCardAvailable())
    {
        LOG_ERROR(
            "Measurement buffer initialization failed: "
            "SD card unavailable");

        setFault(
            FaultCode::SD_READ_FAILED);
        return false;
    }

    if (!createSdDirectory(
            BUFFER_ROOT_DIR))
    {
        LOG_ERROR(
            "Measurement buffer initialization failed: "
            "directory setup failed");
        return false;
    }

    if (!SD.exists(BUFFER_FILE_PATH))
    {
        if (!createEmptyBufferFile())
        {
            return false;
        }
    }
    else if (!loadBufferHeader())
    {
        LOG_WARN(
            "Measurement buffer header invalid: "
            "rebuilding metadata");

        if (!rebuildBufferHeader())
        {
            LOG_ERROR(
                "Measurement buffer initialization failed: "
                "metadata rebuild failed");
            return false;
        }
    }

    if (hasNewBootEpoch &&
        bufferHeader.relativeCount > 0)
    {
        LOG_INFO(
            "New Boot Epoch detected: "
            "clearing relative measurement records");

        if (!clearRelativeRecords())
        {
            LOG_ERROR(
                "Measurement buffer initialization failed: "
                "relative record cleanup failed");
            return false;
        }
    }

    bufferInitialized = true;

    LOG_INFO(
        "Measurement buffer initialized: "
        "absolute=%lu, relative=%lu, total=%lu",
        static_cast<unsigned long>(
            bufferHeader.absoluteCount),
        static_cast<unsigned long>(
            bufferHeader.relativeCount),
        static_cast<unsigned long>(
            bufferHeader.activeCount));

    return true;
}

/*************************************************
 * Function:    pushRecord
 * Description: Appends a measurement record to the
 *              persistent measurement buffer.
 * Parameters:  record - Measurement record to store
 * Returns:     true  - Record stored successfully
 *              false - Record storage failed
 * Notes:       Records are appended as fixed-size
 *              slots and the buffer metadata is
 *              updated after a successful write.
 *************************************************/
bool pushRecord(
    const MeasurementRecord& record)
{
    if (!bufferInitialized ||
        !isSdCardAvailable())
    {
        LOG_ERROR(
            "Cannot buffer measurement record: "
            "buffer unavailable");

        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (!ensureStorageSpace())
    {
        return false;
    }

    const bool absolute =
        isAbsoluteTimestamp(
            record.timestamp);

    StoredMeasurementRecord storedRecord = {};
    storedRecord.state = RECORD_STATE_ACTIVE;
    storedRecord.record = record;

    File file =
        SD.open(BUFFER_FILE_PATH, FILE_APPEND);

    if (!file)
    {
        LOG_ERROR(
            "Failed to open measurement buffer for append");

        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    const size_t writtenBytes =
        file.write(
            reinterpret_cast<const uint8_t*>(
                &storedRecord),
            sizeof(storedRecord));

    file.flush();
    file.close();

    if (writtenBytes != sizeof(storedRecord))
    {
        LOG_ERROR(
            "Incomplete measurement record write");

        setFault(
            FaultCode::SD_WRITE_FAILED);
        return false;
    }

    const uint32_t newSlot =
        bufferHeader.slotCount;

    bufferHeader.slotCount++;
    bufferHeader.activeCount++;

    if (absolute)
    {
        if (bufferHeader.absoluteCount == 0)
        {
            bufferHeader.oldestAbsoluteSlot =
                newSlot;
        }

        bufferHeader.absoluteCount++;
    }
    else
    {
        bufferHeader.relativeCount++;
    }

    if (!writeBufferHeader())
    {
        return false;
    }

    LOG_INFO(
        "Record status: STORED "
        "[timestamp=%lu, type=%s, total=%lu]",
        static_cast<unsigned long>(
            record.timestamp),
        absolute ? "ABSOLUTE" : "RELATIVE",
        static_cast<unsigned long>(
            bufferHeader.activeCount));

    return true;
}

/*************************************************
 * Function:    readOldestRecord
 * Description: Reads the oldest active measurement
 *              record with an absolute timestamp.
 * Parameters:  record - Destination measurement
 *                       record
 * Returns:     true  - Absolute record available
 *              false - No readable absolute record
 * Notes:       The stored oldest slot index allows
 *              direct access without a directory
 *              scan.
 *************************************************/
bool readOldestRecord(
    MeasurementRecord& record)
{
    if (!bufferInitialized ||
        bufferHeader.absoluteCount == 0 ||
        bufferHeader.oldestAbsoluteSlot == UINT32_MAX)
    {
        LOG_DEBUG(
            "No absolute measurement record available");
        return false;
    }

    StoredMeasurementRecord storedRecord;

    if (!readStoredRecord(
            bufferHeader.oldestAbsoluteSlot,
            storedRecord) ||
        storedRecord.state != RECORD_STATE_ACTIVE ||
        !isAbsoluteTimestamp(
            storedRecord.record.timestamp))
    {
        LOG_ERROR(
            "Failed to read oldest measurement record");

        setFault(
            FaultCode::SD_READ_FAILED);
        return false;
    }

    record = storedRecord.record;

    LOG_INFO(
        "Record status: LOADED "
        "[timestamp=%lu, slot=%lu]",
        static_cast<unsigned long>(
            record.timestamp),
        static_cast<unsigned long>(
            bufferHeader.oldestAbsoluteSlot));

    return true;
}

/*************************************************
 * Function:    removeOldestRecord
 * Description: Removes the oldest active absolute
 *              measurement record after transmission.
 * Parameters:  None
 * Returns:     true  - Record removed successfully
 *              false - No absolute record removed
 * Notes:       Removal is logical. The record slot is
 *              reclaimed later during compaction.
 *************************************************/
bool removeOldestRecord()
{
    if (!bufferInitialized ||
        bufferHeader.absoluteCount == 0 ||
        bufferHeader.oldestAbsoluteSlot == UINT32_MAX)
    {
        LOG_DEBUG(
            "Cannot remove buffered record: "
            "no absolute record available");
        return false;
    }

    const uint32_t removedSlot =
        bufferHeader.oldestAbsoluteSlot;

    if (!writeRecordState(
            removedSlot,
            RECORD_STATE_REMOVED))
    {
        return false;
    }

    bufferHeader.activeCount--;
    bufferHeader.absoluteCount--;

    if (bufferHeader.absoluteCount == 0)
    {
        bufferHeader.oldestAbsoluteSlot =
            UINT32_MAX;
    }
    else
    {
        uint32_t nextSlot = UINT32_MAX;

        if (!findNextAbsoluteSlot(
                removedSlot + 1,
                nextSlot))
        {
            LOG_ERROR(
                "Failed to locate next absolute measurement record");

            setFault(
                FaultCode::SD_READ_FAILED);
            return false;
        }

        bufferHeader.oldestAbsoluteSlot =
            nextSlot;
    }

    if (!writeBufferHeader())
    {
        return false;
    }

    LOG_DEBUG(
        "Buffered record removed: slot=%lu",
        static_cast<unsigned long>(
            removedSlot));

    if (bufferHeader.activeCount == 0)
    {
        LOG_INFO(
            "Buffer status: EMPTY");
    }

    return true;
}

/*************************************************
 * Function:    isBufferEmpty
 * Description: Checks whether the measurement buffer
 *              contains any active records.
 * Parameters:  None
 * Returns:     true  - Buffer is empty
 *              false - Buffer contains records
 * Notes:       An uninitialized buffer is treated as
 *              empty.
 *************************************************/
bool isBufferEmpty()
{
    return !bufferInitialized ||
           bufferHeader.activeCount == 0;
}

/*************************************************
 * Function:    getRecordCount
 * Description: Returns the total number of active
 *              measurement records.
 * Parameters:  None
 * Returns:     Number of active records
 * Notes:       The value is read directly from the
 *              in-memory buffer metadata.
 *************************************************/
uint16_t getRecordCount()
{
    const uint32_t count =
        bufferHeader.activeCount;

    return count > UINT16_MAX
        ? UINT16_MAX
        : static_cast<uint16_t>(count);
}

/*************************************************
 * Function:    getAbsoluteRecordCount
 * Description: Returns the number of active records
 *              with absolute timestamps.
 * Parameters:  None
 * Returns:     Number of active absolute records
 * Notes:       The value is read directly from the
 *              in-memory buffer metadata.
 *************************************************/
uint16_t getAbsoluteRecordCount()
{
    const uint32_t count =
        bufferHeader.absoluteCount;

    return count > UINT16_MAX
        ? UINT16_MAX
        : static_cast<uint16_t>(count);
}

/*************************************************
 * Function:    getRelativeRecordCount
 * Description: Returns the number of active records
 *              with relative timestamps.
 * Parameters:  None
 * Returns:     Number of active relative records
 * Notes:       Relative records are not available for
 *              transmission until their timestamps
 *              can be reconstructed.
 *************************************************/
uint16_t getRelativeRecordCount()
{
    const uint32_t count =
        bufferHeader.relativeCount;

    return count > UINT16_MAX
        ? UINT16_MAX
        : static_cast<uint16_t>(count);
}
