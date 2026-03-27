#ifndef BRAKE_MONITOR_H
#define BRAKE_MONITOR_H

#include "Std_Types.h"

void BrakeMonitor_Init(void);
void BrakeMonitor_MainFunction(void);
void BrakeMonitor_SetSimulatedValues(uint16 raw, float pressure);

#endif /* BRAKE_MONITOR_H */