#ifndef DOOR_LOCK_MONITOR_H
#define DOOR_LOCK_MONITOR_H

#include "Std_Types.h"

void DoorLockMonitor_Init(void);
void DoorLockMonitor_MainFunction(void);
void DoorLockMonitor_SetSimulatedValues(uint8 door_locked, uint16 speed_kmh);

#endif /* DOOR_LOCK_MONITOR_H */