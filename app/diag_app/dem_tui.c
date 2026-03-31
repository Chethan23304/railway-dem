#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "Dem.h"
#include "Dem_EventConfig.h"
#include "NvM.h"
#include "brake_monitor.h"
#include "door_lock_monitor.h"
#include "temperature_monitor.h"
#include "motor_controller_monitor.h"
#include "comms_monitor.h"
#include "signal_monitor.h"
#include "hvac_monitor.h"
#include "power_monitor.h"

#define LOG_MAX    20
#define REFRESH_MS 500

static volatile int running = 1;
static char log_lines[LOG_MAX][80];
static int  log_count = 0;

static const char *event_names[15] = {
    "Brake Sensor Loss","Door Lock Failure","Over Temperature",
    "Motor Overcurrent","CAN Timeout","Signal Loss","HVAC Failure",
    "Power Undervoltage","Brake Pressure Low","Door Open At Speed",
    "Motor Stall","CAN Bus-Off","Ethernet Link Down",
    "Ethernet Timeout","Speed Sensor Fail"
};
static const Dem_EventIdType event_ids[15] = {
    RAIL_EVT_BRAKE_SENSORLOSS,RAIL_EVT_DOOR_LOCKFAIL,
    RAIL_EVT_TEMP_OVERTEMP,RAIL_EVT_MOTOR_OVERCURRENT,
    RAIL_EVT_CAN_TIMEOUT,RAIL_EVT_SIGNAL_LOSS,
    RAIL_EVT_HVAC_FAIL,RAIL_EVT_POWER_UNDERVOLT,
    RAIL_EVT_BRAKE_PRESSURE_LOW,RAIL_EVT_DOOR_OPEN_SPEED,
    RAIL_EVT_MOTOR_STALL,RAIL_EVT_CAN_BUS_OFF,
    RAIL_EVT_ETH_LINK_DOWN,RAIL_EVT_ETH_TIMEOUT,
    RAIL_EVT_SPEED_SENSOR_FAIL
};
static const char *sev_names[15] = {
    "HIGH","HIGH","MED","HIGH","MED",
    "MED","LOW","HIGH","HIGH","HIGH",
    "MED","HIGH","MED","MED","HIGH"
};

static void sig_handler(int s){(void)s;running=0;}

static void add_log(const char *msg)
{
    int i;
    if(log_count < LOG_MAX){ strncpy(log_lines[log_count++],msg,79); }
    else{
        for(i=0;i<LOG_MAX-1;i++) strcpy(log_lines[i],log_lines[i+1]);
        strncpy(log_lines[LOG_MAX-1],msg,79);
    }
}

static void draw_header(int cols)
{
    attron(A_BOLD|COLOR_PAIR(1));
    mvprintw(0,0,"%-*s",cols,
        " Railway DEM/DCM Monitor  [q=quit  c=clear  f=fault  s=save  r=reset]");
    attroff(A_BOLD|COLOR_PAIR(1));
}

static void draw_metrics(int ac, int cc)
{
    attron(A_BOLD); mvprintw(2,2,"Active DTCs:"); attroff(A_BOLD);
    attron(COLOR_PAIR(ac>0?3:2)|A_BOLD); mvprintw(2,15,"%d",ac);
    attroff(COLOR_PAIR(ac>0?3:2)|A_BOLD);
    attron(A_BOLD); mvprintw(2,30,"Confirmed:"); attroff(A_BOLD);
    attron(COLOR_PAIR(cc>0?4:2)|A_BOLD); mvprintw(2,41,"%d",cc);
    attroff(COLOR_PAIR(cc>0?4:2)|A_BOLD);
    attron(A_BOLD); mvprintw(2,55,"Total Events: 15"); attroff(A_BOLD);
}

static void draw_dtc_table(int sr)
{
    int i;
    attron(A_BOLD|COLOR_PAIR(1));
    mvprintw(sr,2,"%-4s %-22s %-12s %-6s %-9s %s",
             "ID","Event Name","DTC Code","Sev","Status","UDS");
    attroff(A_BOLD|COLOR_PAIR(1));
    mvhline(sr+1,2,ACS_HLINE,70);
    for(i=0;i<15;i++){
        Dem_EventStatusType uds=0;
        Dem_GetEventStatus(event_ids[i],&uds);
        int failed=(uds&0x01U)!=0, pending=(uds&0x04U)!=0;
        const char *st="PASSED "; int color=2;
        if(failed){st="FAILED ";color=3;}
        else if(pending){st="PENDING";color=4;}
        int sc=2;
        if(strcmp(sev_names[i],"HIGH")==0)sc=3;
        else if(strcmp(sev_names[i],"MED")==0)sc=4;
        mvprintw(sr+2+i,2,"%-4d %-22s 0x%06X     ",
                 i+1,event_names[i],0x010000+((i+1)<<8)+0x01);
        attron(COLOR_PAIR(sc));
        mvprintw(sr+2+i,42,"%-6s",sev_names[i]);
        attroff(COLOR_PAIR(sc));
        attron(COLOR_PAIR(color)|(failed?A_BOLD:0));
        mvprintw(sr+2+i,49,"%-9s",st);
        attroff(COLOR_PAIR(color)|A_BOLD);
        attron(uds?COLOR_PAIR(5):COLOR_PAIR(7));
        mvprintw(sr+2+i,59,"0x%02X",uds);
        attroff(COLOR_PAIR(5)|COLOR_PAIR(7));
    }
}

