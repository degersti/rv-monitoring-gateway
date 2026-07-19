#pragma once

enum class ProgramState
{
    BOOT,
    INIT_SYSTEM,

    SYNC_TIME,
    CREATE_RECORD,
    LOAD_BUFFERED_RECORD,

    CONNECT_WIFI,
    CONNECT_MQTT,
    PUBLISH_DATA,
    BUFFER_DATA,

    POWER_DECISION,
    WAIT_NEXT_CYCLE,
    ENTER_DEEP_SLEEP,

    ERROR
};

enum class PublishSource
{
    CURRENT_MEASUREMENT,
    BUFFERED_RECORD
};

void initStateMachine();
void runStateMachine();