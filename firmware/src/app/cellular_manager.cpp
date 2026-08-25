/*************************************************
 * File:        cellular_manager.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Handles cellular network connectivity using
 * PPP and provides access to the network client.
 *
 * Responsibilities:
 * - LTE modem power control
 * - Non-blocking cellular connection management
 * - Mobile network registration
 * - PPP data connection establishment
 * - Connection status monitoring
 * - Network client provisioning
 *
 *************************************************/

#include <Arduino.h>
#include <Network.h>
#include <NetworkClientSecure.h>
#include <PPP.h>
#include <atomic>

#include "config.h"
#include "app/cellular_manager.h"
#include "app/debug_logger.h"

// TCP client used by higher protocol layers
static NetworkClientSecure cellularClient;

// Public cellular connection state
static NetworkConnectionState cellularState =NetworkConnectionState::IDLE;

// Internal cellular connection phases
enum class CellularConnectionPhase
{
    IDLE,
    POWER_UP,
    START_MODEM,
    WAIT_FOR_MODEM_START,
    WAIT_FOR_NETWORK,
    START_DATA_MODE,
    WAIT_FOR_IP
};

static CellularConnectionPhase connectionPhase = CellularConnectionPhase::IDLE;

// Cellular connection timing and retry state
static uint32_t phaseStartTime = 0;
static uint32_t lastRetryTime = 0;
static uint32_t cellularConnectionAttempt = 0;

// Internal modem start state
enum class ModemStartState
{
    IDLE,
    RUNNING,
    SUCCESS,
    FAILED
};

// Current state of the asynchronous modem initialization
static std::atomic<ModemStartState> modemStartState{ModemStartState::IDLE};
// Signals the modem start task to clean up after initialization
static std::atomic<bool> modemStartCancelRequested{false};
// Handle of the currently running modem start task
static TaskHandle_t modemStartTaskHandle = nullptr;
// Modem start task configuration
static constexpr uint32_t MODEM_START_TASK_STACK_SIZE = 8192;
static constexpr UBaseType_t MODEM_START_TASK_PRIORITY = 1;

/*************************************************
 * Function:    modemStartTask
 * Description: Initializes the PPP modem outside
 *              the main application task.
 * Parameters:  parameter - Unused task parameter
 * Returns:     None
 * Notes:       PPP.begin() may block for several
 *              seconds while communicating with
 *              the modem. Running it in a separate
 *              task keeps the main loop responsive.
 *************************************************/
static void modemStartTask(void* parameter)
{
    const bool success = PPP.begin(
        PPP_MODEM_SIM7000,
        1,
        LTE_BAUDRATE);

    // A disconnect may have been requested while
    // PPP.begin() was still running.
    if (modemStartCancelRequested)
    {
        if (success)
        {
            PPP.end();
        }

        digitalWrite(LTE_POWER_PIN, LOW);

        modemStartState =
            ModemStartState::IDLE;

        modemStartCancelRequested = false;
        modemStartTaskHandle = nullptr;

        vTaskDelete(nullptr);
        return;
    }

    modemStartState =
        success
            ? ModemStartState::SUCCESS
            : ModemStartState::FAILED;

    modemStartTaskHandle = nullptr;

    vTaskDelete(nullptr);
}
/*************************************************
 * Function:    startModemTask
 * Description: Starts asynchronous PPP modem
 *              initialization.
 * Parameters:  None
 * Returns:     true  - Task successfully created
 *              false - Task creation failed
 *************************************************/
static bool startModemTask(void)
{
    if (modemStartState ==
        ModemStartState::RUNNING)
    {
        return true;
    }

    modemStartCancelRequested = false;

    modemStartState =
        ModemStartState::RUNNING;

    const BaseType_t result = xTaskCreate(
        modemStartTask,
        "modem_start",
        MODEM_START_TASK_STACK_SIZE,
        nullptr,
        MODEM_START_TASK_PRIORITY,
        &modemStartTaskHandle);

    if (result != pdPASS)
    {
        modemStartState =
            ModemStartState::IDLE;

        modemStartTaskHandle = nullptr;

        return false;
    }

    return true;
}
/*************************************************
 * Function:    startCellularConnection
 * Description: Starts a new cellular connection
 *              attempt.
 * Parameters:  None
 * Returns:     None
 * Notes:       Powers the LTE modem and starts the
 *              internal connection state machine.
 *************************************************/
