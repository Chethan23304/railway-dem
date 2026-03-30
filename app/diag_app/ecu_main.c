#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include "Dem.h"
#include "Dcm.h"
#include "Can_Udp.h"
#include "brake_monitor.h"
#include "door_lock_monitor.h"
#include "temperature_monitor.h"
#include "motor_controller_monitor.h"
#include "comms_monitor.h"
#include "signal_monitor.h"
#include "hvac_monitor.h"
#include "power_monitor.h"
#include "NvM.h"

static volatile int running = 1;

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    signal(SIGINT, sig_handler);

    printf("\n========================================\n");
    printf("  Railway ECU - DEM/DCM Server\n");
    printf("  Send UDS requests to %s:%d\n",
           CAN_UDP_MCAST_GROUP, CAN_UDP_PORT);
    printf("========================================\n\n");

    /* Init all modules */
    if (Can_Udp_Init() != 0) return 1;
    NvM_Init();
    Dem_Init();
    Dem_NvM_RestoreEventMemory();
    Dcm_Init();


    BrakeMonitor_Init();
    DoorLockMonitor_Init();
    TemperatureMonitor_Init();
    MotorMonitor_Init();
    CommsMonitor_Init();
    SignalMonitor_Init();
    HvacMonitor_Init();
    PowerMonitor_Init();

    /* Inject some faults so there are DTCs to read */
    printf("[ECU] Injecting railway faults...\n\n");
    BrakeMonitor_SetSimulatedValues(50U, 1.0f);
    DoorLockMonitor_SetSimulatedValues(FALSE, 80U);
    MotorMonitor_SetSimulatedValues(75.0f, FALSE);
    CommsMonitor_SetSimulatedValues(FALSE, TRUE);
    SignalMonitor_SetSimulatedValues(FALSE, TRUE, FALSE);

    /* Run SW-Components once to populate DEM */
    BrakeMonitor_MainFunction();
    DoorLockMonitor_MainFunction();
    TemperatureMonitor_MainFunction();
    MotorMonitor_MainFunction();
    CommsMonitor_MainFunction();
    SignalMonitor_MainFunction();
    HvacMonitor_MainFunction();
    PowerMonitor_MainFunction();
    Dem_MainFunction();

    printf("[ECU] Ready - waiting for UDS requests (Ctrl+C to stop)\n\n");

    /* Main loop - process DCM requests */
    while (running)
    {
        Dcm_MainFunction();
    }

    printf("\n[ECU] Shutting down\n");
    Dem_NvM_StoreEventMemory();
    Can_Udp_DeInit();
    return 0;
}