# Railway DEM/DCM — AUTOSAR Diagnostic Stack

Complete AUTOSAR Classic DEM and DCM implementation in pure C for a railway embedded system.

## Features
- 15 railway fault events with DTC codes
- UDS services: 0x19 ReadDTC, 0x14 ClearDTC
- CAN transport over UDP with ISO-TP
- NvM persistence (DTCs survive power cycle)
- Counter and time-based debounce
- 10 Unity unit tests — all passing
- GitHub Actions CI/CD pipeline
- ncurses terminal UI dashboard
- Flask web dashboard

## Build and Run
```bash
mkdir build && cd build
cmake ..
make -j4
./rail_dem        # standalone demo
./rail_ecu        # ECU server
./uds_tester      # UDS tester
./dem_tui         # terminal dashboard
ctest             # unit tests
```

## Stack
- Language: C99
- Build: CMake
- Test: Unity
- Transport: CAN-UDP multicast 239.0.0.1:5555
- Platform: Ubuntu 22.04 / WSL2
