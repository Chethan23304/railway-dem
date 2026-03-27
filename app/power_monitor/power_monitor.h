#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include "Std_Types.h"

void PowerMonitor_Init(void);
void PowerMonitor_MainFunction(void);
void PowerMonitor_SetSimulatedValues(float voltage, uint16 speed_kmh);

#endif /* POWER_MONITOR_H */