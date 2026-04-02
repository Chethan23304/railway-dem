#ifndef KAVACH_BRIDGE_H
#define KAVACH_BRIDGE_H

#include "Std_Types.h"
#include "Dem.h"

#define KAVACH_SIM_IP       "239.0.0.1"
#define KAVACH_SIM_PORT     5556

#define KAVACH_MSG_FAULT_ACTIVE    0x00000181U
#define KAVACH_MSG_FAULT_CLEARED   0x00000182U
#define KAVACH_MSG_DTC_REQUEST     0x00000183U
#define KAVACH_MSG_DTC_RESPONSE    0x00000184U
#define KAVACH_MSG_HEARTBEAT       0x000001FFU

void Kavach_Bridge_Init(void);
void Kavach_Bridge_MainFunction(void);
void Kavach_Bridge_SendFaultActive(Dem_EventIdType eventId, uint32_t dtc, uint8_t severity);
void Kavach_Bridge_SendFaultCleared(uint32_t dtc);
void Kavach_Bridge_SendHeartbeat(void);
void Kavach_Bridge_DeInit(void);

#endif
