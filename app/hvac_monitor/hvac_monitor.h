#ifndef HVAC_MONITOR_H
#define HVAC_MONITOR_H

#include "Std_Types.h"

void HvacMonitor_Init(void);
void HvacMonitor_MainFunction(void);
void HvacMonitor_SetSimulatedValues(uint8 hvac_fail);

#endif /* HVAC_MONITOR_H */