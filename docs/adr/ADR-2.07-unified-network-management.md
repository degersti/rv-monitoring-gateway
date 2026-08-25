# ADR-2.07 -- Unified Network Management

## Status

Accepted

## Context

The gateway supports both WiFi and cellular connectivity. The
corresponding connections are handled independently by the
`wifi_manager` and `cellular_manager`.

Higher-level services such as MQTT should not need to know which network
type is currently being used. They require a network connection and a
corresponding network client regardless of whether the connection is
provided through WiFi or the mobile network.

The gateway also requires a defined strategy for selecting between the
available network types and for handling connection failures.

The preferred network and the use of an alternative network shall be
configurable so that the network behavior can be adapted to different
operating scenarios.

In addition to the initial connection establishment, an already
established connection may be lost while the gateway remains awake. The
network management therefore has to distinguish between maintaining an
active connection, attempting to restore it, switching to a fallback
network, and reporting that the complete connection cycle has failed.

The decision what to do after a complete network connection failure is
outside the responsibility of the network management layer. Depending on
the current operating mode, the higher-level application logic may for
example start a new connection cycle, continue operating offline, buffer
data, or enter a sleep state.

## Decision

A dedicated `network_manager` shall provide a common interface between
higher-level services and the available network types.

The `network_manager` shall manage the `wifi_manager` and
`cellular_manager` as interchangeable network backends. Higher-level
services shall access network connectivity only through the
`network_manager` and shall not depend on the currently active network
type.

The network selection strategy shall be based on two independent
configuration options:

-   **Priority Network** defines which network type shall be used first.
-   **Fallback Enabled** defines whether the alternative network may be
    used if the initially selected network cannot provide or maintain a
    connection.

Both WiFi and cellular connectivity can be selected as the priority
network.

### Network Initialization

When a new network connection cycle is started, only the configured
priority network shall be initialized.

The alternative network shall not be initialized unless it is actually
required as a fallback.

The network connection cycle therefore starts with:

``` text
initNetwork(priorityNetwork, fallbackEnabled)
                │
                ▼
        Initialize Priority
                │
                ▼
          Connection Attempt
```

This avoids unnecessary initialization and power consumption of network
hardware that is not currently required.

### Connection State Handling

The individual network backends shall handle the technical connection
process of their respective interface.

The `wifi_manager` is responsible for WiFi-specific connection handling.

The `cellular_manager` is responsible for mobile network registration,
PPP establishment, and cellular-specific connection handling.

Both managers shall expose their connection state through the common
`NetworkConnectionState`:

``` cpp
enum class NetworkConnectionState
{
    IDLE,
    CONNECTING,
    CONNECTED,
    FAILED
};
```

The `network_manager` shall not interpret WiFi-, modem-, PPP-, or
protocol-specific failure conditions. It reacts only to the common
network connection state reported by the active backend.

### Connection Cycle

A network connection cycle consists of one attempt using the selected
primary network and, if enabled and required, one attempt using the
fallback network.

The internal connection cycle follows the phases:

``` text
PRIMARY
   │
   ├── CONNECTED
   │
   └── FAILED
          │
          ├── Fallback disabled
          │        │
          │        ▼
          │      FAILED
          │
          └── Fallback enabled
                   │
                   ▼
                FALLBACK
                   │
                   ├── CONNECTED
                   │
                   └── FAILED
                          │
                          ▼
                        FAILED
```

Once the complete connection cycle reaches the final `FAILED` state, no
further network backend shall automatically be started by the
`network_manager`.

A new connection cycle must be explicitly initiated from a higher-level
component.

### Connection Maintenance

While a network connection is established, `processNetworkConnection()`
shall continue to be called periodically.

The active network backend is responsible for detecting whether its
connection is still available.

As long as the active backend reports:

``` text
CONNECTED
```

the current network connection remains active and no network switching
is performed.

If the connection is interrupted, the corresponding network backend may
attempt to restore its connection according to its internal connection
handling.

If the backend eventually reports:

``` text
FAILED
```

the `network_manager` shall apply the configured fallback policy.

### Fallback Behavior

If fallback is disabled, a failed primary network immediately terminates
the current network connection cycle:

