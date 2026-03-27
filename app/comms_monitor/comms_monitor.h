#ifndef COMMS_MONITOR_H
#define COMMS_MONITOR_H

#include "Std_Types.h"

void CommsMonitor_Init(void);
void CommsMonitor_MainFunction(void);
void CommsMonitor_SetSimulatedValues(uint8 can_timeout, uint8 can_bus_off);

#endif /* COMMS_MONITOR_H */