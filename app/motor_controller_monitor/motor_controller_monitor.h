#ifndef MOTOR_CONTROLLER_MONITOR_H
#define MOTOR_CONTROLLER_MONITOR_H

#include "Std_Types.h"

void MotorMonitor_Init(void);
void MotorMonitor_MainFunction(void);
void MotorMonitor_SetSimulatedValues(float current_amps, uint8 stall_detected);

#endif /* MOTOR_CONTROLLER_MONITOR_H */