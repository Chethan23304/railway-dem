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
        frame.dlc     = (uint8_t)(len + 1U);
        frame.data[0] = (uint8_t)len;
        memcpy(&frame.data[1], data, len);
        return (Can_Udp_Send(&frame) == 0) ? E_OK : E_NOT_OK;
    }

    frame.dlc     = 8U;
    frame.data[0] = (uint8_t)(ISOTP_FF | ((len >> 8U) & 0x0FU));
    frame.data[1] = (uint8_t)(len & 0xFFU);
    memcpy(&frame.data[2], data, 6U);
    if (Can_Udp_Send(&frame) != 0) return E_NOT_OK;

    offset = 6U;
    sn     = 1U;
    while (offset < len)
    {
        chunk = (((uint16_t)(len - offset)) > 7U)
                ? 7U : ((uint16_t)(len - offset));
        memset(&frame, 0, sizeof(frame));
        frame.can_id  = can_id;
        frame.dlc     = (uint8_t)(chunk + 1U);
        frame.data[0] = (uint8_t)(ISOTP_CF | (sn & 0x0FU));
        memcpy(&frame.data[1], &data[offset], chunk);
        if (Can_Udp_Send(&frame) != 0) return E_NOT_OK;
        offset += chunk;
        sn++;
    }
    return E_OK;
}

Std_ReturnType IsoTp_Receive(IsoTp_MessageType *msg, int timeout_ms,
                              uint32_t rx_filter)
{
    Can_UdpFrameType frame;
    uint8_t          pci_type;
    uint8_t          attempts;

    if (!msg) return E_NOT_OK;

    for (attempts = 0U; attempts < 10U; attempts++)
    {
        memset(&frame, 0, sizeof(frame));
        if (Can_Udp_Receive(&frame, timeout_ms) <= 0) return E_NOT_OK;

        if (rx_filter != 0U && frame.can_id != rx_filter)
        {
            printf("[ISO-TP] Skipping ID=0x%03X (want 0x%03X)\n",
                   frame.can_id, rx_filter);
            continue;
        }

        pci_type = frame.data[0] & 0xF0U;

        if (pci_type == ISOTP_SF)
        {
            msg->length = frame.data[0] & 0x0FU;
            memcpy(msg->data, &frame.data[1], msg->length);
            printf("[ISO-TP] SF received length=%d\n", msg->length);
            return E_OK;
        }

        if (pci_type == ISOTP_FF)
        {
            Can_UdpFrameType fc_frame;
            Can_UdpFrameType cf_frame;
            uint16_t total_len;
            uint16_t offset;
            uint8_t  sn_expected;
            uint16_t chunk;

            total_len = (uint16_t)(((uint16_t)(frame.data[0] & 0x0FU) << 8U)
                                    | (uint16_t)frame.data[1]);
            if (total_len > ISOTP_MAX_PAYLOAD) return E_NOT_OK;

            memcpy(msg->data, &frame.data[2], 6U);
            offset      = 6U;
            sn_expected = 1U;

            memset(&fc_frame, 0, sizeof(fc_frame));
            fc_frame.can_id  = CAN_ID_TESTER_REQ;
            fc_frame.dlc     = 3U;
            fc_frame.data[0] = ISOTP_FC;
            fc_frame.data[1] = 0U;
            fc_frame.data[2] = 0U;
            Can_Udp_Send(&fc_frame);

            while (offset < total_len)
            {
                memset(&cf_frame, 0, sizeof(cf_frame));
                if (Can_Udp_Receive(&cf_frame, 1000) <= 0) return E_NOT_OK;
                if (rx_filter != 0U && cf_frame.can_id != rx_filter) continue;
                if ((cf_frame.data[0] & 0xF0U) != ISOTP_CF) return E_NOT_OK;
                if ((cf_frame.data[0] & 0x0FU) != (sn_expected & 0x0FU))
                    return E_NOT_OK;

                chunk = (((uint16_t)(total_len - offset)) > 7U)
                        ? 7U : (uint16_t)(total_len - offset);
                memcpy(&msg->data[offset], &cf_frame.data[1], chunk);
                offset += chunk;
                sn_expected++;
            }

            msg->length = total_len;
            printf("[ISO-TP] FF reassembled total=%d\n", total_len);
            return E_OK;
        }

        if (pci_type == ISOTP_FC)
        {
            printf("[ISO-TP] Skipping FC\n");
            continue;
        }
    }

    return E_NOT_OK;
}
