#include "IsoTp.h"
#include <string.h>
#include <stdio.h>

Std_ReturnType IsoTp_Send(uint32_t can_id, const uint8_t *data, uint16_t len)
{
    Can_UdpFrameType frame;
    uint16_t offset;
    uint8_t  sn;
    uint16_t chunk;

    memset(&frame, 0, sizeof(frame));
    frame.can_id = can_id;

    if (len <= 7U)
    {
        /* Single Frame */
        frame.dlc     = (uint8_t)(len + 1U);
        frame.data[0] = (uint8_t)len;
        memcpy(&frame.data[1], data, len);
        return (Can_Udp_Send(&frame) == 0) ? E_OK : E_NOT_OK;
    }

    /* First Frame */
    frame.dlc      = 8U;
    frame.data[0]  = (uint8_t)(ISOTP_FF | ((len >> 8U) & 0x0FU));
    frame.data[1]  = (uint8_t)(len & 0xFFU);
    memcpy(&frame.data[2], data, 6U);
    if (Can_Udp_Send(&frame) != 0) return E_NOT_OK;

    /* Consecutive Frames */
    offset = 6U;
    sn     = 1U;
    while (offset < len)
    {
       chunk = (((uint16_t)(len - offset)) > 7U) ? 7U : ((uint16_t)(len - offset));
        memset(&frame, 0, sizeof(frame));
        frame.can_id   = can_id;
        frame.dlc      = (uint8_t)(chunk + 1U);
        frame.data[0]  = (uint8_t)(ISOTP_CF | (sn & 0x0FU));
        memcpy(&frame.data[1], &data[offset], chunk);
        if (Can_Udp_Send(&frame) != 0) return E_NOT_OK;
        offset += chunk;
        sn++;
    }
    return E_OK;
}

Std_ReturnType IsoTp_Receive(IsoTp_MessageType *msg, int timeout_ms)
{
    Can_UdpFrameType frame;
    uint8_t pci_type;

    if (!msg) return E_NOT_OK;

    memset(&frame, 0, sizeof(frame));
    if (Can_Udp_Receive(&frame, timeout_ms) <= 0) return E_NOT_OK;

    pci_type = frame.data[0] & 0xF0U;

    if (pci_type == ISOTP_SF)
    {
        msg->length = frame.data[0] & 0x0FU;
        memcpy(msg->data, &frame.data[1], msg->length);
        printf("[ISO-TP] SF received, length=%d\n", msg->length);
        return E_OK;
    }

    printf("[ISO-TP] Non-SF frame received (FF/CF not yet implemented)\n");
    return E_NOT_OK;
}