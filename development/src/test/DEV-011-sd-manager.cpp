/*************************************************
 * File:        DEV-011-sd-manager.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Development test for the SD card manager.
 *
 * Tested functions:
 * - initSdCard()
 * - isSdCardAvailable()
 * - createSdDirectory()
 * - getSdTotalBytes()
 * - getSdUsedBytes()
 * - getSdFreeBytes()
 *
 * Additional tests:
 * - Create a test directory
 * - Create and write a test file
 * - Read and validate the test file
 * - Delete the test file
 * - Verify directory persistence
 *
 * Test instructions:
 *
 * 1. Insert a FAT32-formatted SD card into the
 *    SD card module.
 *
 * 2. Verify that the SD card module is connected
 *    to the pins configured in config.h:
 *
 *      PIN_SD_CS
 *      PIN_SD_MOSI
 *      PIN_SD_MISO
 *      PIN_SD_SCK
 *
 * 3. Compile and upload the firmware with
 *    setupDevSdManager() called from setup() and
 *    loopDevSdManager() called from loop().
 *
 * 4. Open the serial monitor at 115200 baud.
 *
 * 5. The following tests should report PASS:
 *
 *      SD card initialization
 *      SD card availability
 *      SD card capacity
 *      Test directory creation
 *      Existing directory access
 *      Test file creation
 *      Test file write
 *      Test file read
 *      Test file content validation
 *      Test file deletion
 *
 * 6. Remove the SD card and restart the device.
 *    The initialization test should then report
 *    FAIL and SD_INIT_FAILED should be set.
 *
 * 7. Reinsert the SD card and restart the device.
 *    All tests should pass again.
 *
 * Expected result:
 *
 * - A directory named /dev011 is created.
 * - A temporary file is written, read and deleted.
 * - The directory remains on the SD card after
 *   the test.
 * - The test sequence runs once after startup.
 *
 * Notes:
 * - This test writes temporarily to the SD card.
 * - Existing files outside /dev011 are not changed.
 * - The test file is deleted after a successful
 *   test run.
 * - The test directory is intentionally retained.
 *************************************************/

#include <Arduino.h>
#include <SD.h>

#include "app/sd_manager.h"
#include "app/debug_logger.h"

static constexpr const char* TEST_DIRECTORY =
    "/dev011";

static constexpr const char* TEST_FILE_PATH =
    "/dev011/sd_manager_test.txt";

static constexpr const char* TEST_FILE_CONTENT =
    "DEV-011 SD manager test";

static bool testCompleted = false;

/*************************************************
 * Function:    printTestResult
 * Description: Prints the result of an individual
 *              development test.
 * Parameters:  testName - Name of the test
 *              passed   - Test result
 * Returns:     None
 * Notes:       Internal helper function.
 *************************************************/
static void printTestResult(
    const char* testName,
    bool passed)
{
    Serial.printf(
        "[%-4s] %s\n",
        passed ? "PASS" : "FAIL",
        testName
    );
}

/*************************************************
 * Function:    printStorageInformation
 * Description: Reads and prints the SD card
 *              capacity information.
 * Parameters:  None
 * Returns:     true  - Values are plausible
 *              false - Invalid capacity detected
 * Notes:       Tests all SD capacity functions.
 *************************************************/
static bool printStorageInformation()
{
    const uint64_t totalBytes =
        getSdTotalBytes();

    const uint64_t usedBytes =
        getSdUsedBytes();

    const uint64_t freeBytes =
        getSdFreeBytes();

    const uint64_t bytesPerMegabyte =
        1024ULL * 1024ULL;

    Serial.println();
    Serial.println("SD card storage information:");

    Serial.printf(
        "  Total: %llu bytes (%llu MB)\n",
        static_cast<unsigned long long>(totalBytes),
        static_cast<unsigned long long>(
            totalBytes / bytesPerMegabyte)
    );

    Serial.printf(
        "  Used : %llu bytes (%llu MB)\n",
        static_cast<unsigned long long>(usedBytes),
        static_cast<unsigned long long>(
            usedBytes / bytesPerMegabyte)
    );

    Serial.printf(
        "  Free : %llu bytes (%llu MB)\n",
        static_cast<unsigned long long>(freeBytes),
        static_cast<unsigned long long>(
            freeBytes / bytesPerMegabyte)
    );

    return totalBytes > 0 &&
           usedBytes <= totalBytes &&
           freeBytes <= totalBytes;
}

/*************************************************
 * Function:    testDirectoryCreation
 * Description: Tests creation and reopening of the
 *              development test directory.
 * Parameters:  None
 * Returns:     true  - Directory test successful
 *              false - Directory test failed
 * Notes:       Calls createSdDirectory() twice to
 *              test new and existing directories.
 *************************************************/
static bool testDirectoryCreation()
{
    const bool creationSuccessful =
        createSdDirectory(TEST_DIRECTORY);

    printTestResult(
        "Create test directory",
        creationSuccessful
    );

    if (!creationSuccessful)
    {
        return false;
    }

    const bool existingDirectorySuccessful =
        createSdDirectory(TEST_DIRECTORY);

    printTestResult(
        "Access existing test directory",
        existingDirectorySuccessful
    );

    return existingDirectorySuccessful;
}

