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
 * - Priority network selection
 * - Optional fallback handling
 * - Network connection management
 * - Network connection status monitoring
 * - Network client provisioning
 *
 *************************************************/

#include <Arduino.h>

#include "app/network_manager.h"
#include "app/wifi_manager.h"
#include "app/cellular_manager.h"
#include "app/debug_logger.h"


// Preferred network interface
static NetworkType priorityNetwork =
    NetworkType::WIFI;

// Currently active network interface
static NetworkType activeNetwork =
    NetworkType::WIFI;

// Current network phase
static NetworkConnectionPhase networkPhase =
    NetworkConnectionPhase::PRIORITY;

// Enables automatic fallback to the secondary
// network if the priority network fails
static bool fallbackEnabled = false;


/*************************************************
 * Function:    initNetworkInterface
 * Description: Initializes the specified network
 *              interface.
 * Parameters:  network - Network interface to
 *                        initialize
 * Returns:     None
 * Notes:       Only the selected interface is
 *              initialized. Other interfaces
 *              remain untouched until required.
 *************************************************/
static void initNetworkInterface(
    NetworkType network)
{
    switch (network)
    {
        case NetworkType::WIFI:
            initWifi();
            break;

        case NetworkType::CELLULAR:
            initCellular();
            break;
    }
}


/*************************************************
 * Function:    processNetworkInterface
 * Description: Processes the connection state of
 *              the specified network interface.
 * Parameters:  network - Network interface to
 *                        process
 * Returns:     Current connection state of the
 *              selected interface
 * Notes:       None
 *************************************************/
static NetworkConnectionState processNetworkInterface(NetworkType network)
{
    switch (network)
    {
        case NetworkType::WIFI:
            return processWifiConnection();

        case NetworkType::CELLULAR:
            return processCellularConnection();
    }

    return NetworkConnectionState::FAILED;
}


/*************************************************
 * Function:    disconnectNetworkInterface
 * Description: Disconnects the specified network
 *              interface.
 * Parameters:  network - Network interface to
 *                        disconnect
 * Returns:     None
 * Notes:       None
 *************************************************/
static void disconnectNetworkInterface( NetworkType network)
{
    switch (network)
    {
        case NetworkType::WIFI:
            disconnectWifi();
            break;

        case NetworkType::CELLULAR:
            disconnectCellular();
            break;
    }
}


/*************************************************
 * Function:    switchNetwork
 * Description: Switches from the currently active
 *              network to another interface.
 * Parameters:  network - Network interface to use
 * Returns:     None
 * Notes:       The previous interface is
 *              disconnected before the new one
 *              is initialized.
 *************************************************/
static void switchNetwork(NetworkType network)
{
    if (network == activeNetwork)
    {
        return;
    }

    disconnectNetworkInterface(activeNetwork);

    activeNetwork = network;

    initNetworkInterface(activeNetwork);
}


/*************************************************
 * Function:    getFallbackNetwork
 * Description: Returns the secondary network
 *              relative to the configured priority
 *              network.
 * Parameters:  None
 * Returns:     Fallback network type
 * Notes:       WiFi and Cellular are currently the
 *              only supported network interfaces.
 *************************************************/
static NetworkType getFallbackNetwork(void)
{
    return priorityNetwork == NetworkType::WIFI
        ? NetworkType::CELLULAR
        : NetworkType::WIFI;
}


/*************************************************
 * Function:    initNetwork
 * Description: Initializes network management
 *              using the specified priority
 *              network and fallback setting.
 * Parameters:  priority       - Preferred network
 *              enableFallback - Enables automatic
 *                               fallback
 * Returns:     None
 * Notes:       Only the priority network is
 *              initialized initially. The fallback
 *              interface is initialized only when
 *              required.
 *************************************************/
void initNetwork(NetworkType priority, bool enableFallback)
{
    LOG_INFO("Initializing network manager");

    priorityNetwork = priority;
    activeNetwork = priority;

    fallbackEnabled = enableFallback;
    
    networkPhase = NetworkConnectionPhase::PRIORITY;

    initNetworkInterface(activeNetwork);
}


