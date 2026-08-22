/*************************************************
 * File:        DEV-012-cellular.cpp
 * Author:      Markus Gerstenberg
 *
 * Description:
 * Integration test for the cellular_manager module.
 *
 * Test objective:
 * Verifies that the cellular_manager can establish
 * a working mobile data connection and provide a
 * usable network client to higher protocol layers.
 *
 * Tested functionality:
 * 1. Cellular manager initialization
 * 2. Non-blocking cellular connection handling
 * 3. Mobile network registration
 * 4. PPP data connection establishment
 * 5. IP address assignment
 * 6. DNS resolution through the cellular network
 * 7. TCP connection using getCellularClient()
 * 8. HTTP data transfer over the cellular connection
 *
 * Expected result:
 * - processCellularConnection() eventually returns
 *   NetworkConnectionState::CONNECTED
 * - example.com can be resolved
 * - TCP connection to example.com:80 succeeds
 * - HTTP response is received
 * - Test ends with "Cellular test successful"
 *
 * Notes:
 * - The test accesses the cellular connection only
 *   through the public cellular_manager API.
 * - PPP modem initialization and connection handling
 *   are verified through the cellular_manager API
 *   and are not accessed directly by this test.
 * - HTTP is used only to verify end-to-end IP
 *   connectivity and client provisioning.
 *************************************************/

#include <Arduino.h>
#include <Network.h>

#include "app/cellular_manager.h"


void setupDevCellular()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("--------------------------------");
    Serial.println("DEV-012 Cellular Manager Test");
    Serial.println("--------------------------------");

    initCellular();
}


void loopDevCellular()
{
    static bool testExecuted = false;

    NetworkConnectionState state =
        processCellularConnection();

    if (state != NetworkConnectionState::CONNECTED ||
        testExecuted)
    {
        return;
    }

    testExecuted = true;

    // -------------------------------------------------
    // Test 1: DNS resolution
    // -------------------------------------------------

    IPAddress address;

    Serial.println();
    Serial.println("Test 1: DNS resolution");
    Serial.print("Resolving example.com... ");

    if (Network.hostByName(
            "example.com",
            address) != 1)
    {
        Serial.println("FAILED");
        return;
    }

    Serial.println("OK");

    Serial.print("Address: ");
    Serial.println(address);


    // -------------------------------------------------
    // Test 2: TCP connection
    // -------------------------------------------------

    Serial.println();
    Serial.println("Test 2: TCP connection");

    Client& client = getCellularClient();

    Serial.print(
        "Connecting to example.com:80... ");

    if (!client.connect(
            "example.com",
            80))
    {
        Serial.println("FAILED");
        return;
    }

    Serial.println("OK");


    // -------------------------------------------------
    // Test 3: HTTP data transfer
    // -------------------------------------------------

    Serial.println();
    Serial.println("Test 3: HTTP data transfer");

    client.print(
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Connection: close\r\n"
        "\r\n");

    const uint32_t responseStart =
        millis();

    while (!client.available())
    {
        if (millis() - responseStart >
            10000)
        {
            Serial.println(
                "ERROR: HTTP response timeout");

            client.stop();
            return;
        }

        delay(10);
    }

    Serial.println("HTTP response received");
    Serial.println("--------------------------------");

    while (client.available())
    {
        Serial.write(client.read());
    }

    client.stop();

    Serial.println();
    Serial.println("--------------------------------");
    Serial.println("Cellular test successful");
    Serial.println("--------------------------------");
}