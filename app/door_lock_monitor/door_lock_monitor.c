#include "door_lock_monitor.h"
#include "Dem_EventConfig.h"
#include "Dem.h"

#define SPEED_THRESHOLD_KMH  (5U)

static uint8  door_locked = TRUE;
static uint16 speed_kmh   = 0U;

void DoorLockMonitor_Init(void)
{
    door_locked = TRUE;
    speed_kmh   = 0U;
}

void DoorLockMonitor_SetSimulatedValues(uint8 locked, uint16 speed)
{
    door_locked = locked;
    speed_kmh   = speed;
}

void DoorLockMonitor_MainFunction(void)
{
    Dem_EventStatusType lock_status;
    Dem_EventStatusType door_speed_status;

    lock_status = (door_locked == TRUE)
                  ? DEM_EVENT_STATUS_PASSED
                  : DEM_EVENT_STATUS_FAILED;

    if ((door_locked == FALSE) && (speed_kmh > SPEED_THRESHOLD_KMH))
    {
        door_speed_status = DEM_EVENT_STATUS_FAILED;
    }
    else
    {
        door_speed_status = DEM_EVENT_STATUS_PASSED;
    }

    (void)Dem_SetEventStatus(RAIL_EVT_DOOR_LOCKFAIL,   lock_status);
    (void)Dem_SetEventStatus(RAIL_EVT_DOOR_OPEN_SPEED, door_speed_status);
}