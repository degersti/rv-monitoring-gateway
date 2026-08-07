/*************************************************
 * File:        measurement_buffer.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Persistent measurement buffer implementation
 * using the external SD card.
 *
 * Responsibilities:
 * - Store measurement records as binary files
 * - Separate absolute and relative timestamps
 * - Retrieve absolute records chronologically
 * - Reconstruct buffer state from the SD card
 *
 *******************************************#******/
#include <Arduino.h>
#include <SD.h>

#include "config.h"
#include "app/measurement_buffer.h"
#include "app/sd_manager.h"
#include "app/debug_logger.h"
#include "app/fault_manager.h"

//--------------------------------------------------
// Buffer paths
//--------------------------------------------------
static constexpr const char* BUFFER_ROOT_DIR =
    "/buffer";

static constexpr const char* BUFFER_ABSOLUTE_DIR =
    "/buffer/absolute";

static constexpr const char* BUFFER_RELATIVE_DIR =
    "/buffer/relative";

static constexpr const char* BUFFER_FILE_EXTENSION =
    ".bin";

// Unix timestamps are clearly separated from the
// relative runtime timestamps used before time sync.
static constexpr uint32_t MIN_VALID_UNIX_TIMESTAMP =
    1000000000UL;

// Overflow information is runtime-only.
// ADR-2.02 explicitly forbids persistent buffer
// metadata outside the measurement records.
static uint32_t overflowCounter = 0;

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
 *              expected to remain far below the
 *              Unix timestamp range.
 *************************************************/
static bool isAbsoluteTimestamp(uint32_t timestamp)
{
    return timestamp >= MIN_VALID_UNIX_TIMESTAMP;
}

/*************************************************
 * Function:    isValidRecordFileName
 * Description: Validates a measurement buffer
 *              file name and extracts its
 *              timestamp.
 * Parameters:  name      - File name or path
 *              timestamp - Extracted timestamp
 * Returns:     true  - Valid <timestamp>.bin file
 *              false - Invalid file name
 * Notes:       Only decimal timestamp file names
 *              with the .bin extension are valid.
 *************************************************/
static bool isValidRecordFileName(
    const char* name,
    uint32_t& timestamp)
{
    if (name == nullptr)
    {
        return false;
    }

    const char* fileName = strrchr(name, '/');

    if (fileName != nullptr)
    {
        fileName++;
    }
    else
    {
        fileName = name;
    }

    const size_t length = strlen(fileName);
    const size_t extensionLength =
        strlen(BUFFER_FILE_EXTENSION);

    if (length <= extensionLength)
    {
        return false;
    }

    if (strcmp(
            fileName + length - extensionLength,
            BUFFER_FILE_EXTENSION) != 0)
    {
        return false;
    }

    uint64_t value = 0;

    for (size_t i = 0;
         i < length - extensionLength;
         ++i)
    {
        const char character = fileName[i];

        if (character < '0' || character > '9')
        {
            return false;
        }

        value =
            value * 10ULL +
            static_cast<uint64_t>(character - '0');

        if (value > UINT32_MAX)
        {
            return false;
        }
    }

    timestamp = static_cast<uint32_t>(value);

    return true;
}

/*************************************************
 * Function:    buildRecordPath
 * Description: Creates the full SD path for one
 *              measurement record.
 * Parameters:  directory - Buffer directory
 *              timestamp - Record timestamp
 * Returns:     Complete file path.
 * Notes:       File names directly represent the
 *              chronological record order.
 *************************************************/
static String buildRecordPath(
    const char* directory,
    uint32_t timestamp)
{
    return String(directory) +
           "/" +
           String(timestamp) +
           BUFFER_FILE_EXTENSION;
}

/*************************************************
 * Function:    writeRecordFile
 * Description: Writes one complete measurement
 *              record to the SD card.
 * Parameters:  path   - Target file path
 *              record - Measurement record
 * Returns:     true  - Record stored completely
 *              false - Write failed
 * Notes:       Existing files are rejected to
 *              prevent silent timestamp
 *              collisions or data replacement.
 *************************************************/
