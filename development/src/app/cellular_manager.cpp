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
#include <PPP.h>

#include "config.h"
#include "app/cellular_manager.h"
#include "app/debug_logger.h"

// TCP client used by higher protocol layers
static NetworkClient cellularClient;


// Public cellular connection state
static NetworkConnectionState cellularState =
    NetworkConnectionState::IDLE;


// Internal connection phases
enum class CellularConnectionPhase
{
    IDLE,
    POWER_UP,
    START_MODEM,
    WAIT_FOR_NETWORK,
    START_DATA_MODE,
    WAIT_FOR_IP
};

static CellularConnectionPhase connectionPhase =
    CellularConnectionPhase::IDLE;


static uint32_t phaseStartTime = 0;
static uint32_t lastRetryTime = 0;
static uint32_t cellularConnectionAttempt = 0;


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
    const uint32_t now = millis();

    // Connection established
    if (PPP.hasIP())
    {
        if (cellularState !=
            NetworkConnectionState::CONNECTED)
        {
            LOG_INFO(
                "Cellular status: CONNECTED [IP=%s]",
                PPP.localIP().toString().c_str());

            LOG_DEBUG(
                "Cellular network details: operator=%s, RSSI=%d, gateway=%s, DNS=%s",
                PPP.operatorName().c_str(),
                PPP.RSSI(),
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

            if (!PPP.begin(
                    PPP_MODEM_SIM7000,
                    1,
                    LTE_BAUDRATE))
            {
                failCellularConnection(
                    "PPP begin failed");

                return cellularState;
            }

            LOG_DEBUG("Cellular modem started");

            phaseStartTime = now;

            connectionPhase =
                CellularConnectionPhase::WAIT_FOR_NETWORK;

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
    PPP.end();

    digitalWrite(LTE_POWER_PIN, LOW);

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