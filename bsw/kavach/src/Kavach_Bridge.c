#include "Kavach_Bridge.h"
#include "Dem.h"
#include "Dem_EventConfig.h"
#include "DemLog.h"
#include "EvtLog.h"
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

static int kav_sock = -1;
static struct sockaddr_in kav_addr;
static uint8_t prev_status[DEM_NUM_EVENTS] = {0};
static uint32_t heartbeat_counter = 0U;

static const uint8_t kav_severity[15] = {
    3,3,2,3,2,2,1,3,3,3,2,3,2,2,3
};

static const uint32_t kav_dtcs[15] = {
    0x010101,0x010201,0x010301,0x010401,0x010501,
    0x010601,0x010701,0x010801,0x010901,0x010A01,
    0x010B01,0x010C01,0x010D01,0x010E01,0x010F01
};

void Kavach_Bridge_Init(void)
{
    struct ip_mreq mreq;
    int reuse = 1;
    struct sockaddr_in bind_addr;

    kav_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (kav_sock < 0) { perror("[KAVACH] socket"); return; }

    setsockopt(kav_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(kav_sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family      = AF_INET;
    bind_addr.sin_port        = htons(KAVACH_SIM_PORT);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(kav_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr));

    mreq.imr_multiaddr.s_addr = inet_addr(KAVACH_SIM_IP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(kav_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    memset(&kav_addr, 0, sizeof(kav_addr));
    kav_addr.sin_family      = AF_INET;
    kav_addr.sin_port        = htons(KAVACH_SIM_PORT);
    kav_addr.sin_addr.s_addr = inet_addr(KAVACH_SIM_IP);

    printf("[KAVACH] Bridge initialized -> %s:%d\n", KAVACH_SIM_IP, KAVACH_SIM_PORT);
}

static void kavach_send(uint32_t msg_id, uint8_t *data, uint8_t len)
{
    uint8_t frame[13] = {0};
    frame[0] = (uint8_t)((msg_id >> 24U) & 0xFFU);
    frame[1] = (uint8_t)((msg_id >> 16U) & 0xFFU);
    frame[2] = (uint8_t)((msg_id >>  8U) & 0xFFU);
    frame[3] = (uint8_t)( msg_id         & 0xFFU);
    frame[4] = len;
    memcpy(&frame[5], data, len < 8U ? len : 8U);
    sendto(kav_sock, frame, sizeof(frame), 0,
           (struct sockaddr *)&kav_addr, sizeof(kav_addr));
    DemLog_WriteKavach(msg_id, data, len, "TX");
}

void Kavach_Bridge_SendFaultActive(Dem_EventIdType eventId,
                                    uint32_t dtc, uint8_t severity)
{
    uint8_t payload[8] = {0};
    payload[0] = (uint8_t)((dtc >> 16U) & 0xFFU);
    payload[1] = (uint8_t)((dtc >>  8U) & 0xFFU);
    payload[2] = (uint8_t)( dtc         & 0xFFU);
    payload[3] = severity;
    payload[4] = (uint8_t)(eventId & 0xFFU);
    payload[5] = 0x01U;
    kavach_send(KAVACH_MSG_FAULT_ACTIVE, payload, 6U);
}

void Kavach_Bridge_SendFaultCleared(uint32_t dtc)
{
    uint8_t payload[8] = {0};
    payload[0] = (uint8_t)((dtc >> 16U) & 0xFFU);
    payload[1] = (uint8_t)((dtc >>  8U) & 0xFFU);
    payload[2] = (uint8_t)( dtc         & 0xFFU);
    payload[3] = 0x00U;
    kavach_send(KAVACH_MSG_FAULT_CLEARED, payload, 4U);
}

void Kavach_Bridge_SendHeartbeat(void)
{
    uint8_t payload[4];
    heartbeat_counter++;
    payload[0] = (uint8_t)((heartbeat_counter >> 24U) & 0xFFU);
    payload[1] = (uint8_t)((heartbeat_counter >> 16U) & 0xFFU);
    payload[2] = (uint8_t)((heartbeat_counter >>  8U) & 0xFFU);
    payload[3] = (uint8_t)( heartbeat_counter         & 0xFFU);
    kavach_send(KAVACH_MSG_HEARTBEAT, payload, 4U);
}

void Kavach_Bridge_MainFunction(void)
{
    uint8_t i;
    Dem_EventStatusType uds_status;
    static uint32_t hb_tick = 0U;

    for (i = 0U; i < DEM_NUM_EVENTS; i++)
    {
        Dem_EventIdType eid = (Dem_EventIdType)(i + 1U);
        Dem_GetEventStatus(eid, &uds_status);

        uint8_t now_failed = (uds_status & DEM_UDS_STATUS_TF) ? 1U : 0U;
        uint8_t was_failed = (prev_status[i] & DEM_UDS_STATUS_TF) ? 1U : 0U;

        if (now_failed && !was_failed)
        {
            Kavach_Bridge_SendFaultActive(eid, kav_dtcs[i], kav_severity[i]);
            DemLog_Write(eid, kav_dtcs[i], LOG_EVENT_FAILED,
                         uds_status, 1U, "Kavach_Bridge");
            EvtLog_Write(eid, kav_dtcs[i], "FAILED",
                         uds_status, 1U, "Kavach_Bridge");
        }
        else if (!now_failed && was_failed)
        {
            Kavach_Bridge_SendFaultCleared(kav_dtcs[i]);
            DemLog_Write(eid, kav_dtcs[i], LOG_EVENT_PASSED,
                         uds_status, 0U, "Kavach_Bridge");
            EvtLog_Write(eid, kav_dtcs[i], "PASSED",
                         uds_status, 0U, "Kavach_Bridge");
        }
        prev_status[i] = uds_status;
    }

    hb_tick++;
    if (hb_tick >= 100U) {
        Kavach_Bridge_SendHeartbeat();
        hb_tick = 0U;
    }
}

void Kavach_Bridge_DeInit(void)
{
    if (kav_sock >= 0) { close(kav_sock); kav_sock = -1; }
    printf("[KAVACH] Bridge closed\n");
}
