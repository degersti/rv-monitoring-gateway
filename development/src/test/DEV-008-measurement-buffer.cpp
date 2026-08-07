/*************************************************
 * File:        DEV-012-measurement-buffer.cpp
 *
 * Description:
 * Development test for the SD-card-based
 * measurement buffer.
 *
 * Test procedure:
 *
 * 1. Insert the SD card and start the ESP32.
 *
 * 2. Hold PIN_SERIAL_DEBUG_ENABLE LOW during
 *    startup to create five relative records and
 *    one absolute record.
 *
 * 3. Restart the ESP32 without holding the pin
 *    LOW.
 *
 * 4. Verify that:
 *    - the relative records are removed because a
 *      new Boot Epoch is detected
 *    - the absolute record remains available
 *    - readOldestRecord() returns the absolute
 *      record
 *    - removeOldestRecord() deletes the record
 *
 * 5. Restart the ESP32 again.
 *
 * 6. Verify that the buffer is empty.
 *
 * Expected SD card structure:
 *
 * /buffer/
 *   absolute/
 *     <timestamp>.bin
 *
 *   relative/
 *     <timestamp>.bin
 *
 * Notes:
 * - Records with a valid Unix timestamp are stored
 *   in /buffer/absolute.
 * - TEST_ABSOLUTE_TIMESTAMP is an artificial Unix
 *   timestamp used only by this development test.
 * - Records without a valid Unix timestamp are
 *   stored in /buffer/relative.
 * - readOldestRecord() only returns records from
 *   /buffer/absolute.
 *************************************************/

#include <Arduino.h>

#include "config.h"

#include "app/runtime_manager.h"
#include "app/time_manager.h"
#include "app/sd_manager.h"
#include "app/measurement_buffer.h"


static constexpr uint8_t RELATIVE_TEST_RECORD_COUNT = 5;
static constexpr uint32_t TEST_ABSOLUTE_TIMESTAMP = 1750000000UL;

static MeasurementRecord record;


/*************************************************
 * Function:    printMeasurementRecord
 * Description: Prints a measurement record to the
 *              serial console.
 * Parameters:  record - Measurement record
 * Returns:     None
 *************************************************/
static void printMeasurementRecord(
    const MeasurementRecord& record)
{
    Serial.println(
        "----------------------------------------");

    Serial.printf(
        "Boot Epoch ID : %lu\n",
        static_cast<unsigned long>(
            record.bootEpochId));

    Serial.printf(
        "Timestamp     : %lu\n",
        static_cast<unsigned long>(
            record.timestamp));

    Serial.printf(
        "House Battery : %.2f V\n",
        record.houseBatteryVoltage);

    Serial.printf(
        "Engine Battery: %.2f V\n",
        record.engineBatteryVoltage);

    Serial.printf(
        "Temperature   : %.2f °C\n",
        record.temperature);

    Serial.printf(
        "Humidity      : %.2f %%\n",
        record.humidity);

    Serial.printf(
        "Water Alarm   : %s\n",
        record.waterAlarm
            ? "YES"
            : "NO");

    Serial.printf(
        "Smoke Alarm   : %s\n",
        record.smokeAlarm
            ? "YES"
            : "NO");

    Serial.println(
        "----------------------------------------");
}


/*************************************************
 * Function:    createRelativeTestRecord
 * Description: Creates one relative measurement
 *              record with test data.
 * Parameters:  index - Test record number
 * Returns:     Initialized measurement record
 *************************************************/
static MeasurementRecord createRelativeTestRecord(
    uint8_t index)
{
    MeasurementRecord testRecord{};

    testRecord.bootEpochId =
        getBootEpochId();

    testRecord.timestamp =
        getCurrentTimestamp();

    testRecord.houseBatteryVoltage =
        12.0f + (index * 0.1f);

    testRecord.engineBatteryVoltage =
        12.5f + (index * 0.1f);

    testRecord.temperature =
        20.0f + index;

    testRecord.humidity =
        50.0f + index;

    testRecord.waterAlarm =
        index == 4;

    testRecord.smokeAlarm =
        index == 5;

    return testRecord;
}


/*************************************************
 * Function:    createAbsoluteTestRecord
 * Description: Creates one absolute measurement
 *              record with recognizable test data.
 * Parameters:  None
 * Returns:     Initialized measurement record
 *************************************************/
static MeasurementRecord createAbsoluteTestRecord()
{
    MeasurementRecord testRecord{};

    testRecord.bootEpochId =
        getBootEpochId();

    testRecord.timestamp =
        TEST_ABSOLUTE_TIMESTAMP;

    testRecord.houseBatteryVoltage = 13.25f;
    testRecord.engineBatteryVoltage = 14.10f;
    testRecord.temperature = 27.50f;
    testRecord.humidity = 63.00f;
    testRecord.waterAlarm = true;
    testRecord.smokeAlarm = false;

    return testRecord;
}


/*************************************************
 * Function:    writeTestRecord
 * Description: Prints and writes one test record.
 * Parameters:  type   - Record type label
 *              index  - Displayed record number
 *              record - Record to write
 * Returns:     true  - Record written
 *              false - Write failed
 *************************************************/
static bool writeTestRecord(
    const char* type,
    uint8_t index,
    const MeasurementRecord& record)
{
    Serial.println();

    Serial.printf(
        "Writing %s record %u:\n",
        type,
        index);

    printMeasurementRecord(record);

    Serial.print("Write status  : ");

    if (!pushRecord(record))
    {
        Serial.println("FAILED");
        return false;
    }

    Serial.println("OK");
    return true;
}


