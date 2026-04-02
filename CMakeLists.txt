cmake_minimum_required(VERSION 3.16)
project(railway_dem C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

include_directories(
    bsw/dem/include
    bsw/can/include
    bsw/nvm/include
    bsw/dcm/include
    bsw/kavach/include
)

set(DEM_SOURCES
    bsw/dem/src/Dem.c
    bsw/dem/src/DemLog.c
    bsw/dem/src/EvtLog.c
)

set(NVM_SOURCES
    bsw/nvm/src/NvM.c
)

set(CAN_SOURCES
    bsw/can/src/Can_Udp.c
    bsw/can/src/IsoTp.c
)

set(KAVACH_SOURCES
    bsw/kavach/src/Kavach_Bridge.c
    bsw/kavach/src/Kavach_Eth.c
)

# Kavach DMI demo - main executable
add_executable(rail_kavach_demo
    app/diag_app/ecu_kavach_main.c
    ${DEM_SOURCES}
    ${NVM_SOURCES}
    ${CAN_SOURCES}
    ${KAVACH_SOURCES}
    bsw/dcm/src/Dcm.c
)
target_link_libraries(rail_kavach_demo sqlite3)
