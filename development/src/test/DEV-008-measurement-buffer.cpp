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
 *    startup to create five measurement records.
 *
 * 3. Restart the ESP32 without holding the pin
 *    LOW.
 *
 * 4. Verify that:
 *    - initBuffer() detects the existing records
 *    - getRecordCount() reports the stored records
 *    - readOldestRecord() returns records in
 *      chronological order
 *    - removeOldestRecord() deletes the records
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
 * Function:    createTestRecord
 * Description: Creates one measurement record
 *              with test data.
 * Parameters:  index - Test record number
 * Returns:     Initialized measurement record
 *************************************************/
static MeasurementRecord createTestRecord(
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

    testRecord.waterAlarm = false;
    testRecord.smokeAlarm = false;

    return testRecord;
}


/*************************************************
 * Function:    writeTestRecords
 * Description: Writes five test records to the
 *              measurement buffer.
 * Parameters:  None
 * Returns:     None
 *************************************************/
static void writeTestRecords()
{
    Serial.println();
    Serial.println(
        "TEST: Writing measurement records");

    for (uint8_t i = 1; i <= 5; ++i)
    {
        MeasurementRecord testRecord =
            createTestRecord(i);

        Serial.printf(
            "Writing record %u "
            "[timestamp=%lu]... ",
            i,
            static_cast<unsigned long>(
                testRecord.timestamp));

        if (pushRecord(testRecord))
        {
            Serial.println("OK");
        }
        else
        {
            Serial.println("FAILED");
        }

        // Ensure unique timestamps.
        delay(1100);
    }

    Serial.printf(
        "Record count after write: %u\n",
        getRecordCount());
}


/*************************************************
 * Function:    readAndRemoveTestRecords
 * Description: Reads all transmissible records in
 *              chronological order and removes
 *              them afterwards.
 * Parameters:  None
 * Returns:     None
 *************************************************/
static void readAndRemoveTestRecords()
{
    Serial.println();
    Serial.println(
        "TEST: Reading measurement records");

    const uint16_t initialRecordCount =
        getRecordCount();

    Serial.printf(
        "Buffered records: %u\n",
        initialRecordCount);

    uint16_t readCount = 0;

    while (readOldestRecord(record))
    {
        readCount++;

        Serial.printf(
            "\nRecord %u:\n",
            readCount);

        printMeasurementRecord(record);

        Serial.print(
            "Removing record... ");

        if (removeOldestRecord())
        {
            Serial.println("OK");
        }
        else
        {
            Serial.println("FAILED");
            break;
        }
    }

    Serial.println();

    Serial.printf(
        "Records read and removed: %u\n",
        readCount);

    Serial.printf(
        "Remaining record count: %u\n",
        getRecordCount());

    Serial.printf(
        "Buffer empty: %s\n",
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

    if (!initBuffer())
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
     * Create five new measurement records.
     *
     * Jumper HIGH:
     * Read and remove existing absolute records.
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