/*************************************************
 * Function:    writeTestRecords
 * Description: Writes five relative records and
 *              one absolute record.
 * Parameters:  None
 * Returns:     None
 *************************************************/
static void writeTestRecords()
{
    Serial.println();
    Serial.println(
        "TEST: Writing relative and absolute records");

    for (uint8_t i = 1;
         i <= RELATIVE_TEST_RECORD_COUNT;
         ++i)
    {
        const MeasurementRecord relativeRecord =
            createRelativeTestRecord(i);

        writeTestRecord(
            "RELATIVE",
            i,
            relativeRecord);

        // Ensure unique relative timestamps.
        delay(1100);
    }

    const MeasurementRecord absoluteRecord =
        createAbsoluteTestRecord();

    writeTestRecord(
        "ABSOLUTE",
        1,
        absoluteRecord);

    Serial.println();

    Serial.printf(
        "Record count after write: %u\n",
        getRecordCount());

    Serial.println(
        "Expected count          : 6");
}


/*************************************************
 * Function:    readAndRemoveTestRecords
 * Description: Reads all transmissible absolute
 *              records, prints their contents and
 *              removes them afterwards.
 * Parameters:  None
 * Returns:     None
 *************************************************/
static void readAndRemoveTestRecords()
{
    Serial.println();
    Serial.println(
        "TEST: Reading absolute measurement records");

    Serial.printf(
        "Buffered records before read: %u\n",
        getRecordCount());

    uint16_t readCount = 0;

    while (readOldestRecord(record))
    {
        readCount++;

        Serial.println();

        Serial.printf(
            "Read absolute record %u:\n",
            readCount);

        printMeasurementRecord(record);

        Serial.print("Remove status : ");

        if (!removeOldestRecord())
        {
            Serial.println("FAILED");
            break;
        }

        Serial.println("OK");
    }

    Serial.println();

    Serial.printf(
        "Absolute records read   : %u\n",
        readCount);

    Serial.printf(
        "Remaining record count  : %u\n",
        getRecordCount());

    Serial.printf(
        "Buffer empty            : %s\n",
        isBufferEmpty()
            ? "YES"
            : "NO");
}


/*************************************************
 * Function:    printBufferStatus
 * Description: Prints the current measurement
 *              buffer status.
 * Parameters:  None
 * Returns:     None
 *************************************************/
static void printBufferStatus()
{
    Serial.println();
    Serial.println(
        "Measurement buffer status:");

    Serial.printf(
        "Record count : %u\n",
        getRecordCount());

    Serial.printf(
        "Buffer empty : %s\n",
        isBufferEmpty()
            ? "YES"
            : "NO");

    Serial.printf(
        "Buffer full  : %s\n",
        isBufferFull()
            ? "YES"
            : "NO");

    Serial.printf(
        "Overflow     : %lu\n",
        static_cast<unsigned long>(
            getOverflowCount()));
}


/*************************************************
 * Function:    setupDevMeasurementBuffer
 * Description: Initializes and executes the
 *              measurement buffer development
 *              test.
 * Parameters:  None
 * Returns:     None
 *************************************************/
void setupDevMeasurementBuffer()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println(
        "========================================");
    Serial.println(
        "DEV-012 SD Measurement Buffer Test");
    Serial.println(
        "========================================");

    pinMode(
        PIN_SERIAL_DEBUG_ENABLE,
        INPUT_PULLUP);

    initRuntimeManager();
    initTimeManager();

    Serial.printf(
        "New Boot Epoch: %s\n",
        hasNewBootEpoch()
            ? "YES"
            : "NO");

    Serial.printf(
        "Debug pin    : %s\n",
        digitalRead(PIN_SERIAL_DEBUG_ENABLE) == LOW
            ? "LOW"
            : "HIGH");

    Serial.println();
    Serial.println(
        "Initializing SD card...");

    if (!initSdCard())
    {
        Serial.println(
            "SD card initialization FAILED");

        return;
    }

    Serial.println(
        "SD card initialization OK");

    Serial.println();
    Serial.println(
        "Initializing measurement buffer...");

    if (!initBuffer(hasNewBootEpoch()))
    {
        Serial.println(
            "Measurement buffer initialization FAILED");

        return;
    }

    Serial.println(
        "Measurement buffer initialization OK");

    printBufferStatus();

    /*
     * Jumper LOW:
     * Write five relative records and one absolute
     * record.
     *
     * Jumper HIGH:
     * Read, display and remove all remaining
     * absolute records.
     */
    if (digitalRead(
            PIN_SERIAL_DEBUG_ENABLE) == LOW)
    {
        writeTestRecords();
    }
    else
    {
        readAndRemoveTestRecords();
    }

    printBufferStatus();

    Serial.println();
    Serial.println(
        "Overflow counter test:");

    Serial.printf(
        "Overflow before reset: %lu\n",
        static_cast<unsigned long>(
            getOverflowCount()));

    Serial.printf(
        "Reset status: %s\n",
        resetOverflowCount()
            ? "OK"
            : "FAILED");

    Serial.printf(
        "Overflow after reset: %lu\n",
        static_cast<unsigned long>(
            getOverflowCount()));

    Serial.println();
    Serial.println(
        "========================================");
    Serial.println(
        "Measurement buffer test finished");
    Serial.println(
        "========================================");
}


/*************************************************
 * Function:    loopDevMeasurementBuffer
 * Description: Development test loop.
 * Parameters:  None
 * Returns:     None
 *************************************************/
void loopDevMeasurementBuffer()
{
    delay(1000);
}

