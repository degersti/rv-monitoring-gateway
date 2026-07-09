#pragma once

enum class ProgramState {
    BOOT,
    INIT_HARDWARE,
    COLLECT_DATA,
    CONNECT_WIFI,
    CONNECT_MQTT,
    PUBLISH_DATA,
    BUFFER_DATA,
    WAIT_NEXT_CYCLE,
    ERROR
};

void initStateMachine();
void runStateMachine();