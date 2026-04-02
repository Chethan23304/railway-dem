#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "Dem.h"
#include "Dem_EventConfig.h"
#include "NvM.h"
#include "EvtLog.h"
#include "Kavach_Eth.h"

static volatile int running = 1;
static void sig_handler(int s) { (void)s; running = 0; }

/* Track which events already logged - log each FAILED only once */
static uint8_t already_logged[0x00C0U] = {0};

static void log_failed_event(Dem_EventIdType eventId,
                              Dem_DTCType dtc,
                              const char *name)
{
    if (eventId >= 0x00C0U) return;
    if (already_logged[eventId]) return;  /* already logged - skip */

    Dem_SetEventStatus(eventId, DEM_EVENT_STATUS_FAILED);

    Dem_EventStatusType uds = 0;
    Dem_GetEventStatus(eventId, &uds);

    EvtLog_Write(eventId, dtc, "FAILED", uds, 1U, "Kavach_ECU");

    already_logged[eventId] = 1U;

    printf("[ECU] LOGGED ONCE: 0x%04X %-25s DTC=0x%06X\n",
           eventId, name, dtc);
}

static void run_kavach_scenario(void)
{
    Kavach_EthFrameType frame = {0};

    printf("\n========================================\n");
    printf("  Kavach Event Scenario\n");
    printf("========================================\n\n");

    /* 1. Mode: Full Supervision */
    frame.msg_type = KAVACH_ETH_MSG_MODE_CHANGE;
    frame.event_id = KAVACH_EVT_MODE_FS;
    frame.dtc      = 0x010201U;
    frame.severity = DEM_SEVERITY_LOW;
    frame.status   = DEM_EVENT_STATUS_FAILED;
    Kavach_Eth_SendEvent(&frame);
    log_failed_event(KAVACH_EVT_MODE_FS, 0x010201U,
                     "Full_Supervision");
    sleep(1);

    /* 2. Critical: Over Speeding */
    frame.msg_type   = KAVACH_ETH_MSG_CRITICAL_EVENT;
    frame.event_id   = KAVACH_EVT_OVERSPEED;
    frame.dtc        = 0x00A101U;
    frame.severity   = DEM_SEVERITY_HIGH;
    frame.status     = DEM_EVENT_STATUS_FAILED;
    frame.payload[0] = 120U;  /* actual speed */
    frame.payload[1] = 80U;   /* permitted speed */
    Kavach_Eth_SendEvent(&frame);
    log_failed_event(KAVACH_EVT_OVERSPEED, 0x00A101U,
                     "Over_Speeding");
    sleep(1);

    /* 3. Critical: SPAD */
    frame.msg_type   = KAVACH_ETH_MSG_CRITICAL_EVENT;
    frame.event_id   = KAVACH_EVT_SPAD;
    frame.dtc        = 0x00A201U;
    frame.severity   = DEM_SEVERITY_HIGH;
    frame.status     = DEM_EVENT_STATUS_FAILED;
    frame.payload[0] = 0x02U;
    Kavach_Eth_SendEvent(&frame);
    log_failed_event(KAVACH_EVT_SPAD, 0x00A201U,
                     "SPAD");
    sleep(1);

    /* 4. Critical: Roll Back */
    frame.msg_type = KAVACH_ETH_MSG_CRITICAL_EVENT;
    frame.event_id = KAVACH_EVT_ROLLBACK;
    frame.dtc      = 0x00A401U;
    frame.severity = DEM_SEVERITY_HIGH;
    frame.status   = DEM_EVENT_STATUS_FAILED;
    Kavach_Eth_SendEvent(&frame);
    log_failed_event(KAVACH_EVT_ROLLBACK, 0x00A401U,
                     "Roll_Back");
    sleep(1);

    /* 5. Critical: Radio Loss */
    frame.msg_type = KAVACH_ETH_MSG_CRITICAL_EVENT;
    frame.event_id = KAVACH_EVT_RADIO_LOSS;
    frame.dtc      = 0x00A501U;
    frame.severity = DEM_SEVERITY_MEDIUM;
    frame.status   = DEM_EVENT_STATUS_FAILED;
    Kavach_Eth_SendEvent(&frame);
    log_failed_event(KAVACH_EVT_RADIO_LOSS, 0x00A501U,
                     "Radio_Loss");
    sleep(1);

    /* 6. Mode: Trip */
    frame.msg_type = KAVACH_ETH_MSG_MODE_CHANGE;
    frame.event_id = KAVACH_EVT_MODE_TR;
    frame.dtc      = 0x010701U;
    frame.severity = DEM_SEVERITY_HIGH;
    frame.status   = DEM_EVENT_STATUS_FAILED;
    Kavach_Eth_SendEvent(&frame);
    log_failed_event(KAVACH_EVT_MODE_TR, 0x010701U,
                     "Trip_SPAD_Emergency_Brake");
    sleep(1);

    /* 7. Critical: Brake Command */
    frame.msg_type = KAVACH_ETH_MSG_CRITICAL_EVENT;
    frame.event_id = KAVACH_EVT_BRAKE_CMD;
    frame.dtc      = 0x00A601U;
    frame.severity = DEM_SEVERITY_HIGH;
    frame.status   = DEM_EVENT_STATUS_FAILED;
    Kavach_Eth_SendEvent(&frame);
    log_failed_event(KAVACH_EVT_BRAKE_CMD, 0x00A601U,
                     "Brake_Command");
    sleep(1);

    /* 8. Info: RFID Tag Read */
    frame.msg_type   = KAVACH_ETH_MSG_INFO_EVENT;
    frame.event_id   = KAVACH_EVT_RFID;
    frame.dtc        = 0x00B101U;
    frame.severity   = DEM_SEVERITY_LOW;
    frame.status     = DEM_EVENT_STATUS_FAILED;
    frame.payload[0] = 0xABU;
    frame.payload[1] = 0xCDU;
    frame.payload[2] = 0xEFU;
    frame.payload[3] = 0x01U;
    Kavach_Eth_SendEvent(&frame);
    log_failed_event(KAVACH_EVT_RFID, 0x00B101U,
                     "RFID_Tag_Read");

    printf("\n========================================\n");
    printf("  All Kavach events sent and logged\n");
    printf("  Check: logs/events.txt\n");
    printf("         logs/events.csv\n");
    printf("         logs/events.json\n");
    printf("========================================\n\n");
}

int main(void)
{
    signal(SIGINT, sig_handler);

    printf("\n========================================\n");
    printf("  Railway ECU - Kavach DMI Integration\n");
    printf("  Kavach Simulator: %s:%d\n",
           KAVACH_ETH_IP, KAVACH_ETH_PORT);
    printf("========================================\n\n");

    NvM_Init();
    Dem_Init();
    Dem_NvM_RestoreEventMemory();
    EvtLog_Init();
    Kavach_Eth_Init();

    printf("[ECU] Ready. Starting scenario in 2 seconds...\n\n");
    sleep(2);

    /* Run scenario - send + log each event ONCE */
    run_kavach_scenario();

    /* Save to NvM */
    Dem_NvM_StoreEventMemory();

    /* Close logs */
    EvtLog_Close();
    Kavach_Eth_DeInit();

    printf("[ECU] Done. Logs saved.\n\n");
    return 0;
}