/*************************************************
 * Function:    processNetworkConnection
 * Description: Processes the currently active
 *              network and performs an automatic
 *              fallback if enabled and required.
 * Parameters:  None
 * Returns:     Current network connection state
 * Notes:       Must be called repeatedly while the
 *              application is trying to connect.
 *
 *              Automatic fallback is performed only
 *              once. If the fallback network also
 *              fails, FAILED is returned.
 *************************************************/
NetworkConnectionState processNetworkConnection(void)
{
    // Complete connection cycle already failed
    if (networkPhase == NetworkConnectionPhase::FAILED)
    {
        return NetworkConnectionState::FAILED;
    }

    NetworkConnectionState state =
        processNetworkInterface(activeNetwork);

    if (state != NetworkConnectionState::FAILED)
    {
        return state;
    }

    // Priority network failed
    if (networkPhase == NetworkConnectionPhase::PRIORITY)
    {
        if (!fallbackEnabled)
        {
            networkPhase =
                NetworkConnectionPhase::FAILED;

            return NetworkConnectionState::FAILED;
        }

        switchNetwork(getFallbackNetwork());

        networkPhase =
            NetworkConnectionPhase::FALLBACK;

        return NetworkConnectionState::CONNECTING;
    }

    // Fallback network failed
    networkPhase =
        NetworkConnectionPhase::FAILED;

    return NetworkConnectionState::FAILED;
}


/*************************************************
 * Function:    disconnectNetwork
 * Description: Disconnects the currently active
 *              network interface.
 * Parameters:  None
 * Returns:     None
 * Notes:       Does not change the configured
 *              priority or fallback settings.
 *************************************************/
void disconnectNetwork(void)
{
    disconnectNetworkInterface(activeNetwork);

    networkPhase = NetworkConnectionPhase::FAILED;
}


/*************************************************
 * Function:    setActiveNetwork
 * Description: Manually switches the active
 *              network interface.
 * Parameters:  network - Network interface to use
 * Returns:     true  - Network changed
 *              false - Requested network already
 *                      active
 * Notes:       Manual network selection clears the
 *              current automatic fallback state.
 *              The configured priority network is
 *              not changed.
 *************************************************/
bool setActiveNetwork(NetworkType network)
{
    if (network == activeNetwork)
    {
        return false;
    }

    switchNetwork(network);

    networkPhase = NetworkConnectionPhase::PRIORITY;

    return true;
}


/*************************************************
 * Function:    getNetworkConnectionState
 * Description: Returns whether the currently active
 *              network interface is connected.
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
            return isWifiConnected();

        case NetworkType::CELLULAR:
            return isCellularConnected();
    }

    return false;
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
            return getCellularClient();
    }

    // Fallback return required by compiler.
    // This path should never be reached because
    // NetworkType currently contains only valid
    // WiFi and Cellular values.
    return getWifiClient();
}


/*************************************************
 * Function:    getActiveNetwork
 * Description: Returns the currently active
 *              network interface.
 * Parameters:  None
 * Returns:     Active network type
 * Notes:       May differ from the priority network
 *              while fallback is active.
 *************************************************/
NetworkType getActiveNetwork(void)
{
    return activeNetwork;
}


/*************************************************
 * Function:    getPriorityNetwork
 * Description: Returns the configured priority
 *              network interface.
 * Parameters:  None
 * Returns:     Priority network type
 * Notes:       None
 *************************************************/
NetworkType getPriorityNetwork(void)
{
    return priorityNetwork;
}


/*************************************************
 * Function:    isFallbackEnabled
 * Description: Returns whether automatic network
 *              fallback is enabled.
 * Parameters:  None
 * Returns:     true  - Fallback enabled
 *              false - Fallback disabled
 * Notes:       None
 *************************************************/
bool isFallbackEnabled(void)
{
    return fallbackEnabled;
}


/*************************************************
 * Function:    isFallbackActive
 * Description: Returns whether the fallback
 *              network is currently active.
 * Parameters:  None
 * Returns:     true  - Fallback network active
 *              false - Priority or manually
 *                      selected network active
 * Notes:       None
 *************************************************/
bool isFallbackActive(void)
{
    return networkPhase ==
        NetworkConnectionPhase::FALLBACK;
}