static void draw_log(int sr, int cols)
{
    int i, show=log_count<6?log_count:6;
    attron(A_BOLD|COLOR_PAIR(1));
    mvprintw(sr,2,"System Log:");
    attroff(A_BOLD|COLOR_PAIR(1));
    mvhline(sr+1,2,ACS_HLINE,cols-4);
    for(i=0;i<show;i++){
        int idx=log_count-show+i, color=7;
        if(strstr(log_lines[idx],"FAILED"))color=3;
        else if(strstr(log_lines[idx],"OK")||
                strstr(log_lines[idx],"saved")||
                strstr(log_lines[idx],"reset"))color=2;
        else if(strstr(log_lines[idx],"Injected")||
                strstr(log_lines[idx],"Restored"))color=4;
        attron(COLOR_PAIR(color));
        mvprintw(sr+2+i,2,"%-*s",cols-4,log_lines[idx]);
        attroff(COLOR_PAIR(color));
    }
}

static void run_monitors(void)
{
    BrakeMonitor_MainFunction(); DoorLockMonitor_MainFunction();
    TemperatureMonitor_MainFunction(); MotorMonitor_MainFunction();
    CommsMonitor_MainFunction(); SignalMonitor_MainFunction();
    HvacMonitor_MainFunction(); PowerMonitor_MainFunction();
    Dem_MainFunction();
}

static void inject_fault(void)
{
    static int idx=0; char msg[80];
    switch(idx%5){
        case 0: BrakeMonitor_SetSimulatedValues(50U,1.0f);
            snprintf(msg,80,"[ECU] Injected: Brake sensor out of range"); break;
        case 1: DoorLockMonitor_SetSimulatedValues(0,80U);
            snprintf(msg,80,"[ECU] Injected: Door open at speed 80kmh"); break;
        case 2: TemperatureMonitor_SetSimulatedValues(95.0f);
            snprintf(msg,80,"[ECU] Injected: Over temperature 95C"); break;
        case 3: CommsMonitor_SetSimulatedValues(0,1);
            snprintf(msg,80,"[ECU] Injected: CAN bus-off"); break;
        case 4: SignalMonitor_SetSimulatedValues(0,1,0);
            snprintf(msg,80,"[ECU] Injected: Ethernet link down"); break;
        default: snprintf(msg,80,"[ECU] Injected: fault"); break;
    }
    idx++; add_log(msg); run_monitors();
}

int main(void)
{
    signal(SIGINT,sig_handler);
    NvM_Init(); Dem_Init(); Dem_NvM_RestoreEventMemory();
    BrakeMonitor_Init(); DoorLockMonitor_Init();
    TemperatureMonitor_Init(); MotorMonitor_Init();
    CommsMonitor_Init(); SignalMonitor_Init();
    HvacMonitor_Init(); PowerMonitor_Init();
    add_log("[DEM] Initialized - 8 slots, 15 events");
    add_log("[NvM] Storage: dem_nvram.bin");

    initscr(); cbreak(); noecho();
    keypad(stdscr,TRUE); nodelay(stdscr,TRUE); curs_set(0);
    start_color();
    init_pair(1,COLOR_CYAN,   COLOR_BLACK);
    init_pair(2,COLOR_GREEN,  COLOR_BLACK);
    init_pair(3,COLOR_RED,    COLOR_BLACK);
    init_pair(4,COLOR_YELLOW, COLOR_BLACK);
    init_pair(5,COLOR_BLUE,   COLOR_BLACK);
    init_pair(6,COLOR_MAGENTA,COLOR_BLACK);
    init_pair(7,COLOR_WHITE,  COLOR_BLACK);

    while(running){
        int rows,cols; getmaxyx(stdscr,rows,cols);
        clear();
        Dem_FilterType filter; uint16_t ac=0,cc=0;
        Dem_SetDTCFilter(0x01U,&filter);
        Dem_GetNumberOfFilteredDTC(&filter,&ac);
        Dem_SetDTCFilter(0x08U,&filter);
        Dem_GetNumberOfFilteredDTC(&filter,&cc);
        draw_header(cols);
        draw_metrics((int)ac,(int)cc);
        mvhline(3,0,ACS_HLINE,cols);
        draw_dtc_table(4);
        mvhline(21,0,ACS_HLINE,cols);
        draw_log(22,cols);
        mvhline(rows-2,0,ACS_HLINE,cols);
        attron(COLOR_PAIR(7));
        mvprintw(rows-1,2,"q:Quit  c:ClearDTC  f:InjectFault  s:SaveNvM  r:Reset");
        attroff(COLOR_PAIR(7));
        refresh();
        int ch=getch();
        switch(ch){
            case 'q':case 'Q': running=0; break;
            case 'c':case 'C':
                Dem_ClearDTC(DEM_DTC_GROUP_ALL);
                add_log("[DEM] ClearDTC ALL - OK"); break;
            case 'f':case 'F': inject_fault(); break;
            case 's':case 'S':
                Dem_NvM_StoreEventMemory();
                add_log("[NvM] DTCs saved to dem_nvram.bin"); break;
            case 'r':case 'R':
                BrakeMonitor_Init(); DoorLockMonitor_Init();
                TemperatureMonitor_Init(); MotorMonitor_Init();
                CommsMonitor_Init(); SignalMonitor_Init();
                HvacMonitor_Init(); PowerMonitor_Init();
                Dem_ClearDTC(DEM_DTC_GROUP_ALL);
                add_log("[ECU] All monitors reset to nominal"); break;
        }
        usleep(REFRESH_MS*1000);
        (void)rows;
    }
    endwin();
    Dem_NvM_StoreEventMemory();
    printf("\n[ECU] Shutdown complete. DTCs saved.\n");
    return 0;
}
