#ifndef ISOTP_H
#define ISOTP_H

#include "Std_Types.h"
#include "Can_Udp.h"

#define ISOTP_MAX_PAYLOAD   4095U

#define ISOTP_SF   0x00U
#define ISOTP_FF   0x10U
#define ISOTP_CF   0x20U
#define ISOTP_FC   0x30U

typedef struct {
    uint8_t  data[ISOTP_MAX_PAYLOAD];
    uint16_t length;
} IsoTp_MessageType;

Std_ReturnType IsoTp_Send(uint32_t can_id, const uint8_t *data, uint16_t len);
Std_ReturnType IsoTp_Receive(IsoTp_MessageType *msg, int timeout_ms);

#endif /* ISOTP_H */