``` text
PRIMARY
   │
   ▼
FAILED
```

If fallback is enabled, the currently active primary network shall first
be disconnected before the alternative network is initialized.

The fallback sequence shall be:

``` text
Priority Network
       │
       ▼
     FAILED
       │
       ▼
Disconnect Priority
       │
       ▼
Initialize Fallback
       │
       ▼
Fallback Connection Attempt
```

If the fallback network establishes a connection successfully, it
becomes the active network for the remainder of the current connection
cycle.

If the fallback network also reports `FAILED`, the entire network
connection cycle is considered failed.

The `network_manager` shall not automatically switch back and forth
between the available network types.

In particular, the following behavior shall be avoided:

``` text
WiFi → Cellular → WiFi → Cellular → ...
```

### Connection Loss While Using Fallback

If the fallback network has successfully established a connection and
that connection is later lost, the fallback backend may attempt to
restore its own connection.

If this attempt ultimately fails, the entire network connection cycle
shall transition to `FAILED`.

The `network_manager` shall not automatically return to the configured
priority network within the same connection cycle.

A new attempt using the priority network requires a new externally
initiated connection cycle.

### Higher-Level Failure Handling

A final `FAILED` state represents the result of the complete current
network connection cycle.

The `network_manager` shall not decide what happens after this point.

Higher-level application logic shall evaluate the returned network state
and decide how the system should continue.

Possible reactions include:

``` text
NetworkConnectionState::FAILED
             │
             ├── Start a new network connection cycle
             ├── Continue operating offline
             ├── Buffer unsent data
             ├── Report a fault
             └── Enter sleep mode
```

This separation is especially relevant for the gateway's sleep-based
operating model.

After a new wake cycle, the application may initiate a completely new
network connection cycle using the configured priority and fallback
settings.

If the gateway remains awake for an extended period, the existing
connection shall instead remain active and be continuously monitored.

### Active and Priority Network

The configured priority network and the currently active network shall
be treated as separate concepts.

For example:

``` text
priorityNetwork = WIFI
activeNetwork   = CELLULAR
```

is valid and indicates that WiFi was preferred but the current
connection is operating through the cellular fallback.

The `network_manager` shall therefore provide access to both the
configured priority network and the currently active network.

It shall also provide information about whether the current connection
is operating through the fallback path.

### Manual Network Selection

The `network_manager` may provide an explicit network switching function
for development, diagnostics, or higher-level application control.

A manually requested network change shall disconnect the currently
active interface before initializing the newly selected network.

Manual switching shall remain separate from the automatic fallback
policy.

### Network Client Abstraction

Higher-level protocol layers shall obtain their network client
exclusively through the `network_manager`.

The `network_manager` shall return the client belonging to the currently
active network backend:

``` text
                       network_manager
                             │
                      getNetworkClient()
                             │
                ┌────────────┴────────────┐
                ▼                         ▼
         WiFi network client      Cellular network client
```

This allows higher-level services such as MQTT to remain independent of
the selected network transport.

The resulting architecture is:

``` text
                 Application Services
                         │
                         ▼
                  network_manager
                    /         \
                   /           \
                  ▼             ▼
           wifi_manager   cellular_manager
                  │             │
                  ▼             ▼
                WiFi           PPP
                  │             │
                  ▼             ▼
            WiFi Network   Mobile Network
```

## Consequences

Higher-level services are independent of the underlying network
technology.

WiFi and cellular connectivity can be selected using the same network
interface.

Only the network hardware that is currently required is initialized.

The configured priority network can be changed without modifying
higher-level services.

Fallback behavior is deterministic and limited to one alternative
network attempt per connection cycle.

A complete network failure results in a stable `FAILED` state rather
than continuous automatic switching between network interfaces.

The responsibility for retrying an entire connection cycle, entering
sleep mode, buffering data, or applying other system-level behavior
remains with the higher-level application logic.

The architecture therefore separates:

``` text
Network backend
    │
    └── Establish and maintain one network connection

network_manager
    │
    └── Select Priority / Fallback and manage one connection cycle

Application / Runtime
    │
    └── Decide what happens after CONNECTED or FAILED
```
