#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "config.h"
#include "app/sd_manager.h"
#include "app/debug_logger.h"
#include "app/fault_manager.h"

static SPIClass sdSpi(FSPI);
static bool sdCardAvailable = false;

/*************************************************
 * Function:    initSdCard
 * Description: Initializes the SD card and makes
 *              it available to the application.
 * Parameters:  None
 * Returns:     true  - Initialization successful
 *              false - Initialization failed
 * Notes:       Sets SD_INIT_FAILED if the SD card
 *              cannot be initialized or detected.
 *************************************************/
bool initSdCard()
{
    LOG_INFO("Initializing SD card...");

    sdCardAvailable = false;

    sdSpi.begin(
        PIN_SD_SCK,
        PIN_SD_MISO,
        PIN_SD_MOSI,
        PIN_SD_CS
    );

    if (!SD.begin(PIN_SD_CS, sdSpi))
    {
        LOG_ERROR("SD card initialization failed");

        setFault(FaultCode::SD_INIT_FAILED);
        return false;
    }

    const uint8_t cardType = SD.cardType();

    if (cardType == CARD_NONE)
    {
        LOG_ERROR("No SD card detected");

        SD.end();

        setFault(FaultCode::SD_INIT_FAILED);
        return false;
    }

    sdCardAvailable = true;

    clearFault(FaultCode::SD_INIT_FAILED);

    LOG_INFO(
        "SD card initialized: total=%llu MB, used=%llu MB",
        static_cast<unsigned long long>(
            getSdTotalBytes() / (1024ULL * 1024ULL)),
        static_cast<unsigned long long>(
            getSdUsedBytes() / (1024ULL * 1024ULL))
    );

    return true;
}

/*************************************************
 * Function:    isSdCardAvailable
 * Description: Checks whether the SD card is
 *              currently available.
 * Parameters:  None
 * Returns:     true  - SD card available
 *              false - SD card unavailable
 * Notes:       Returns the internal availability
 *              state set during initialization.
 *************************************************/
bool isSdCardAvailable()
{
    return sdCardAvailable;
}

/*************************************************
 * Function:    createSdDirectory
 * Description: Creates a directory on the SD
 *              card if it does not already exist.
 * Parameters:  path - Absolute directory path
 * Returns:     true  - Directory available
 *              false - Directory creation failed
 * Notes:       Sets SD_READ_FAILED if an existing
 *              path cannot be accessed and
 *              SD_WRITE_FAILED if the directory
 *              cannot be created.
 *************************************************/
bool createSdDirectory(const char* path)
{
    if (!sdCardAvailable)
    {
        LOG_ERROR(
            "Cannot create SD directory: SD card unavailable"
        );

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (path == nullptr)
    {
        LOG_ERROR(
            "Cannot create SD directory: path is null"
        );

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    if (SD.exists(path))
    {
        File directory = SD.open(path, FILE_READ);

        if (!directory)
        {
            LOG_ERROR(
                "Failed to open SD directory: %s",
                path
            );

            setFault(FaultCode::SD_READ_FAILED);
            return false;
        }

        const bool isDirectory =
            directory.isDirectory();

        directory.close();

        if (!isDirectory)
        {
            LOG_ERROR(
                "SD path is not a directory: %s",
                path
            );

            setFault(FaultCode::SD_READ_FAILED);
            return false;
        }

        clearFault(FaultCode::SD_READ_FAILED);

        return true;
    }

    if (!SD.mkdir(path))
    {
        LOG_ERROR(
            "Failed to create SD directory: %s",
            path
        );

        setFault(FaultCode::SD_WRITE_FAILED);
        return false;
    }

    clearFault(FaultCode::SD_WRITE_FAILED);

    LOG_DEBUG(
        "SD directory created: %s",
        path
    );

    return true;
}

/*************************************************
 * Function:    getSdTotalBytes
 * Description: Returns the total capacity of the
 *              SD card.
 * Parameters:  None
 * Returns:     Total capacity in bytes.
 * Notes:       Returns zero if the SD card is not
 *              available.
 *************************************************/
uint64_t getSdTotalBytes()
{
    if (!sdCardAvailable)
    {
        return 0;
    }

    return SD.totalBytes();
}

/*************************************************
 * Function:    getSdUsedBytes
 * Description: Returns the currently used storage
 *              space on the SD card.
 * Parameters:  None
 * Returns:     Used capacity in bytes.
 * Notes:       Returns zero if the SD card is not
 *              available.
 *************************************************/
uint64_t getSdUsedBytes()
{
    if (!sdCardAvailable)
    {
        return 0;
    }

    return SD.usedBytes();
}

/*************************************************
 * Function:    getSdFreeBytes
 * Description: Returns the currently available
 *              storage space on the SD card.
 * Parameters:  None
 * Returns:     Free capacity in bytes.
 * Notes:       Returns zero if the SD card is not
 *              available or no free space remains.
 *************************************************/
uint64_t getSdFreeBytes()
{
    const uint64_t totalBytes =
        getSdTotalBytes();

    const uint64_t usedBytes =
        getSdUsedBytes();

    if (usedBytes >= totalBytes)
    {
        return 0;
    }

    return totalBytes - usedBytes;
}