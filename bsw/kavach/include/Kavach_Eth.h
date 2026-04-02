#ifndef KAVACH_ETH_H
#define KAVACH_ETH_H

#include "Std_Types.h"
#include "Dem_EventConfig.h"

/* Kavach DMI Simulator Ethernet config */
#define KAVACH_ETH_IP           "192.168.0.110"   /* change to simulator IP */
#define KAVACH_ETH_PORT         1502
#define KAVACH_ETH_LISTEN_PORT  5601

/* Kavach Ethernet message types */
#define KAVACH_ETH_MSG_MODE_CHANGE    0x01U
#define KAVACH_ETH_MSG_CRITICAL_EVENT 0x02U
#define KAVACH_ETH_MSG_INFO_EVENT     0x03U
#define KAVACH_ETH_MSG_HEARTBEAT      0x04U
#define KAVACH_ETH_MSG_DTC_REPORT     0x05U
#define KAVACH_ETH_MSG_ACK            0x06U

/* Kavach Ethernet frame:
   [MSG_TYPE(1)] [EVENT_ID(2)] [DTC(3)] [SEVERITY(1)] [STATUS(1)] [PAYLOAD(8)] = 16 bytes
*/
#define KAVACH_ETH_FRAME_LEN    16U

typedef struct {
    uint8_t  msg_type;
    uint16_t event_id;
    uint32_t dtc;
    uint8_t  severity;
    uint8_t  status;
    uint8_t  payload[8];
} Kavach_EthFrameType;

void     Kavach_Eth_Init(void);
void     Kavach_Eth_DeInit(void);
void     Kavach_Eth_SendEvent(const Kavach_EthFrameType *frame);
void     Kavach_Eth_SendHeartbeat(void);
void     Kavach_Eth_MainFunction(void);
int      Kavach_Eth_Receive(Kavach_EthFrameType *frame, int timeout_ms);

#endif /* KAVACH_ETH_H */
