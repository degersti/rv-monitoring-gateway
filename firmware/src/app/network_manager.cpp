/*************************************************
 * File:        network_manager.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Provides a common interface for network
 * connectivity independent of the underlying
 * transport.
 *
 * Responsibilities:
 * - Network interface abstraction
 * - Network connection management
 * - Network connection status monitoring
 * - Network client provisioning
 *
 *************************************************/

#include <Arduino.h>
#include "app/network_manager.h"
#include "app/wifi_manager.h"
#include "app/debug_logger.h"


static NetworkType activeNetwork = NetworkType::WIFI;


/*************************************************
 * Function:    initNetwork
 * Description: Initializes the currently selected
 *              network interface.
 * Parameters:  None
 * Returns:     None
 * Notes:       Currently only WiFi is supported.
 *************************************************/
void initNetwork(void)
{
    LOG_INFO("Initializing network manager");

    activeNetwork = NetworkType::WIFI;

    initWifi();
}

/*************************************************
 * Function:    disconnectNetwork
 * Description: Disconnects the currently active
 *              network interface.
 * Parameters:  None
 * Returns:     None
 * Notes:       None
 *************************************************/
void disconnectNetwork(void)
{
    switch (activeNetwork)
    {
        case NetworkType::WIFI:
            disconnectWifi();
            break;

        case NetworkType::CELLULAR:
            // CELLULAR support will be added later.
            break;
    }
}


/*************************************************
 * Function:    getNetworkConnectionState
 * Description: Returns whether the currently
 *              selected network is connected.
 * Parameters:  None
 * Returns:     true  - Network connected
 *              false - Network not connected
 * Notes:       None
 *************************************************/
bool getNetworkConnectionState(void)
{
    switch (activeNetwork)
    {
        case NetworkType::WIFI:
            return getWiFiConnectionState();

        case NetworkType::CELLULAR:
            // CELLULAR support will be added later.
            return false;
    }

    return false;
}

/*************************************************
 * Function:    processNetworkConnection
 * Description: Processes the connection state of
 *              the currently selected network.
 * Parameters:  None
 * Returns:     Current network connection state
 * Notes:       Must be called repeatedly while the
 *              application is trying to connect.
 *************************************************/
NetworkConnectionState processNetworkConnection(void)
{
    switch (activeNetwork)
    {
        case NetworkType::WIFI:
        {
            WiFiConnectionState state = processWifiConnection();

            switch (state)
            {
                case WiFiConnectionState::IDLE:
                    return NetworkConnectionState::IDLE;

                case WiFiConnectionState::CONNECTING:
                    return NetworkConnectionState::CONNECTING;

                case WiFiConnectionState::CONNECTED:
                    return NetworkConnectionState::CONNECTED;

                case WiFiConnectionState::FAILED:
                    return NetworkConnectionState::FAILED;
            }

            break;
        }

        case NetworkType::CELLULAR:
            // Later:
            // return convertCellularState(processCellularConnection());
            break;
    }

    return NetworkConnectionState::FAILED;
}

/*************************************************
 * Function:    getNetworkClient
 * Description: Provides access to the client of
 *              the currently active network.
 * Parameters:  None
 * Returns:     Reference to network client
 * Notes:       Used by higher protocol layers
 *              such as MQTT.
 *************************************************/
Client& getNetworkClient(void)
{
    switch (activeNetwork)
    {
        case NetworkType::WIFI:
            return getWifiClient();

        case NetworkType::CELLULAR:
            // CELLULAR support will be added later.
            break;
    }

    // Currently unreachable because WiFi is the
    // default and only supported network.
    return getWifiClient();
}


/*************************************************
 * Function:    getActiveNetwork
 * Description: Returns the currently selected
 *              network interface.
 * Parameters:  None
 * Returns:     Active network type
 * Notes:       None
 *************************************************/
NetworkType getActiveNetwork(void)
{
    return activeNetwork;
}

