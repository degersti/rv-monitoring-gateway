#include <Arduino.h>
#include <Client.h>

enum class NetworkConnectionState
{
    IDLE,
    CONNECTING,
    CONNECTED,
    FAILED
};
enum class NetworkType
{
    WIFI,
    CELLULAR
};

void initNetwork(void);
void disconnectNetwork(void);
bool getNetworkConnectionState(void);
NetworkConnectionState processNetworkConnection(void);
Client& getNetworkClient(void);
NetworkType getActiveNetwork(void);