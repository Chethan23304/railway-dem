#include "power_monitor.h"
#include "Dem_EventConfig.h"
#include "Dem.h"

#define VOLTAGE_MIN_V       (20.0f)
#define SPEED_SENSOR_MAX    (4000U)

static float  sim_voltage  = 24.0f;
static uint16 sim_speed_raw = 2048U;

void PowerMonitor_Init(void)
{
    sim_voltage   = 24.0f;
    sim_speed_raw = 2048U;
}

void PowerMonitor_SetSimulatedValues(float voltage, uint16 speed_kmh)
{
    sim_voltage   = voltage;
    sim_speed_raw = speed_kmh;
}

void PowerMonitor_MainFunction(void)
{
    Dem_EventStatusType volt_status;
    Dem_EventStatusType speed_status;

    volt_status = (sim_voltage < VOLTAGE_MIN_V)
                  ? DEM_EVENT_STATUS_FAILED
                  : DEM_EVENT_STATUS_PASSED;

    speed_status = (sim_speed_raw > SPEED_SENSOR_MAX)
                   ? DEM_EVENT_STATUS_FAILED
                   : DEM_EVENT_STATUS_PASSED;

    (void)Dem_SetEventStatus(RAIL_EVT_POWER_UNDERVOLT,  volt_status);
    (void)Dem_SetEventStatus(RAIL_EVT_SPEED_SENSOR_FAIL, speed_status);
}