#ifndef TEMPERATURE_MONITOR_H
#define TEMPERATURE_MONITOR_H

#include "Std_Types.h"

void TemperatureMonitor_Init(void);
void TemperatureMonitor_MainFunction(void);
void TemperatureMonitor_SetSimulatedValues(float temp_celsius);

#endif /* TEMPERATURE_MONITOR_H */