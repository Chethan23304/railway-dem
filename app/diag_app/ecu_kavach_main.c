#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include "Dem.h"
#include "Dem_EventConfig.h"
#include "NvM.h"
#include "EvtLog.h"
#include "Kavach_Eth.h"
#include "Kavach_Conditions.h"

static volatile int running = 1;
static void sig_handler(int s) { (void)s; running = 0; }

static void run_kavach_scenario(void)
{
    printf("\n========================================\n");
    printf("  Kavach Real Condition Demo\n");
    printf("========================================\n\n");

    /* Step 1: Normal operation - no faults */
    printf("[DEMO] Step 1: Normal operation\n");
    Kavach_SetSpeed(60, 80);          /* speed OK */
    Kavach_SetSignalAspect(2);        /* GREEN */
    Kavach_SetRadio(1);               /* radio OK */
    Kavach_LiveData.direction_forward = 1U;
    Kavach_SetBrake(5);               /* brake OK */
    Kavach_SetSOS(0);                 /* no SOS */
    Kavach_SetRFID(1, 1);             /* tag read, valid location */
    Kavach_LiveData.current_time_ms   = 1000U;
    Kavach_LiveData.last_radio_rx_ms  = 1000U;
    Kavach_EvalConditions();
    printf("[DEMO] No faults expected above\n\n");
    sleep(1);

    /* Step 2: Over Speeding */
    printf("[DEMO] Step 2: Train overspeeds (120 > 80 kmh)\n");
    Kavach_SetSpeed(120, 80);
    Kavach_EvalConditions();
    sleep(1);

    /* Step 3: SPAD - passed red signal */
    printf("\n[DEMO] Step 3: SPAD - RED signal, train still moving\n");
    Kavach_SetSpeed(30, 80);
    Kavach_SetSignalAspect(0);        /* RED */
    Kavach_EvalConditions();
    sleep(1);

    /* Step 4: SOS button pressed */
    printf("\n[DEMO] Step 4: Driver presses SOS button\n");
    Kavach_SetSignalAspect(2);        /* clear signal */
    Kavach_SetSOS(1);
    Kavach_EvalConditions();
    sleep(1);

    /* Step 5: Radio loss */
    printf("\n[DEMO] Step 5: Radio loss (last RX was 6 seconds ago)\n");
    Kavach_SetSOS(0);
    Kavach_LiveData.current_time_ms  = 7000U;
    Kavach_LiveData.last_radio_rx_ms = 1000U; /* 6000ms ago */
    Kavach_EvalConditions();
    sleep(1);

    /* Step 6: Roll back */
    printf("\n[DEMO] Step 6: Train rolling backward\n");
    Kavach_SetRadio(1);
    Kavach_LiveData.last_radio_rx_ms = 7000U;
    Kavach_LiveData.direction_forward = 0U;   /* backward */
    Kavach_SetSpeed(10, 80);
    Kavach_EvalConditions();
    sleep(1);

    /* Step 7: RFID tag not read */
    printf("\n[DEMO] Step 7: RFID tag missed at station\n");
    Kavach_LiveData.direction_forward = 1U;
    Kavach_SetRFID(0, 0);             /* tag not read */
    Kavach_EvalConditions();
    sleep(1);

    /* Step 8: RFID tag at wrong location */
    printf("\n[DEMO] Step 8: RFID tag read at unexpected location\n");
    Kavach_SetRFID(1, 0);             /* tag read but wrong place */
    Kavach_EvalConditions();
    sleep(1);

    /* Step 9: Brake pressure low while moving */
    printf("\n[DEMO] Step 9: Low brake pressure while moving\n");
    Kavach_SetRFID(1, 1);
    Kavach_SetBrake(1);               /* pressure < 2 bar */
    Kavach_SetSpeed(50, 80);
    Kavach_EvalConditions();
    sleep(1);

    /* Step 10: Mode Trip */
    printf("\n[DEMO] Step 10: System enters Trip mode\n");
    Kavach_SetBrake(5);
    Kavach_SetMode(0x07U);
    Kavach_EvalConditions();
    sleep(1);

    printf("\n========================================\n");
    printf("  All conditions evaluated\n");
    printf("  Check: logs/events.txt\n");
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

    printf("[ECU] Ready. Starting in 1 second...\n\n");
    sleep(1);

    run_kavach_scenario();

    Dem_NvM_StoreEventMemory();
    EvtLog_Close();
    Kavach_Eth_DeInit();

    printf("[ECU] Done. Logs saved.\n\n");
    return 0;
}
