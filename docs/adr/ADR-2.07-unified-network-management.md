# ADR-2.07 – Unified Network Management

## Status

Accepted

## Context

The gateway supports both WiFi and cellular connectivity. The corresponding connections are handled independently by the `wifi_manager` and `cellular_manager`.

Higher-level services such as MQTT should not need to know which network type is currently being used. They require a network connection and a corresponding network client regardless of whether the connection is provided through WiFi or the mobile network.

The gateway also needs a defined strategy for selecting between the available network types. WiFi should normally be preferred when available, while cellular connectivity can provide an alternative when WiFi cannot be used.

The preferred network and the use of the alternative network should be configurable so that the network behavior can be adapted to different operating scenarios.

## Decision


A dedicated `network_manager` shall provide a common interface between higher-level services and the available network types.

The `network_manager` shall manage the `wifi_manager` and `cellular_manager` as interchangeable network backends. Higher-level services shall access network connectivity only through the `network_manager` and shall not depend on the currently active network type.

The network selection strategy shall be based on two independent configuration options:

* **Priority Network** defines which network type should be used first.
* **Fallback** defines whether the alternative network may be used if the priority network cannot provide a connection.

Both WiFi and cellular connectivity can be selected as the priority network.

When fallback is enabled and the priority network is unavailable, the `network_manager` shall attempt to establish a connection using the alternative network.

When fallback is disabled, only the configured priority network shall be used.

The resulting architecture is:

```text
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

Higher-level services therefore use a single network interface regardless of the selected network type.