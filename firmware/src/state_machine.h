#pragma once

enum class ProgramState {
    BOOT,
    INIT_SYSTEM,
    COLLECT_DATA,
    CONNECT_WIFI,
    CONNECT_MQTT,
    PUBLISH_DATA,
    BUFFER_DATA,
    WAIT_NEXT_CYCLE,
    SYNC_TIME,
    ERROR
};

void initStateMachine();
void runStateMachine();