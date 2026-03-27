#include "temperature_monitor.h"
#include "Dem_EventConfig.h"
#include "Dem.h"

#define TEMP_MAX_CELSIUS  (85.0f)

static float temperature_celsius = 25.0f;

void TemperatureMonitor_Init(void)
{
    temperature_celsius = 25.0f;
}

void TemperatureMonitor_SetSimulatedValues(float temp_celsius)
{
    temperature_celsius = temp_celsius;
}

void TemperatureMonitor_MainFunction(void)
{
    Dem_EventStatusType status;

    status = (temperature_celsius > TEMP_MAX_CELSIUS)
             ? DEM_EVENT_STATUS_PREFAILED
             : DEM_EVENT_STATUS_PASSED;

    (void)Dem_SetEventStatus(RAIL_EVT_TEMP_OVERTEMP, status);
}