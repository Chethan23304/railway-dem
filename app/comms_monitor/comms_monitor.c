#include "comms_monitor.h"
#include "Dem_EventConfig.h"
#include "Dem.h"

static uint8 sim_can_timeout = FALSE;
static uint8 sim_can_bus_off = FALSE;

void CommsMonitor_Init(void)
{
    sim_can_timeout = FALSE;
    sim_can_bus_off = FALSE;
}

void CommsMonitor_SetSimulatedValues(uint8 can_timeout, uint8 can_bus_off)
{
    sim_can_timeout = can_timeout;
    sim_can_bus_off = can_bus_off;
}

void CommsMonitor_MainFunction(void)
{
    Dem_EventStatusType timeout_status;
    Dem_EventStatusType busoff_status;

    timeout_status = (sim_can_timeout == TRUE)
                     ? DEM_EVENT_STATUS_FAILED
                     : DEM_EVENT_STATUS_PASSED;

    busoff_status  = (sim_can_bus_off == TRUE)
                     ? DEM_EVENT_STATUS_FAILED
                     : DEM_EVENT_STATUS_PASSED;

    (void)Dem_SetEventStatus(RAIL_EVT_CAN_TIMEOUT, timeout_status);
    (void)Dem_SetEventStatus(RAIL_EVT_CAN_BUS_OFF, busoff_status);
}