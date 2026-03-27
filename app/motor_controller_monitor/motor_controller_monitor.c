#include "motor_controller_monitor.h"
#include "Dem_EventConfig.h"
#include "Dem.h"

#define MOTOR_OVERCURRENT_AMPS  (50.0f)

static float motor_current_amps = 10.0f;
static uint8 motor_stall        = FALSE;

void MotorMonitor_Init(void)
{
    motor_current_amps = 10.0f;
    motor_stall        = FALSE;
}

void MotorMonitor_SetSimulatedValues(float current_amps, uint8 stall_detected)
{
    motor_current_amps = current_amps;
    motor_stall        = stall_detected;
}

void MotorMonitor_MainFunction(void)
{
    Dem_EventStatusType overcurrent_status;
    Dem_EventStatusType stall_status;

    overcurrent_status = (motor_current_amps > MOTOR_OVERCURRENT_AMPS)
                         ? DEM_EVENT_STATUS_FAILED
                         : DEM_EVENT_STATUS_PASSED;

    stall_status = (motor_stall == TRUE)
                   ? DEM_EVENT_STATUS_PREFAILED
                   : DEM_EVENT_STATUS_PASSED;

    (void)Dem_SetEventStatus(RAIL_EVT_MOTOR_OVERCURRENT, overcurrent_status);
    (void)Dem_SetEventStatus(RAIL_EVT_MOTOR_STALL,       stall_status);
}
