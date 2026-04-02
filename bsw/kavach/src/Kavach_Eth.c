#include "Kavach_Eth.h"
#include "Dem.h"
#include "Dem_EventConfig.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

static int tx_sock    = -1;
static int rx_sock    = -1;
static struct sockaddr_in sim_addr;
static uint32_t hb_counter = 0U;

/* Event name lookup */
static const char *kavach_event_names[] = {
    "UNKNOWN",
    "Stand_By",           /* 0x01 */
    "Full_Supervision",   /* 0x02 */
    "Limited_Supervision",/* 0x03 */
    "Staff_Responsible",  /* 0x04 */
    "Shunting",           /* 0x05 */
    "On_Sight",           /* 0x06 */
    "Trip",               /* 0x07 */
    "Post_Trip",          /* 0x08 */
    "Reverse",            /* 0x09 */
};

static const char *get_event_name(uint16_t event_id)
{
    if (event_id <= 0x09U) return kavach_event_names[event_id];
    if (event_id == 0x0FU) return "System_Failure";
    if (event_id == 0xA1U) return "Over_Speeding";
    if (event_id == 0xA2U) return "SPAD";
    if (event_id == 0xA3U) return "SOS_Received";
    if (event_id == 0xA4U) return "Roll_Back";
    if (event_id == 0xA5U) return "Radio_Loss";
    if (event_id == 0xA6U) return "Brake_Command";
    if (event_id == 0xB1U) return "RFID_Tag_Read";
    if (event_id == 0xB2U) return "Aspect_Change";
    if (event_id == 0xB3U) return "MA_Update";
    return "UNKNOWN";
}

void Kavach_Eth_Init(void)
{
    int reuse = 1;
    struct sockaddr_in rx_addr;

    /* TX socket - send to simulator */
    tx_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (tx_sock < 0) { perror("[KAVACH_ETH] TX socket"); return; }

    memset(&sim_addr, 0, sizeof(sim_addr));
    sim_addr.sin_family      = AF_INET;
    sim_addr.sin_port        = htons(KAVACH_ETH_PORT);
    sim_addr.sin_addr.s_addr = inet_addr(KAVACH_ETH_IP);

    /* RX socket - receive from simulator */
    rx_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (rx_sock < 0) { perror("[KAVACH_ETH] RX socket"); return; }

    setsockopt(rx_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(rx_sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

    memset(&rx_addr, 0, sizeof(rx_addr));
    rx_addr.sin_family      = AF_INET;
    rx_addr.sin_port        = htons(KAVACH_ETH_LISTEN_PORT);
    rx_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(rx_sock, (struct sockaddr *)&rx_addr, sizeof(rx_addr)) < 0)
        perror("[KAVACH_ETH] RX bind");

    printf("[KAVACH_ETH] Initialized\n"
           "  TX -> %s:%d\n"
           "  RX <- 0.0.0.0:%d\n",
           KAVACH_ETH_IP, KAVACH_ETH_PORT, KAVACH_ETH_LISTEN_PORT);
}

void Kavach_Eth_DeInit(void)
{
    if (tx_sock >= 0) { close(tx_sock); tx_sock = -1; }
    if (rx_sock >= 0) { close(rx_sock); rx_sock = -1; }
    printf("[KAVACH_ETH] Closed\n");
}

void Kavach_Eth_SendEvent(const Kavach_EthFrameType *frame)
{
    uint8_t  buf[KAVACH_ETH_FRAME_LEN] = {0};
    if (!frame || tx_sock < 0) return;

    /* Pack frame into buffer */
    buf[0]  = frame->msg_type;
    buf[1]  = (uint8_t)((frame->event_id >> 8U) & 0xFFU);
    buf[2]  = (uint8_t)( frame->event_id        & 0xFFU);
    buf[3]  = (uint8_t)((frame->dtc >> 16U) & 0xFFU);
    buf[4]  = (uint8_t)((frame->dtc >>  8U) & 0xFFU);
    buf[5]  = (uint8_t)( frame->dtc         & 0xFFU);
    buf[6]  = frame->severity;
    buf[7]  = frame->status;
    memcpy(&buf[8], frame->payload, 8U);

    sendto(tx_sock, buf, KAVACH_ETH_FRAME_LEN, 0,
           (struct sockaddr *)&sim_addr, sizeof(sim_addr));

    printf("[KAVACH_ETH] TX MsgType=0x%02X EventId=0x%04X "
           "DTC=0x%06X Sev=%d Status=0x%02X -> %s:%d\n",
           frame->msg_type, frame->event_id, frame->dtc,
           frame->severity, frame->status,
           KAVACH_ETH_IP, KAVACH_ETH_PORT);

    /* Logging done in ecu_kavach_main - not here */
}

void Kavach_Eth_SendHeartbeat(void)
{
    Kavach_EthFrameType frame = {0};
    hb_counter++;
    frame.msg_type  = KAVACH_ETH_MSG_HEARTBEAT;
    frame.event_id  = 0x0000U;
    frame.payload[0]= (uint8_t)((hb_counter >> 24U) & 0xFFU);
    frame.payload[1]= (uint8_t)((hb_counter >> 16U) & 0xFFU);
    frame.payload[2]= (uint8_t)((hb_counter >>  8U) & 0xFFU);
    frame.payload[3]= (uint8_t)( hb_counter         & 0xFFU);

    uint8_t buf[KAVACH_ETH_FRAME_LEN] = {0};
    buf[0] = frame.msg_type;
    memcpy(&buf[8], frame.payload, 4U);
    sendto(tx_sock, buf, KAVACH_ETH_FRAME_LEN, 0,
           (struct sockaddr *)&sim_addr, sizeof(sim_addr));
    printf("[KAVACH_ETH] Heartbeat #%u sent\n", hb_counter);
}

int Kavach_Eth_Receive(Kavach_EthFrameType *frame, int timeout_ms)
{
    uint8_t buf[KAVACH_ETH_FRAME_LEN] = {0};
    struct timeval tv;
    ssize_t n;

    if (!frame || rx_sock < 0) return -1;

    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(rx_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    n = recv(rx_sock, buf, sizeof(buf), 0);
    if (n <= 0) return -1;

    frame->msg_type  = buf[0];
    frame->event_id  = ((uint16_t)buf[1] << 8U) | buf[2];
    frame->dtc       = ((uint32_t)buf[3] << 16U) |
                       ((uint32_t)buf[4] <<  8U) |
                        (uint32_t)buf[5];
    frame->severity  = buf[6];
    frame->status    = buf[7];
    memcpy(frame->payload, &buf[8], 8U);

    printf("[KAVACH_ETH] RX MsgType=0x%02X EventId=0x%04X "
           "(%s) DTC=0x%06X\n",
           frame->msg_type, frame->event_id,
           get_event_name(frame->event_id), frame->dtc);
    return 0;
}

void Kavach_Eth_MainFunction(void)
{
    Kavach_EthFrameType rx_frame = {0};
    static uint32_t tick = 0U;

    tick++;

    /* Poll for incoming messages from simulator */
    if (Kavach_Eth_Receive(&rx_frame, 10) == 0)
    {
        printf("[KAVACH_ETH] Simulator msg received: "
               "type=0x%02X event=0x%04X\n",
               rx_frame.msg_type, rx_frame.event_id);

        /* Log received message */
        EvtLog_Write((Dem_EventIdType)rx_frame.event_id,
                     rx_frame.dtc, "RX_FROM_SIM",
                     rx_frame.status, 0U, "Kavach_Eth_RX");
    }
}