/*************************************************
 * Function:    testFileWrite
 * Description: Creates a test file and writes
 *              predefined test content.
 * Parameters:  None
 * Returns:     true  - File written successfully
 *              false - File write failed
 * Notes:       Removes an old test file before
 *              creating a new one.
 *************************************************/
static bool testFileWrite()
{
    if (SD.exists(TEST_FILE_PATH))
    {
        if (!SD.remove(TEST_FILE_PATH))
        {
            Serial.println(
                "Failed to remove previous test file"
            );

            return false;
        }
    }

    File file = SD.open(
        TEST_FILE_PATH,
        FILE_WRITE
    );

    if (!file)
    {
        return false;
    }

    const size_t expectedBytes =
        strlen(TEST_FILE_CONTENT);

    const size_t writtenBytes =
        file.print(TEST_FILE_CONTENT);

    file.flush();
    file.close();

    Serial.printf(
        "Written bytes: %u/%u\n",
        static_cast<unsigned>(writtenBytes),
        static_cast<unsigned>(expectedBytes)
    );

    return writtenBytes == expectedBytes &&
           SD.exists(TEST_FILE_PATH);
}

/*************************************************
 * Function:    testFileRead
 * Description: Reads the test file and validates
 *              its content.
 * Parameters:  None
 * Returns:     true  - File content is valid
 *              false - File read or validation
 *                      failed
 * Notes:       Tests direct SD card file access.
 *************************************************/
static bool testFileRead()
{
    File file = SD.open(
        TEST_FILE_PATH,
        FILE_READ
    );

    if (!file)
    {
        return false;
    }

    String fileContent;

    while (file.available())
    {
        fileContent +=
            static_cast<char>(file.read());
    }

    file.close();

    Serial.printf(
        "Read content: \"%s\"\n",
        fileContent.c_str()
    );

    return fileContent == TEST_FILE_CONTENT;
}

/*************************************************
 * Function:    testFileDeletion
 * Description: Deletes the temporary test file.
 * Parameters:  None
 * Returns:     true  - File deleted successfully
 *              false - File deletion failed
 * Notes:       The test directory is retained.
 *************************************************/
static bool testFileDeletion()
{
    if (!SD.exists(TEST_FILE_PATH))
    {
        return false;
    }

    if (!SD.remove(TEST_FILE_PATH))
    {
        return false;
    }

    return !SD.exists(TEST_FILE_PATH);
}

/*************************************************
 * Function:    runSdManagerTests
 * Description: Runs the complete SD manager
 *              development test sequence.
 * Parameters:  None
 * Returns:     None
 * Notes:       The sequence is executed once.
 *************************************************/
static void runSdManagerTests()
{
    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("DEV-011 SD Manager Test");
    Serial.println("--------------------------------");

    const bool initializationSuccessful =
        initSdCard();

    printTestResult(
        "SD card initialization",
        initializationSuccessful
    );

    const bool cardAvailable =
        isSdCardAvailable();

    printTestResult(
        "SD card availability",
        cardAvailable
    );

    if (!initializationSuccessful ||
        !cardAvailable)
    {
        Serial.println();
        Serial.println(
            "Test aborted: SD card unavailable"
        );

        Serial.println("--------------------------------");
        return;
    }

    const bool capacityValid =
        printStorageInformation();

    printTestResult(
        "SD card capacity values",
        capacityValid
    );

    Serial.println();

    const bool directoryTestSuccessful =
        testDirectoryCreation();

    if (!directoryTestSuccessful)
    {
        Serial.println();
        Serial.println(
            "Test aborted: directory unavailable"
        );

        Serial.println("--------------------------------");
        return;
    }

    const bool writeSuccessful =
        testFileWrite();

    printTestResult(
        "Create and write test file",
        writeSuccessful
    );

    bool readSuccessful = false;

    if (writeSuccessful)
    {
        readSuccessful =
            testFileRead();
    }

    printTestResult(
        "Read and validate test file",
        readSuccessful
    );

    bool deletionSuccessful = false;

    if (writeSuccessful)
    {
        deletionSuccessful =
            testFileDeletion();
    }

    printTestResult(
        "Delete test file",
        deletionSuccessful
    );

    const bool completeTestSuccessful =
        initializationSuccessful &&
        cardAvailable &&
        capacityValid &&
        directoryTestSuccessful &&
        writeSuccessful &&
        readSuccessful &&
        deletionSuccessful;

    Serial.println();
    Serial.println("--------------------------------");

    printTestResult(
        "DEV-011 complete test",
        completeTestSuccessful
    );

    Serial.println("--------------------------------");
}

/*************************************************
 * Function:    setupDevSdManager
 * Description: Initializes the DEV-011 SD manager
 *              test program.
 * Parameters:  None
 * Returns:     None
 * Notes:       Call this function from setup().
 *************************************************/
void setupDevSdManager()
{
    Serial.begin(115200);
    delay(1000);

    testCompleted = false;
}

/*************************************************
 * Function:    loopDevSdManager
 * Description: Executes the DEV-011 SD manager
 *              test sequence.
 * Parameters:  None
 * Returns:     None
 * Notes:       Call this function from loop().
 *              The test is executed only once.
 *************************************************/
void loopDevSdManager()
{
    if (testCompleted)
    {
        return;
    }

    testCompleted = true;

    runSdManagerTests();
}
