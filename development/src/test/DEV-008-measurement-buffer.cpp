/*************************************************
 * File:        DEV-008-measurement-buffer.cpp
 *
 * Description:
 * Development test for the SD-card-based
 * measurement buffer.
 *
 * Test procedure:
 *
 * 1. Restart with PIN_SERIAL_DEBUG_ENABLE LOW.
 *
 *    Expected:
 *    - relative test records are written
 *    - one absolute test record is written
 *    - all written records are printed
 *
 * 2. Restart again with
 *    PIN_SERIAL_DEBUG_ENABLE LOW.
 *
 *    Expected:
 *    - old relative records are removed during
 *      initBuffer() because a new Boot Epoch exists
 *    - new relative records are written
 *    - writing the absolute record is rejected
 *      because the file already exists
 *
 * 3. Restart with PIN_SERIAL_DEBUG_ENABLE HIGH.
 *
 *    Expected:
 *    - relative records are removed during
 *      initBuffer()
 *    - the absolute record remains available
 *    - the absolute record is read and printed
 *    - removeOldestRecord() deletes the record
 *    - a second read confirms that no absolute
 *      record remains
 *
 * Expected SD card structure after first LOW start:
 *
 * /buffer/
 *   absolute/
 *     1750000000.bin
 *
 *   relative/
 *     <relativeTimestamp>.bin
 *************************************************/

#include <Arduino.h>

#include "config.h"

#include "app/runtime_manager.h"
#include "app/time_manager.h"
#include "app/sd_manager.h"
#include "app/measurement_buffer.h"
#include "app/measurement_record.h"

static constexpr uint8_t RELATIVE_TEST_RECORD_COUNT = 5;

static constexpr uint32_t TEST_ABSOLUTE_TIMESTAMP =
    1850000000UL;

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
        index;

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
 * Function:    writeRelativeTestRecords
 * Description: Writes and prints the relative
 *              measurement test records.
 * Parameters:  None
 * Returns:     None
 *************************************************/
static void writeRelativeTestRecords()
{
    Serial.println();
    Serial.println(
        "TEST: Writing relative records");

    for (uint8_t i = 1;
         i <= RELATIVE_TEST_RECORD_COUNT;
         ++i)
    {
        const MeasurementRecord testRecord =
            createRelativeTestRecord(i);

        Serial.println();

        Serial.printf(
            "Relative record %u:\n",
            i);

        printMeasurementRecord(
            testRecord);

        Serial.print(
            "Write status   : ");

        if (pushRecord(testRecord))
        {
            Serial.println("OK");
        }
        else
        {
            Serial.println("FAILED");
        }

        delay(1100);
    }
}


/*************************************************
 * Function:    writeAbsoluteTestRecord
 * Description: Writes and prints the absolute
 *              measurement test record.
 * Parameters:  None
 * Returns:     None
 * Notes:       The same fixed timestamp is used on
 *              every LOW startup. The second write
 *              must therefore be rejected.
 *************************************************/
static void writeAbsoluteTestRecord()
{
    const MeasurementRecord testRecord =
        createAbsoluteTestRecord();

    Serial.println();
    Serial.println(
        "TEST: Writing absolute record");

    printMeasurementRecord(
        testRecord);

    Serial.print(
        "Write status   : ");

    if (pushRecord(testRecord))
    {
        Serial.println("OK");
        Serial.println(
            "Expected       : OK on first LOW start");
    }
    else
    {
        Serial.println("FAILED");
        Serial.println(
            "Expected       : FAILED if absolute record already exists");
    }
}


/*************************************************
 * Function:    writeTestRecords
 * Description: Writes relative records followed
 *              by the fixed absolute test record.
 * Parameters:  None
 * Returns:     None
 *************************************************/
static void writeTestRecords()
{
    writeRelativeTestRecords();
    writeAbsoluteTestRecord();

    Serial.println();

    Serial.printf(
        "Record count after write: %u\n",
        getRecordCount());
}


/*************************************************
 * Function:    readAndRemoveAbsoluteRecord
 * Description: Reads, prints and removes the
 *              oldest absolute record and verifies
 *              that it has actually been deleted.
 * Parameters:  None
 * Returns:     None
 *************************************************/
static void readAndRemoveAbsoluteRecord()
{
    Serial.println();
    Serial.println(
        "TEST: Reading absolute record");

    if (!readOldestRecord(record))
    {
        Serial.println(
            "Read status    : FAILED");

        Serial.println(
            "No absolute record available");

        return;
    }

    Serial.println(
        "Read status    : OK");

    printMeasurementRecord(
        record);

    Serial.print(
        "Remove status  : ");

    if (!removeOldestRecord())
    {
        Serial.println("FAILED");
        return;
    }

    Serial.println("OK");

    Serial.println();
    Serial.println(
        "TEST: Verifying absolute record deletion");

    MeasurementRecord verificationRecord{};

    if (readOldestRecord(
            verificationRecord))
    {
        Serial.println(
            "Delete check   : FAILED");

        Serial.println(
            "Absolute record is still available");

        printMeasurementRecord(
            verificationRecord);

        return;
    }

    Serial.println(
        "Delete check   : OK");

    Serial.println(
        "No absolute record remains");
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
        "DEV-008 SD Measurement Buffer Test");
    Serial.println(
        "========================================");

    pinMode(
        PIN_SERIAL_DEBUG_ENABLE,
        INPUT_PULLUP);

    initRuntimeManager();
    initTimeManager();

    const bool newBootEpoch =
        hasNewBootEpoch();

    const bool writeMode =
        digitalRead(
            PIN_SERIAL_DEBUG_ENABLE) == LOW;

    Serial.printf(
        "New Boot Epoch: %s\n",
        newBootEpoch
            ? "YES"
            : "NO");

    Serial.printf(
        "Debug pin    : %s\n",
        writeMode
            ? "LOW"
            : "HIGH");

    Serial.printf(
        "Test mode    : %s\n",
        writeMode
            ? "WRITE"
            : "READ + DELETE");

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

    if (!initBuffer(newBootEpoch))
    {
        Serial.println(
            "Measurement buffer initialization FAILED");

        return;
    }

    Serial.println(
        "Measurement buffer initialization OK");

    printBufferStatus();

    if (writeMode)
    {
        writeTestRecords();
    }
    else
    {
        readAndRemoveAbsoluteRecord();
    }

    printBufferStatus();

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