static void startCellularConnection(void)
{
    cellularConnectionAttempt++;

    if (cellularConnectionAttempt == 1)
    {
        LOG_INFO(
            "Cellular status: CONNECTING [APN=%s]",
            LTE_APN);
    }
    else
    {
        LOG_INFO(
            "Cellular status: CONNECTING [attempt=%lu]",
            static_cast<unsigned long>(
                cellularConnectionAttempt));
    }

    digitalWrite(LTE_POWER_PIN, HIGH);

    phaseStartTime = millis();

    connectionPhase =
        CellularConnectionPhase::POWER_UP;

    cellularState =
        NetworkConnectionState::CONNECTING;
}


/*************************************************
 * Function:    failCellularConnection
 * Description: Marks the current cellular
 *              connection attempt as failed.
 * Parameters:  reason - Failure description
 * Returns:     None
 * Notes:       Schedules a new connection attempt
 *              after the retry interval.
 *************************************************/
static void failCellularConnection(
    const char* reason)
{
    LOG_WARN(
        "Cellular status: CONNECTION_FAILED [reason=%s, attempt=%lu]",
        reason,
        static_cast<unsigned long>(
            cellularConnectionAttempt));

    PPP.end();

    digitalWrite(LTE_POWER_PIN, LOW);

    connectionPhase =
        CellularConnectionPhase::IDLE;

    cellularState =
        NetworkConnectionState::FAILED;

    lastRetryTime = millis();
}


/*************************************************
 * Function:    initCellular
 * Description: Initializes cellular connection
 *              handling.
 * Parameters:  None
 * Returns:     None
 * Notes:       Does not block and does not start a
 *              connection attempt by itself.
 *************************************************/
void initCellular(void)
{
    LOG_INFO("Initializing cellular network");

    pinMode(LTE_POWER_PIN, OUTPUT);
    digitalWrite(LTE_POWER_PIN, LOW);

    cellularClient.setInsecure();
    PPP.setApn(LTE_APN);

    PPP.setPins(
        LTE_TX_PIN,
        LTE_RX_PIN);

    cellularState =
        NetworkConnectionState::IDLE;

    connectionPhase =
        CellularConnectionPhase::IDLE;

    phaseStartTime = 0;
    lastRetryTime = 0;
    cellularConnectionAttempt = 0;

    modemStartState = ModemStartState::IDLE;
    modemStartCancelRequested = false;
    modemStartTaskHandle = nullptr;
}


/*************************************************
 * Function:    processCellularConnection
 * Description: Handles cellular connection
 *              progress without blocking the
 *              main loop.
 * Parameters:  None
 * Returns:     Current cellular connection state
 * Notes:       Must be called repeatedly while the
 *              application is trying to connect.
 *************************************************/