static bool writeRecordFile(
    const char* path,
    const MeasurementRecord& record)
{
    if (!isSdCardAvailable())
    {
        LOG_ERROR(
            "Cannot write measurement record: SD card unavailable");

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (path == nullptr)
    {
        LOG_ERROR(
            "Cannot write measurement record: path is null");

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (SD.exists(path))
    {
        LOG_ERROR(
            "Measurement record already exists: %s",
            path);

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    File file = SD.open(path, FILE_WRITE);

    if (!file)
    {
        LOG_ERROR(
            "Failed to create measurement record: %s",
            path);

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    const size_t writtenBytes =
        file.write(
            reinterpret_cast<const uint8_t*>(&record),
            sizeof(record));

    file.flush();
    file.close();

    if (writtenBytes != sizeof(record))
    {
        LOG_ERROR(
            "Incomplete measurement record write: %s [%u/%u bytes]",
            path,
            static_cast<unsigned>(writtenBytes),
            static_cast<unsigned>(sizeof(record)));

        SD.remove(path);

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    File verifyFile = SD.open(path, FILE_READ);

    if (!verifyFile)
    {
        LOG_ERROR(
            "Failed to verify measurement record: %s",
            path);

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    const size_t storedSize = verifyFile.size();
    verifyFile.close();

    if (storedSize != sizeof(record))
    {
        LOG_ERROR(
            "Invalid stored measurement record size: %s [%u/%u bytes]",
            path,
            static_cast<unsigned>(storedSize),
            static_cast<unsigned>(sizeof(record)));

        SD.remove(path);

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    clearFault(FaultCode::SD_WRITE_FAILED);

    return true;
}

/*************************************************
 * Function:    readRecordFile
 * Description: Reads one complete measurement
 *              record from the SD card.
 * Parameters:  path   - Source file path
 *              record - Destination record
 * Returns:     true  - Record loaded completely
 *              false - Read failed
 * Notes:       Invalid record sizes are treated
 *              as read failures.
 *************************************************/
static bool readRecordFile(
    const char* path,
    MeasurementRecord& record)
{
    if (!isSdCardAvailable())
    {
        LOG_ERROR(
            "Cannot read measurement record: SD card unavailable");

        setFault(FaultCode::SD_READ_FAILED);
        return false;
    }

    File file = SD.open(path, FILE_READ);

    if (!file)
    {
        LOG_ERROR(
            "Failed to open measurement record: %s",
            path);

        setFault(FaultCode::SD_READ_FAILED);
        return false;
    }

    if (file.isDirectory() ||
        file.size() != sizeof(record))
    {
        LOG_ERROR(
            "Invalid measurement record file: %s",
            path);

        file.close();

        setFault(FaultCode::SD_READ_FAILED);
        return false;
    }

    const size_t readBytes =
        file.read(
            reinterpret_cast<uint8_t*>(&record),
            sizeof(record));

    file.close();

    if (readBytes != sizeof(record))
    {
        LOG_ERROR(
            "Incomplete measurement record read: %s [%u/%u bytes]",
            path,
            static_cast<unsigned>(readBytes),
            static_cast<unsigned>(sizeof(record)));

        setFault(FaultCode::SD_READ_FAILED);
        return false;
    }

    clearFault(FaultCode::SD_READ_FAILED);

    return true;
}

/*************************************************
 * Function:    countValidRecords
 * Description: Counts valid measurement record
 *              files in one buffer directory.
 * Parameters:  directoryPath - Directory to scan
 *              count         - Resulting count
 * Returns:     true  - Directory scanned
 *              false - Directory access failed
 * Notes:       A valid record must have a valid
 *              timestamp file name and exactly
 *              sizeof(MeasurementRecord) bytes.
 *************************************************/
static bool countValidRecords(
    const char* directoryPath,
    uint32_t& count)
{
    count = 0;

    if (!isSdCardAvailable())
    {
        setFault(FaultCode::SD_READ_FAILED);
        return false;
    }

    File directory =
        SD.open(directoryPath, FILE_READ);

    if (!directory || !directory.isDirectory())
    {
        if (directory)
        {
            directory.close();
        }

        LOG_ERROR(
            "Failed to scan buffer directory: %s",
            directoryPath);

        setFault(FaultCode::SD_READ_FAILED);
        return false;
    }

    File file = directory.openNextFile();

    while (file)
    {
        if (!file.isDirectory())
        {
            uint32_t timestamp = 0;

            if (isValidRecordFileName(
                    file.name(),
                    timestamp) &&
                file.size() == sizeof(MeasurementRecord))
            {
                count++;
            }
        }

        file.close();
        file = directory.openNextFile();
    }

    directory.close();

    clearFault(FaultCode::SD_READ_FAILED);

    return true;
}

/*************************************************
 * Function:    findOldestRecord
 * Description: Finds the valid record with the
 *              smallest timestamp in one buffer
 *              directory.
 * Parameters:  directoryPath - Directory to scan
 *              path          - Oldest file path
 *              timestamp     - Oldest timestamp
 * Returns:     true  - Record found
 *              false - No valid record found or
 *                      directory access failed
 * Notes:       No persistent read index is used.
 *************************************************/
static bool findOldestRecord(
    const char* directoryPath,
    String& path,
    uint32_t& timestamp)
{
    path = "";
    timestamp = 0;

    if (!isSdCardAvailable())
    {
        setFault(FaultCode::SD_READ_FAILED);
        return false;
    }

    File directory =
        SD.open(directoryPath, FILE_READ);

    if (!directory || !directory.isDirectory())
    {
        if (directory)
        {
            directory.close();
        }

        LOG_ERROR(
            "Failed to open buffer directory: %s",
            directoryPath);

        setFault(FaultCode::SD_READ_FAILED);
        return false;
    }

    bool recordFound = false;
    File file = directory.openNextFile();

    while (file)
    {
        if (!file.isDirectory() &&
            file.size() == sizeof(MeasurementRecord))
        {
            uint32_t currentTimestamp = 0;

            if (isValidRecordFileName(
                    file.name(),
                    currentTimestamp))
            {
                if (!recordFound ||
                    currentTimestamp < timestamp)
                {
                    timestamp = currentTimestamp;
                    path = buildRecordPath(
                        directoryPath,
                        currentTimestamp);

                    recordFound = true;
                }
            }
        }

        file.close();
        file = directory.openNextFile();
    }

    directory.close();

    if (recordFound)
    {
        clearFault(FaultCode::SD_READ_FAILED);
    }

    return recordFound;
}

/*************************************************
 * Function:    removeRecordFile
 * Description: Removes one measurement record
 *              file from the SD card.
 * Parameters:  path - File path to remove
 * Returns:     true  - File removed
 *              false - Removal failed
 * Notes:       Used after successful transmission
 *              or when discarding the oldest
 *              record during buffer overflow.
 *************************************************/
static bool removeRecordFile(const char* path)
{
    if (!isSdCardAvailable())
    {
        LOG_ERROR(
            "Cannot remove measurement record: SD card unavailable");

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (!SD.remove(path))
    {
        LOG_ERROR(
            "Failed to remove measurement record: %s",
            path);

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    clearFault(FaultCode::SD_WRITE_FAILED);

    return true;
}

/*************************************************
 * Function:    removeOldestStoredRecord
 * Description: Removes the globally oldest valid
 *              buffered record from absolute or
 *              relative storage.
 * Parameters:  None
 * Returns:     true  - Oldest record removed
 *              false - No removable record found
 * Notes:       Used only to preserve the existing
 *              bounded-buffer overflow behavior.
 *************************************************/
static bool removeOldestStoredRecord()
{
    String absolutePath;
    String relativePath;

    uint32_t absoluteTimestamp = 0;
    uint32_t relativeTimestamp = 0;

    const bool hasAbsolute =
        findOldestRecord(
            BUFFER_ABSOLUTE_DIR,
            absolutePath,
            absoluteTimestamp);

    const bool hasRelative =
        findOldestRecord(
            BUFFER_RELATIVE_DIR,
            relativePath,
            relativeTimestamp);

    if (!hasAbsolute && !hasRelative)
    {
        return false;
    }

    // Relative records originate before a valid
    // absolute time reference was available and
    // therefore precede later absolute records of
    // the same continued Boot Epoch.
    //
    // If relative records still exist, discard the
    // oldest relative record first. Startup logic
    // must already have removed relative records
    // from invalid previous Boot Epochs.
    const String& path =
        hasRelative ? relativePath : absolutePath;

    if (!removeRecordFile(path.c_str()))
    {
        return false;
    }

    overflowCounter++;

    LOG_WARN(
        "Measurement buffer full: oldest record removed [%s, overflow=%lu]",
        path.c_str(),
        static_cast<unsigned long>(overflowCounter));

    return true;
}

//--------------------------------------------------
// Public buffer API
//--------------------------------------------------

/*************************************************
 * Function:    initBuffer
 * Description: Initializes the SD card based
 *              measurement buffer.
 * Parameters:  None
 * Returns:     true  - Initialization successful
 *              false - Initialization failed
 * Notes:       Creates required directories and
 *              reconstructs the buffer state by
 *              scanning the SD card.
 *************************************************/
bool initBuffer()
{
    LOG_INFO("Initializing measurement buffer...");

    overflowCounter = 0;

    if (!isSdCardAvailable())
    {
        LOG_ERROR(
            "Measurement buffer initialization failed: SD card unavailable");

        setFault(FaultCode::SD_READ_FAILED);
        return false;
    }

    if (!createSdDirectory(BUFFER_ROOT_DIR) ||
        !createSdDirectory(BUFFER_ABSOLUTE_DIR) ||
        !createSdDirectory(BUFFER_RELATIVE_DIR))
    {
        LOG_ERROR(
            "Measurement buffer initialization failed: directory setup failed");

        return false;
    }

    uint32_t absoluteCount = 0;
    uint32_t relativeCount = 0;

    if (!countValidRecords(
            BUFFER_ABSOLUTE_DIR,
            absoluteCount) ||
        !countValidRecords(
            BUFFER_RELATIVE_DIR,
            relativeCount))
    {
        LOG_ERROR(
            "Measurement buffer initialization failed: directory scan failed");

        return false;
    }

    LOG_INFO(
        "Measurement buffer initialized: absolute=%lu, relative=%lu, total=%lu",
        static_cast<unsigned long>(absoluteCount),
        static_cast<unsigned long>(relativeCount),
        static_cast<unsigned long>(
            absoluteCount + relativeCount));

    return true;
}

/*************************************************
 * Function:    pushRecord
 * Description: Stores a measurement record in the
 *              persistent measurement buffer.
 * Parameters:  record - Measurement record
 * Returns:     true  - Record stored
 *              false - Storage failed
 * Notes:       Absolute and relative records are
 *              stored in separate directories.
 *************************************************/
bool pushRecord(const MeasurementRecord& record)
{
    if (!isSdCardAvailable())
    {
        LOG_ERROR(
            "Cannot buffer measurement record: SD card unavailable");

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (isBufferFull())
    {
        if (!removeOldestStoredRecord())
        {
            LOG_ERROR(
                "Cannot free space in full measurement buffer");

            return false;
        }
    }

    const bool absolute =
        isAbsoluteTimestamp(record.timestamp);

    const char* directory =
        absolute
            ? BUFFER_ABSOLUTE_DIR
            : BUFFER_RELATIVE_DIR;

    const String path =
        buildRecordPath(
            directory,
            record.timestamp);

    LOG_INFO(
        "Record status: STORING [timestamp=%lu, type=%s]",
        static_cast<unsigned long>(record.timestamp),
        absolute ? "ABSOLUTE" : "RELATIVE");

    if (!writeRecordFile(
            path.c_str(),
            record))
    {
        LOG_ERROR(
            "Failed to store measurement record: %s",
            path.c_str());

        return false;
    }

    LOG_DEBUG(
        "Measurement record stored: %s",
        path.c_str());

    return true;
}

/*************************************************
 * Function:    readOldestRecord
 * Description: Returns the oldest transmissible
 *              measurement record without
 *              removing it from the buffer.
 * Parameters:  record - Measurement record
 * Returns:     true  - Absolute record available
 *              false - No absolute record
 * Notes:       ADR-2.02 permits transmission only
 *              from /buffer/absolute.
 *************************************************/
bool readOldestRecord(MeasurementRecord& record)
{
    String path;
    uint32_t timestamp = 0;

    if (!findOldestRecord(
            BUFFER_ABSOLUTE_DIR,
            path,
            timestamp))
    {
        LOG_DEBUG(
            "No absolute measurement record available");

        return false;
    }

    if (!readRecordFile(
            path.c_str(),
            record))
    {
        LOG_ERROR(
            "Failed to read oldest measurement record: %s",
            path.c_str());

        return false;
    }

    LOG_INFO(
        "Record status: LOADED [timestamp=%lu, file=%s]",
        static_cast<unsigned long>(record.timestamp),
        path.c_str());

    return true;
}

/*************************************************
 * Function:    removeOldestRecord
 * Description: Removes the oldest transmissible
 *              measurement record after
 *              successful transmission.
 * Parameters:  None
 * Returns:     true  - Record removed
 *              false - No absolute record or
 *                      removal failed
 * Notes:       Only absolute records are removed
 *              through the transmission API.
 *************************************************/
bool removeOldestRecord()
{
    String path;
    uint32_t timestamp = 0;

    if (!findOldestRecord(
            BUFFER_ABSOLUTE_DIR,
            path,
            timestamp))
    {
        LOG_DEBUG(
            "Cannot remove buffered record: no absolute record available");

        return false;
    }

    if (!removeRecordFile(path.c_str()))
    {
        return false;
    }

    LOG_DEBUG(
        "Buffered record removed: %s",
        path.c_str());

    if (isBufferEmpty())
    {
        LOG_INFO("Buffer status: EMPTY");
    }

    return true;
}

/*************************************************
 * Function:    isBufferFull
 * Description: Checks whether the configured
 *              measurement buffer limit has been
 *              reached.
 * Parameters:  None
 * Returns:     true  - Buffer is full
 *              false - Buffer has free capacity
 * Notes:       The count is reconstructed directly
 *              from valid files on the SD card.
 *************************************************/
bool isBufferFull()
{
    return getRecordCount() >=
           MEASUREMENT_BUFFER_SIZE;
}

/*************************************************
 * Function:    isBufferEmpty
 * Description: Checks whether the measurement
 *              buffer contains any valid records.
 * Parameters:  None
 * Returns:     true  - Buffer is empty
 *              false - Buffer contains records
 * Notes:       Both absolute and relative records
 *              are included.
 *************************************************/
bool isBufferEmpty()
{
    return getRecordCount() == 0;
}

/*************************************************
 * Function:    getRecordCount
 * Description: Returns the number of valid
 *              buffered measurement records.
 * Parameters:  None
 * Returns:     Number of records in absolute and
 *              relative buffer directories.
 * Notes:       No persistent record counter is
 *              maintained.
 *************************************************/
uint16_t getRecordCount()
{
    uint32_t absoluteCount = 0;
    uint32_t relativeCount = 0;

    if (!countValidRecords(
            BUFFER_ABSOLUTE_DIR,
            absoluteCount) ||
        !countValidRecords(
            BUFFER_RELATIVE_DIR,
            relativeCount))
    {
        return 0;
    }

    const uint32_t totalCount =
        absoluteCount + relativeCount;

    if (totalCount > UINT16_MAX)
    {
        return UINT16_MAX;
    }

    return static_cast<uint16_t>(
        totalCount);
}

/*************************************************
 * Function:    getOverflowCount
 * Description: Returns the runtime overflow count.
 * Parameters:  None
 * Returns:     Number of records discarded because
 *              the configured buffer limit was
 *              reached.
 * Notes:       The counter is intentionally not
 *              persisted because ADR-2.02 forbids
 *              separate persistent buffer metadata.
 *************************************************/
uint32_t getOverflowCount()
{
    return overflowCounter;
}

/*************************************************
 * Function:    resetOverflowCount
 * Description: Resets the runtime overflow count.
 * Parameters:  None
 * Returns:     true
 * Notes:       No persistent storage operation is
 *              required.
 *************************************************/
bool resetOverflowCount()
{
    overflowCounter = 0;

    return true;
}
