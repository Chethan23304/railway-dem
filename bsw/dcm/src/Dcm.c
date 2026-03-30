#include "Dcm.h"
#include "Can_Udp.h"
#include <string.h>
#include <stdio.h>

static void Dcm_HandleReadDTC(const uint8_t *req, uint16_t len);
static void Dcm_HandleClearDTC(const uint8_t *req, uint16_t len);
static void Dcm_SendNRC(uint8_t sid, uint8_t nrc);

void Dcm_Init(void)
{
    printf("[DCM] Initialized - listening on UDP %s:%d\n",
           CAN_UDP_MCAST_GROUP, CAN_UDP_PORT);
}

void Dcm_MainFunction(void)
{
    IsoTp_MessageType msg;
    memset(&msg, 0, sizeof(msg));

    if (IsoTp_Receive(&msg, 100, CAN_ID_TESTER_REQ) != E_OK) return;
    if (msg.length == 0U) return;

    uint8_t sid = msg.data[0];
    printf("[DCM] Request SID=0x%02X length=%d\n", sid, msg.length);

    switch (sid)
    {
        case UDS_SID_READ_DTC:
            Dcm_HandleReadDTC(msg.data, msg.length);
            break;
        case UDS_SID_CLEAR_DTC:
            Dcm_HandleClearDTC(msg.data, msg.length);
            break;
        default:
            Dcm_SendNRC(sid, UDS_NRC_SNS);
            break;
    }
}

/* UDS 0x19 0x02 - ReadDTCByStatusMask */
static void Dcm_HandleReadDTC(const uint8_t *req, uint16_t len)
{
    uint8_t        resp[3U + (15U * 4U)];
    uint16_t       resp_len = 0U;
    Dem_FilterType filter;
    Dem_DTCType    dtc;
    uint8_t        dtc_status;
    uint16_t       count = 0U;

    if (len < 3U || req[1] != 0x02U)
    {
        Dcm_SendNRC(UDS_SID_READ_DTC, UDS_NRC_IMLOIF);
        return;
    }

    uint8_t mask = req[2];

    resp[resp_len++] = UDS_SID_READ_DTC | UDS_SID_POS_RESP;
    resp[resp_len++] = 0x02U;
    resp[resp_len++] = mask;

    Dem_SetDTCFilter(mask, &filter);
    Dem_GetNumberOfFilteredDTC(&filter, &count);

    Dem_SetDTCFilter(mask, &filter);
    while (Dem_GetNextFilteredDTC(&filter, &dtc, &dtc_status) == E_OK)
    {
        if (resp_len + 4U > sizeof(resp)) break;
        resp[resp_len++] = (uint8_t)((dtc >> 16U) & 0xFFU);
        resp[resp_len++] = (uint8_t)((dtc >>  8U) & 0xFFU);
        resp[resp_len++] = (uint8_t)( dtc         & 0xFFU);
        resp[resp_len++] = dtc_status;
    }

    printf("[DCM] 0x19 response: %d DTCs found\n", count);
    IsoTp_Send(CAN_ID_ECU_RESP, resp, resp_len);
}

/* UDS 0x14 - ClearDiagnosticInformation */
static void Dcm_HandleClearDTC(const uint8_t *req, uint16_t len)
{
    uint32_t group;
    uint8_t  pos_resp[1];

    if (len < 4U)
    {
        Dcm_SendNRC(UDS_SID_CLEAR_DTC, UDS_NRC_IMLOIF);
        return;
    }

    group = ((uint32_t)req[1] << 16U) |
            ((uint32_t)req[2] <<  8U) |
             (uint32_t)req[3];

    if (Dem_ClearDTC(group) == E_OK)
    {
        pos_resp[0] = UDS_SID_CLEAR_DTC | UDS_SID_POS_RESP;
        IsoTp_Send(CAN_ID_ECU_RESP, pos_resp, 1U);
        printf("[DCM] 0x14 ClearDTC group=0x%06X OK\n", group);
    }
    else
    {
        Dcm_SendNRC(UDS_SID_CLEAR_DTC, UDS_NRC_IMLOIF);
    }
}

static void Dcm_SendNRC(uint8_t sid, uint8_t nrc)
{
    uint8_t nrc_resp[3] = {0x7FU, sid, nrc};
    IsoTp_Send(CAN_ID_ECU_RESP, nrc_resp, 3U);
    printf("[DCM] NRC sent: SID=0x%02X NRC=0x%02X\n", sid, nrc);
}