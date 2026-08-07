#pragma once

#include <Arduino.h>
#include "app/measurement_record.h"

/*************************************************
 * Function:    initBuffer
 * Description: Initializes the persistent
 *              measurement buffer.
 * Parameters:  None
 * Returns:     true  - Initialization successful
 *              false - Initialization failed
 * Notes:       Reconstructs the buffer state by
 *              scanning the SD card directories.
 *************************************************/
bool initBuffer(bool hasNewBootEpoch);

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
bool pushRecord(const MeasurementRecord& record);

/*************************************************
 * Function:    readOldestRecord
 * Description: Reads the oldest transmissible
 *              measurement record without
 *              removing it from the buffer.
 * Parameters:  record - Destination record
 * Returns:     true  - Absolute record available
 *              false - No absolute record or read
 *                      failed
 * Notes:       Only records from the absolute
 *              buffer directory are returned.
 *************************************************/
bool readOldestRecord(MeasurementRecord& record);

/*************************************************
 * Function:    removeOldestRecord
 * Description: Removes the oldest transmissible
 *              measurement record.
 * Parameters:  None
 * Returns:     true  - Record removed
 *              false - No absolute record or
 *                      removal failed
 * Notes:       Intended to be called only after
 *              successful transmission.
 *************************************************/
bool removeOldestRecord();

/*************************************************
 * Function:    isBufferFull
 * Description: Checks whether the configured
 *              measurement buffer limit has been
 *              reached.
 * Parameters:  None
 * Returns:     true  - Buffer full
 *              false - Free capacity available
 * Notes:       The current size is determined from
 *              valid files on the SD card.
 *************************************************/
bool isBufferFull();

/*************************************************
 * Function:    isBufferEmpty
 * Description: Checks whether the measurement
 *              buffer contains any records.
 * Parameters:  None
 * Returns:     true  - Buffer empty
 *              false - Records available
 * Notes:       Includes both absolute and relative
 *              records.
 *************************************************/
bool isBufferEmpty();

/*************************************************
 * Function:    getRecordCount
 * Description: Returns the number of currently
 *              buffered measurement records.
 * Parameters:  None
 * Returns:     Number of valid records in the
 *              absolute and relative directories.
 * Notes:       No persistent record counter is
 *              maintained.
 *************************************************/
uint16_t getRecordCount();

/*************************************************
 * Function:    getOverflowCount
 * Description: Returns the runtime buffer overflow
 *              counter.
 * Parameters:  None
 * Returns:     Number of discarded records caused
 *              by a full configured buffer.
 * Notes:       The counter is not persisted.
 *************************************************/
uint32_t getOverflowCount();

/*************************************************
 * Function:    resetOverflowCount
 * Description: Resets the runtime overflow counter.
 * Parameters:  None
 * Returns:     true
 * Notes:       No persistent storage operation is
 *              required.
 *************************************************/
bool resetOverflowCount();
