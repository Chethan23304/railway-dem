cmake_minimum_required(VERSION 3.16)
project(railway_dem C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

# Include paths
include_directories(
    bsw/dem/include
    bsw/can/include
    bsw/nvm/include
    bsw/dcm/include
    app/brake_monitor
    app/door_lock_monitor
    app/temperature_monitor
    app/motor_controller_monitor
    app/comms_monitor
    app/signal_monitor
    app/hvac_monitor
    app/power_monitor
)

# DEM Core
set(DEM_SOURCES
    bsw/dem/src/Dem.c
)

# CAN-UDP transport
set(CAN_SOURCES
    bsw/can/src/Can_Udp.c
    bsw/can/src/IsoTp.c
)

set(NVM_SOURCES
    bsw/nvm/src/NvM.c
)

# Application SW-Components
set(ASW_SOURCES
    app/brake_monitor/brake_monitor.c
    app/door_lock_monitor/door_lock_monitor.c
    app/temperature_monitor/temperature_monitor.c
    app/motor_controller_monitor/motor_controller_monitor.c
    app/comms_monitor/comms_monitor.c
    app/signal_monitor/signal_monitor.c
    app/hvac_monitor/hvac_monitor.c
    app/power_monitor/power_monitor.c
)

# Main executable
add_executable(rail_dem
    app/diag_app/main.c
    ${ASW_SOURCES}
    ${DEM_SOURCES}
    ${NVM_SOURCES}
    ${CAN_SOURCES}
)



# UDS tester tool
add_executable(uds_tester
    scripts/uds_tester.c
    ${CAN_SOURCES}
)
target_include_directories(uds_tester PRIVATE bsw/dem/include)

add_executable(rail_ecu
    app/diag_app/ecu_main.c
    ${ASW_SOURCES}
    ${DEM_SOURCES}
    ${NVM_SOURCES}
    ${CAN_SOURCES}
    bsw/dcm/src/Dcm.c
)
target_include_directories(rail_ecu PRIVATE bsw/dcm/include bsw/nvm/include)

# Unity unit tests
enable_testing()

add_executable(test_dem
    tests/unit/test_dem.c
    tests/unity/unity.c
    ${DEM_SOURCES}
    ${NVM_SOURCES}
)
target_include_directories(test_dem PRIVATE
    tests/unity
    bsw/dem/include
    bsw/nvm/include
)
add_test(NAME DEM_Unit_Tests COMMAND test_dem)