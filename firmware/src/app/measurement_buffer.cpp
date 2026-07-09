/*************************************************
 * File:        measurement_buffer.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Persistent measurement buffer implementation.
 *
 * Responsibilities:
 * - Store measurement records
 * - Retrieve measurement records
 * - Manage persistent ring buffer
 *
 *******************************************#******/
#include "config.h"
#include "stdint.h"
#include <Preferences.h>
#include "app/measurement_buffer.h"
#include "app/debug_logger.h"

// Ring buffer indices.
struct MeasurementBufferMetadata metaData;
// Valid measurement buffer metadata marker.
constexpr uint32_t BUFFER_METADATA_MAGIC = 0x4D425546;   // "MBUF"
//--------------------------------------------------
// Non-volatile storage instance.
// NVS namespace for measurement buffer data.
// NVS key for buffer metadata.
//--------------------------------------------------
static Preferences preferences;
constexpr const char* BUFFER_NAMESPACE = "meas_buf";
constexpr const char* METADATA_KEY = "metadata";

//------------------------------------------------
// Helper functions for read from / write to FLASH
//------------------------------------------------
/*************************************************
 * Function:    saveMetadata
 * Description: Stores the measurement buffer
 *              metadata in non-volatile storage.
 * Parameters:  None
 * Returns:     true  - Metadata stored
 *              false - Storage failed
 * Notes:       Internal helper function.
 *************************************************/
static bool saveMetadata()
{
    preferences.begin(BUFFER_NAMESPACE, false);

    size_t writtenBytes = preferences.putBytes(
        METADATA_KEY,
        &metaData,
        sizeof(metaData)
    );

    LOG_DEBUG("saveMetadata written bytes: %u", writtenBytes);
    LOG_DEBUG("sizeof(metaData): %u", sizeof(metaData));

    preferences.end();

    return writtenBytes == sizeof(metaData);
}
/*************************************************
 * Function:    loadMetadata
 * Description: Loads the measurement buffer
 *              metadata from non-volatile storage.
 * Parameters:  None
 * Returns:     true  - Valid metadata loaded
 *              false - No valid metadata found
 * Notes:       Internal helper function.
 *************************************************/
static bool loadMetadata()
{

    preferences.begin(BUFFER_NAMESPACE, false);

    size_t readBytes = preferences.getBytes(
        METADATA_KEY,
        &metaData,
        sizeof(metaData)
    );

    preferences.end();

    LOG_DEBUG("loadMetadata read bytes: %u", readBytes);
    LOG_DEBUG("sizeof(metaData): %u", sizeof(metaData));
    LOG_DEBUG("Magic read: 0x%08lX", metaData.magic);
    LOG_DEBUG("Magic exp : 0x%08lX", BUFFER_METADATA_MAGIC);

    if (readBytes != sizeof(metaData))
    {
        return false;
    }

    return metaData.magic == BUFFER_METADATA_MAGIC;
}

/*************************************************
 * Function:    getRecordKey
 * Description: Creates the NVS key for a record
 *              slot.
 * Parameters:  index - Buffer slot index
 * Returns:     NVS key string.
 * Notes:       Internal helper function.
 *************************************************/
static String getRecordKey(uint16_t index)
{
    return "rec_" + String(index);
}
/*************************************************
 * Function:    writeRecord
 * Description: Stores a measurement record in a
 *              buffer slot.
 * Parameters:  index  - Buffer slot index
 *              record - Measurement record
 * Returns:     true  - Record stored
 *              false - Storage failed
 * Notes:       Internal helper function.
 *************************************************/
static bool writeToFlash(uint16_t index, const MeasurementRecord& record)
{
    if (index >= MEASUREMENT_BUFFER_SIZE)
    {
        return false;
    }

    String key = getRecordKey(index);

    preferences.begin(BUFFER_NAMESPACE, false);

    size_t writtenBytes = preferences.putBytes(
        key.c_str(),
        &record,
        sizeof(record)
    );

    preferences.end();

    return writtenBytes == sizeof(record);
}
/*************************************************
 * Function:    readFromFlash
 * Description: Reads a measurement record from a
 *              buffer slot.
 * Parameters:  index  - Buffer slot index
 *              record - Measurement record
 * Returns:     true  - Record loaded
 *              false - Read failed
 * Notes:       Internal helper function.
 *************************************************/
static bool readFromFlash(uint16_t index, MeasurementRecord& record)
{
    if (index >= MEASUREMENT_BUFFER_SIZE)
    {
        return false;
    }

    String key = getRecordKey(index);

    preferences.begin(BUFFER_NAMESPACE, true);

    size_t readBytes = preferences.getBytes(
        key.c_str(),
        &record,
        sizeof(record)
    );

    preferences.end();

    return readBytes == sizeof(record);
}
//------------------------------------------------
// Implementation of ring buffer
//------------------------------------------------
/*************************************************
 * Function:    initBuffer
 * Description: Initializes the measurement
 *              buffer.
 * Parameters:  None
 * Returns:     true  - Initialization successful
 *              false - Initialization failed
 * Notes:       Loads metadata from Flash or
 *              creates a new buffer state.
 *************************************************/
