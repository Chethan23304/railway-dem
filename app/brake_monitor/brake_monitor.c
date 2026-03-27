#include "brake_monitor.h"
#include "Dem_EventConfig.h"
#include "Dem.h"

#define BRAKE_SENSOR_VALID_MIN  (100U)
#define BRAKE_SENSOR_VALID_MAX  (4000U)
#define BRAKE_PRESSURE_MIN_BAR  (2.0f)

static uint16 brake_sensor_raw = 2048U;
static float  brake_pressure   = 5.0f;

void BrakeMonitor_Init(void)
{
    brake_sensor_raw = 2048U;
    brake_pressure   = 5.0f;
}

void BrakeMonitor_SetSimulatedValues(uint16 raw, float pressure)
{
    brake_sensor_raw = raw;
    brake_pressure   = pressure;
}

void BrakeMonitor_MainFunction(void)
{
    Dem_EventStatusType sensor_status;
    Dem_EventStatusType pressure_status;

    if ((brake_sensor_raw < BRAKE_SENSOR_VALID_MIN) ||
        (brake_sensor_raw > BRAKE_SENSOR_VALID_MAX))
    {
        sensor_status = DEM_EVENT_STATUS_FAILED;
    }
    else
    {
        sensor_status = DEM_EVENT_STATUS_PASSED;
    }

    if (brake_pressure < BRAKE_PRESSURE_MIN_BAR)
    {
        pressure_status = DEM_EVENT_STATUS_FAILED;
    }
    else
    {
        pressure_status = DEM_EVENT_STATUS_PASSED;
    }

    (void)Dem_SetEventStatus(RAIL_EVT_BRAKE_SENSORLOSS,   sensor_status);
    (void)Dem_SetEventStatus(RAIL_EVT_BRAKE_PRESSURE_LOW, pressure_status);
}