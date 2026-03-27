#include "hvac_monitor.h"
#include "Dem_EventConfig.h"
#include "Dem.h"

static uint8 sim_hvac_fail = FALSE;

void HvacMonitor_Init(void)
{
    sim_hvac_fail = FALSE;
}

void HvacMonitor_SetSimulatedValues(uint8 hvac_fail)
{
    sim_hvac_fail = hvac_fail;
}

void HvacMonitor_MainFunction(void)
{
    Dem_EventStatusType status;

    status = (sim_hvac_fail == TRUE)
             ? DEM_EVENT_STATUS_FAILED
             : DEM_EVENT_STATUS_PASSED;

    (void)Dem_SetEventStatus(RAIL_EVT_HVAC_FAIL, status);
}