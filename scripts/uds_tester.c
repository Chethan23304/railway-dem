#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "Can_Udp.h"
#include "IsoTp.h"

static void print_response(const char *label, IsoTp_MessageType *msg)
{
    uint16_t i;
    printf("[TESTER] %s response (%d bytes): ", label, msg->length);
    for (i = 0U; i < msg->length; i++)
        printf("%02X ", msg->data[i]);
    printf("\n\n");
}

int main(void)
{
    IsoTp_MessageType resp;
    uint8_t req[8];

    printf("\n========================================\n");
    printf("  UDS Tester - Railway ECU\n");
    printf("========================================\n\n");

    if (Can_Udp_Init() != 0) return 1;

    sleep(1); /* let ECU settle */

    /* Test 1: UDS 0x19 0x02 - Read all active DTCs */
    printf("--- Test 1: ReadDTC (0x19 0x02) mask=0xFF ---\n");
    req[0] = 0x19U; req[1] = 0x02U; req[2] = 0xFFU;
    IsoTp_Send(CAN_ID_TESTER_REQ, req, 3U);
    memset(&resp, 0, sizeof(resp));
    if (IsoTp_Receive(&resp, 3000, CAN_ID_ECU_RESP) == E_OK)
        print_response("0x19", &resp);
    else
        printf("[TESTER] No response\n\n");

    sleep(1);

    /* Test 2: UDS 0x19 0x02 - Read only confirmed DTCs (bit3=0x08) */
    printf("--- Test 2: ReadDTC confirmed only (mask=0x08) ---\n");
    req[0] = 0x19U; req[1] = 0x02U; req[2] = 0x08U;
    IsoTp_Send(CAN_ID_TESTER_REQ, req, 3U);
    memset(&resp, 0, sizeof(resp));
    if (IsoTp_Receive(&resp, 3000, CAN_ID_ECU_RESP) == E_OK)
        print_response("0x19", &resp);
    else
        printf("[TESTER] No response\n\n");

    sleep(1);

    /* Test 3: UDS 0x14 - Clear all DTCs */
    printf("--- Test 3: ClearDTC (0x14) all ---\n");
    req[0] = 0x14U; req[1] = 0xFFU; req[2] = 0xFFU; req[3] = 0xFFU;
    IsoTp_Send(CAN_ID_TESTER_REQ, req, 4U);
    memset(&resp, 0, sizeof(resp));
    if (IsoTp_Receive(&resp, 3000, CAN_ID_ECU_RESP) == E_OK)
        print_response("0x14", &resp);
    else
        printf("[TESTER] No response\n\n");

    sleep(1);

    /* Test 4: Read again after clear - should be empty */
    printf("--- Test 4: ReadDTC after clear (expect 0 DTCs) ---\n");
    req[0] = 0x19U; req[1] = 0x02U; req[2] = 0xFFU;
    IsoTp_Send(CAN_ID_TESTER_REQ, req, 3U);
    memset(&resp, 0, sizeof(resp));
    if (IsoTp_Receive(&resp, 3000, CAN_ID_ECU_RESP) == E_OK)
        print_response("0x19", &resp);
    else
        printf("[TESTER] No response\n\n");

    printf("========================================\n");
    printf("  All UDS tests complete\n");
    printf("========================================\n\n");

    Can_Udp_DeInit();
    return 0;
}