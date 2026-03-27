#ifndef CAN_UDP_H
#define CAN_UDP_H

#include "Std_Types.h"
#include <stdint.h>

#define CAN_UDP_MCAST_GROUP   "239.0.0.1"
#define CAN_UDP_PORT           5555
#define CAN_UDP_MAX_FRAME_LEN  (4U + 1U + 8U)

#define CAN_ID_TESTER_REQ     0x7DFU
#define CAN_ID_ECU_RESP       0x7E8U
#define CAN_ID_TESTER_PHYS    0x7E0U

#pragma pack(push, 1)
typedef struct {
    uint32_t can_id;
    uint8_t  dlc;
    uint8_t  data[8];
} Can_UdpFrameType;
#pragma pack(pop)

int  Can_Udp_Init(void);
void Can_Udp_DeInit(void);
int  Can_Udp_Send(const Can_UdpFrameType *frame);
int  Can_Udp_Receive(Can_UdpFrameType *frame, int timeout_ms);

#endif /* CAN_UDP_H */
