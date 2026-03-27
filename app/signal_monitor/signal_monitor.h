#ifndef SIGNAL_MONITOR_H
#define SIGNAL_MONITOR_H

#include "Std_Types.h"

void SignalMonitor_Init(void);
void SignalMonitor_MainFunction(void);
void SignalMonitor_SetSimulatedValues(uint8 signal_lost, uint8 eth_link_down,
                                      uint8 eth_timeout);

#endif /* SIGNAL_MONITOR_H */