NetworkConnectionState processCellularConnection(void)
{

    // Wait until a cancelled modem initialization
    // has completely finished.
    if (modemStartCancelRequested ||
        ((modemStartState == ModemStartState::RUNNING) &&
        (connectionPhase == CellularConnectionPhase::IDLE)))
    {
        return cellularState;
    }

    const uint32_t now = millis();

    // Connection established
    if (PPP.hasIP())
    {
        if (cellularState != NetworkConnectionState::CONNECTED)
        {
            LOG_INFO(
                "Cellular status: CONNECTED [IP=%s]",
                PPP.localIP().toString().c_str());

            LOG_DEBUG(
                "Cellular network details: gateway=%s, DNS=%s",
                PPP.gatewayIP().toString().c_str(),
                PPP.dnsIP().toString().c_str());
        }

        cellularState =
            NetworkConnectionState::CONNECTED;

        return cellularState;
    }

    // Connection lost after previously being connected
    if (cellularState ==
        NetworkConnectionState::CONNECTED)
    {
        LOG_WARN("Cellular connection lost");

        PPP.end();

        digitalWrite(LTE_POWER_PIN, LOW);

        connectionPhase =
            CellularConnectionPhase::IDLE;

        cellularState =
            NetworkConnectionState::IDLE;

        return cellularState;
    }

    // Start a new connection attempt
    if (cellularState ==
        NetworkConnectionState::IDLE)
    {
        startCellularConnection();

        return cellularState;
    }

    // Retry after configured retry interval
    if (cellularState ==
        NetworkConnectionState::FAILED)
    {
        if (now - lastRetryTime >=
            LTE_RETRY_INTERVAL_MS)
        {
            startCellularConnection();
        }

        return cellularState;
    }


    switch (connectionPhase)
    {
        // -------------------------------------------------
        // Wait for modem power-up
        // -------------------------------------------------

        case CellularConnectionPhase::POWER_UP:
        {
            if (now - phaseStartTime >=
                LTE_POWER_UP_DELAY_MS)
            {
                connectionPhase =
                    CellularConnectionPhase::START_MODEM;
            }

            break;
        }


        // -------------------------------------------------
        // Initialize PPP modem
        // -------------------------------------------------

        case CellularConnectionPhase::START_MODEM:
        {
            LOG_DEBUG("Starting cellular modem");

            if (!startModemTask())
            {
                failCellularConnection(
                    "modem start task failed");

                return cellularState;
            }

            connectionPhase =
                CellularConnectionPhase::WAIT_FOR_MODEM_START;

            break;
        }


        // -------------------------------------------------
        // Wait for mobile network registration
        // -------------------------------------------------

        case CellularConnectionPhase::WAIT_FOR_NETWORK:
        {
            if (PPP.attached())
            {
                LOG_INFO(
                    "Cellular network attached [operator=%s, RSSI=%d]",
                    PPP.operatorName().c_str(),
                    PPP.RSSI());

                connectionPhase =
                    CellularConnectionPhase::START_DATA_MODE;

                break;
            }

            if (now - phaseStartTime >=
                LTE_ATTACH_TIMEOUT_MS)
            {
                failCellularConnection(
                    "network attach timeout");

                return cellularState;
            }

            break;
        }


        // -------------------------------------------------
        // Switch modem into PPP data mode
        // -------------------------------------------------

        case CellularConnectionPhase::START_DATA_MODE:
        {
            LOG_DEBUG(
                "Starting PPP data connection");

            if (!PPP.mode(
                    ESP_MODEM_MODE_DATA))
            {
                failCellularConnection(
                    "PPP data mode failed");

                return cellularState;
            }

            phaseStartTime = now;

            connectionPhase =
                CellularConnectionPhase::WAIT_FOR_IP;

            break;
        }

        // -------------------------------------------------
        // Wait for PPP modem initialization
        // -------------------------------------------------

        case CellularConnectionPhase::WAIT_FOR_MODEM_START:
        {
            if (modemStartState ==
                ModemStartState::RUNNING)
            {
                break;
            }

            if (modemStartState ==
                ModemStartState::FAILED)
            {
                modemStartState =
                    ModemStartState::IDLE;

                failCellularConnection(
                    "PPP begin failed");

                return cellularState;
            }

            if (modemStartState ==
                ModemStartState::SUCCESS)
            {
                modemStartState =
                    ModemStartState::IDLE;

                LOG_DEBUG("Cellular modem started");

                phaseStartTime = millis();

                connectionPhase =
                    CellularConnectionPhase::WAIT_FOR_NETWORK;
            }

            break;
        }

        // -------------------------------------------------
        // Wait for PPP IP address
        // -------------------------------------------------

        case CellularConnectionPhase::WAIT_FOR_IP:
        {
            if (PPP.hasIP())
            {
                // Connection will be handled at the
                // beginning of the next process call.
                break;
            }

            if (now - phaseStartTime >=
                LTE_IP_TIMEOUT_MS)
            {
                failCellularConnection(
                    "PPP IP timeout");

                return cellularState;
            }

            break;
        }


        case CellularConnectionPhase::IDLE:
            break;
    }

    return cellularState;
}


/*************************************************
 * Function:    disconnectCellular
 * Description: Disconnects the cellular network
 *              and powers down the LTE modem.
 * Parameters:  None
 * Returns:     None
 * Notes:       Resets the connection state.
 *************************************************/
void disconnectCellular(void)
{
    if (modemStartState ==
        ModemStartState::RUNNING)
    {
        modemStartCancelRequested = true;
    }
    else
    {
        PPP.end();

        digitalWrite(LTE_POWER_PIN, LOW);
    }

    cellularState =
        NetworkConnectionState::IDLE;

    connectionPhase =
        CellularConnectionPhase::IDLE;

    phaseStartTime = 0;
    lastRetryTime = 0;
    cellularConnectionAttempt = 0;

    LOG_DEBUG("Cellular network disconnected");
}


/*************************************************
 * Function:    getNetworkConnectionState
 * Description: Returns whether the cellular
 *              network is connected.
 * Parameters:  None
 * Returns:     true  - Cellular network connected
 *              false - Cellular network not
 *                      connected
 * Notes:       None
 *************************************************/
bool isCellularConnected(void)
{
    return PPP.hasIP();
}


/*************************************************
 * Function:    getCellularClient
 * Description: Provides access to the cellular
 *              network client.
 * Parameters:  None
 * Returns:     Reference to network client
 * Notes:       Used by higher protocol layers
 *              such as MQTT.
 *************************************************/
Client& getCellularClient(void)
{
    return cellularClient;
}