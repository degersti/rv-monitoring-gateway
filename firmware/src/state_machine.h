#pragma once

enum class ProgramState {
    BOOT,
    INIT_SYSTEM,
    CREATE_RECORD,
    LOAD_BUFFERED_RECORD,
    CONNECT_WIFI,
    CONNECT_MQTT,
    PUBLISH_DATA,
    BUFFER_DATA,
    WAIT_NEXT_CYCLE,
    SYNC_TIME,
    ERROR
};

enum class PublishSource
{
    CURRENT_MEASUREMENT,
    BUFFERED_RECORD
};

void initStateMachine();
void runStateMachine();