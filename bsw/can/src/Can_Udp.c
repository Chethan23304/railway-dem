#include "Can_Udp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

static int udp_sock = -1;
static struct sockaddr_in mcast_addr;

int Can_Udp_Init(void)
{
    int reuse = 1;
    struct ip_mreq mreq;
    struct sockaddr_in bind_addr;

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) { perror("[CAN_UDP] socket"); return -1; }

    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(udp_sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family      = AF_INET;
    bind_addr.sin_port        = htons(CAN_UDP_PORT);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(udp_sock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("[CAN_UDP] bind"); close(udp_sock); return -1;
    }

    mreq.imr_multiaddr.s_addr = inet_addr(CAN_UDP_MCAST_GROUP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   &mreq, sizeof(mreq)) < 0) {
        perror("[CAN_UDP] IP_ADD_MEMBERSHIP"); close(udp_sock); return -1;
    }

    memset(&mcast_addr, 0, sizeof(mcast_addr));
    mcast_addr.sin_family      = AF_INET;
    mcast_addr.sin_port        = htons(CAN_UDP_PORT);
    mcast_addr.sin_addr.s_addr = inet_addr(CAN_UDP_MCAST_GROUP);

    printf("[CAN_UDP] Initialized on %s:%d\n", CAN_UDP_MCAST_GROUP, CAN_UDP_PORT);
    return 0;
}

void Can_Udp_DeInit(void)
{
    if (udp_sock >= 0) { close(udp_sock); udp_sock = -1; }
}

int Can_Udp_Send(const Can_UdpFrameType *frame)
{
    int i;
    if (!frame || udp_sock < 0) return -1;

    ssize_t sent = sendto(udp_sock, frame, sizeof(Can_UdpFrameType), 0,
                          (struct sockaddr *)&mcast_addr, sizeof(mcast_addr));
    if (sent < 0) { perror("[CAN_UDP] sendto"); return -1; }

    printf("[CAN_UDP] TX: ID=0x%03X DLC=%d Data=", frame->can_id, frame->dlc);
    for (i = 0; i < frame->dlc; i++) printf("%02X ", frame->data[i]);
    printf("\n");
    return 0;
}

int Can_Udp_Receive(Can_UdpFrameType *frame, int timeout_ms)
{
    int i;
    struct timeval tv;
    if (!frame || udp_sock < 0) return -1;

    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    ssize_t received = recv(udp_sock, frame, sizeof(Can_UdpFrameType), 0);
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        perror("[CAN_UDP] recv"); return -1;
    }

    printf("[CAN_UDP] RX: ID=0x%03X DLC=%d Data=", frame->can_id, frame->dlc);
    for (i = 0; i < frame->dlc; i++) printf("%02X ", frame->data[i]);
    printf("\n");
    return (int)received;
}