#include <stdio.h>
#include <unistd.h>

#include "Dem.h"
#include "Dem_EventConfig.h"
#include "brake_monitor.h"
#include "door_lock_monitor.h"
#include "temperature_monitor.h"
#include "motor_controller_monitor.h"
#include "comms_monitor.h"
#include "signal_monitor.h"
#include "hvac_monitor.h"
#include "power_monitor.h"

#define MAIN_LOOP_CYCLES  (5U)

static void System_Init(void)
{
    printf("\n========================================\n");
    printf("  Railway DEM/DCM System Starting\n");
    printf("========================================\n\n");

    Dem_Init();

    BrakeMonitor_Init();
    DoorLockMonitor_Init();
    TemperatureMonitor_Init();
    MotorMonitor_Init();
    CommsMonitor_Init();
    SignalMonitor_Init();
    HvacMonitor_Init();
    PowerMonitor_Init();

    printf("[MAIN] All SW-Components initialized\n\n");
}

static void System_InjectFaults(void)
{
    printf("========================================\n");
    printf("  Injecting test faults\n");
    printf("========================================\n\n");

    /* Brake sensor out of range */
    BrakeMonitor_SetSimulatedValues(50U, 5.0f);

    /* Door open while moving */
    DoorLockMonitor_SetSimulatedValues(FALSE, 80U);

    /* Over temperature */
    TemperatureMonitor_SetSimulatedValues(95.0f);

    /* Motor overcurrent */
    MotorMonitor_SetSimulatedValues(75.0f, FALSE);

    /* CAN bus off */
    CommsMonitor_SetSimulatedValues(FALSE, TRUE);

    /* Ethernet link down */
    SignalMonitor_SetSimulatedValues(FALSE, TRUE, FALSE);

    /* HVAC failure */
    HvacMonitor_SetSimulatedValues(TRUE);

    /* Undervoltage */
    PowerMonitor_SetSimulatedValues(15.0f, 2048U);
}

static void System_RunMainFunctions(void)
{
    BrakeMonitor_MainFunction();
    DoorLockMonitor_MainFunction();
    TemperatureMonitor_MainFunction();
    MotorMonitor_MainFunction();
    CommsMonitor_MainFunction();
    SignalMonitor_MainFunction();
    HvacMonitor_MainFunction();
    PowerMonitor_MainFunction();
    Dem_MainFunction();
}

static void System_PrintDTCReport(void)
{
    Dem_FilterType   filter;
    Dem_DTCType      dtc;
    uint8_t          dtc_status;
    uint16_t         count = 0U;

    printf("\n========================================\n");
    printf("  DTC Report (UDS 0x19 simulation)\n");
    printf("========================================\n");

    Dem_SetDTCFilter(DEM_UDS_STATUS_TF, &filter);
    Dem_GetNumberOfFilteredDTC(&filter, &count);
    printf("Active failed DTCs: %d\n\n", count);

    Dem_SetDTCFilter(DEM_UDS_STATUS_TF, &filter);
    while (Dem_GetNextFilteredDTC(&filter, &dtc, &dtc_status) == E_OK)
    {
        printf("  DTC: 0x%06X  Status: 0x%02X\n", dtc, dtc_status);
    }
    printf("\n");
}

static void System_ClearAndVerify(void)
{
    uint16_t count = 0U;
    Dem_FilterType filter;

    printf("========================================\n");
    printf("  Clearing all DTCs (UDS 0x14)\n");
    printf("========================================\n\n");

    Dem_ClearDTC(DEM_DTC_GROUP_ALL);

    Dem_SetDTCFilter(0xFFU, &filter);
    Dem_GetNumberOfFilteredDTC(&filter, &count);
    printf("DTCs remaining after clear: %d\n\n", count);
}

int main(void)
{
    uint8_t cycle;

    System_Init();

    /* Run 2 clean cycles first - no faults */
    printf("--- Running 2 clean cycles (no faults) ---\n\n");
    for (cycle = 0U; cycle < 2U; cycle++)
    {
        printf("[Cycle %u]\n", cycle + 1U);
        System_RunMainFunctions();
    }

    /* Inject faults */
    System_InjectFaults();

    /* Run fault cycles */
    printf("\n--- Running %u fault cycles ---\n\n", MAIN_LOOP_CYCLES);
    for (cycle = 0U; cycle < MAIN_LOOP_CYCLES; cycle++)
    {
        printf("[Cycle %u]\n", cycle + 1U);
        System_RunMainFunctions();
        sleep(1);
    }

    /* Print DTC report */
    System_PrintDTCReport();

    /* Clear all DTCs */
    System_ClearAndVerify();

    printf("========================================\n");
    printf("  System test complete\n");
    printf("========================================\n\n");

    return 0;
}