bool initBuffer()
{
    LOG_INFO("Initializing measurement buffer...");
    if (loadMetadata())
    { 
        LOG_DEBUG("Measurement buffer metadata loaded.");
        LOG_DEBUG("ReadIndex : %u", metaData.readIndex);
        LOG_DEBUG("WriteIndex: %u", metaData.writeIndex);
        LOG_DEBUG("Count     : %u", metaData.recordCount);
        LOG_DEBUG("Overflow  : %lu", metaData.overflowCounter);

        return true;
    }

    metaData.magic = BUFFER_METADATA_MAGIC;
    metaData.writeIndex = 0;
    metaData.readIndex = 0;
    metaData.recordCount = 0;
    metaData.overflowCounter = 0;

    return saveMetadata();
}
/*************************************************
 * Function:    pushRecord
 * Description: Stores a measurement record in the
 *              measurement buffer.
 * Parameters:  record - Measurement record
 * Returns:     true  - Record stored
 *              false - Storage failed
 * Notes:       Overwrites the oldest record if
 *              the buffer is full.
 *************************************************/
bool pushRecord(const MeasurementRecord& record)
{
    LOG_INFO("Storing measurement record in buffer at index %u", metaData.writeIndex);
    if (isFull())
    {
        // Discard oldest record.
        LOG_WARN("Measurement buffer full. Overwriting oldest record.");
        metaData.readIndex = (metaData.readIndex + 1) % MEASUREMENT_BUFFER_SIZE;

        metaData.overflowCounter++;
    }
    else
    {
        LOG_INFO("Storing measurement record at index %u", metaData.writeIndex);
        metaData.recordCount++;
    }

    if (!writeToFlash(metaData.writeIndex, record))
    {
        return false;
    }

    metaData.writeIndex = (metaData.writeIndex + 1) % MEASUREMENT_BUFFER_SIZE;

    return saveMetadata();
}
/*************************************************
 * Function:    readOldestRecord
 * Description: Returns the oldest measurement
 *              record without removing it from
 *              the buffer.
 * Parameters:  record - Measurement record
 * Returns:     true  - Record available
 *              false - Buffer is empty
 * Notes:       None
 *************************************************/
bool readOldestRecord(MeasurementRecord& record)
{
    if (isEmpty())
    {
        return false;
    }

    // record = buffer[metaData.readIndex]; 
    readFromFlash(metaData.readIndex, record);

    return true;
}
/*************************************************
 * Function:    confirmMeasurementRecordSent
 * Description: Removes the oldest measurement
 *              record from the buffer after
 *              successful transmission.
 * Parameters:  None
 * Returns:     true  - Record removed
 *              false - Buffer is empty
 * Notes:       Advances the read index.
 *************************************************/
bool removeOldestRecord()
{
    if (isEmpty())
    {
        return false;
    }

    metaData.readIndex = (metaData.readIndex + 1) % MEASUREMENT_BUFFER_SIZE;
    metaData.recordCount--;

    return saveMetadata();
}
/*************************************************
 * Function:    isFull
 * Description: Checks whether the measurement
 *              buffer is full.
 * Parameters:  None
 * Returns:     true  - Buffer is full
 *              false - Buffer has free space
 * Notes:       None
 *************************************************/
bool isFull()
{
    return metaData.recordCount >= MEASUREMENT_BUFFER_SIZE;
}
/*************************************************
 * Function:    isEmpty
 * Description: Checks whether the measurement
 *              buffer is empty.
 * Parameters:  None
 * Returns:     true  - Buffer is empty
 *              false - Buffer contains records
 * Notes:       None
 *************************************************/
bool isEmpty()
{
    return metaData.recordCount == 0;
}
/*************************************************
 * Function:    getRecordCount
 * Description: Returns the number of records
 *              currently stored in the measurement
 *              buffer.
 * Parameters:  None
 * Returns:     Number of stored records.
 * Notes:       None
 *************************************************/
uint16_t getRecordCount()
{
    return metaData.recordCount;
}
/*************************************************
 * Function:    getOverflowCount
 * Description: Returns the number of measurement
 *              records that have been overwritten
 *              due to a full buffer.
 * Parameters:  None
 * Returns:     Number of overwritten records.
 * Notes:       Counter is reset after successful
 *              transmission to the backend.
 *************************************************/
uint32_t getOverflowCount()
{
    return metaData.overflowCounter;
}
/*************************************************
 * Function:    resetOverflowCount
 * Description: Resets the overflow counter.
 * Parameters:  None
 * Returns:     true  - Counter reset
 *              false - Storage failed
 * Notes:       Call after the overflow count has
 *              been successfully transmitted.
 *************************************************/
bool resetOverflowCount()
{
    metaData.overflowCounter = 0;

    return saveMetadata();
}
