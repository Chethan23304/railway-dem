#include "DemLog.h"
#include "Dem_EventConfig.h"
#include <sqlite3.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

static sqlite3 *db      = NULL;
static FILE    *log_txt = NULL;

static const char *event_names[16] = {
    "UNKNOWN",
    "Brake Sensor Loss",    "Door Lock Failure",
    "Over Temperature",     "Motor Overcurrent",
    "CAN Timeout",          "Signal Loss",
    "HVAC Failure",         "Power Undervoltage",
    "Brake Pressure Low",   "Door Open At Speed",
    "Motor Stall",          "CAN Bus-Off",
    "Ethernet Link Down",   "Ethernet Timeout",
    "Speed Sensor Fail"
};

static const char *log_event_str[] = {
    "FAILED","PASSED","CLEARED","RESTORED","KAVACH_MSG"
};

static void get_timestamp(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", t);
}

void DemLog_Init(void)
{
    char ts[32];
    mkdir("logs", 0755);

    if (sqlite3_open(DEM_LOG_DB_PATH, &db) != SQLITE_OK) {
        printf("[LOG] ERROR: %s\n", sqlite3_errmsg(db));
        return;
    }

    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS dem_events ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp TEXT NOT NULL,"
        "event_id  INTEGER NOT NULL,"
        "event_name TEXT NOT NULL,"
        "dtc        TEXT NOT NULL,"
        "log_event  TEXT NOT NULL,"
        "uds_status TEXT NOT NULL,"
        "occurrences INTEGER NOT NULL,"
        "source     TEXT NOT NULL);",
        NULL, NULL, NULL);

    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS kavach_messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp   TEXT NOT NULL,"
        "msg_id      TEXT NOT NULL,"
        "direction   TEXT NOT NULL,"
        "payload_hex TEXT NOT NULL,"
        "length      INTEGER NOT NULL);",
        NULL, NULL, NULL);

    log_txt = fopen(DEM_LOG_TXT_PATH, "a");
    get_timestamp(ts, sizeof(ts));
    if (log_txt) {
        fprintf(log_txt,
            "\n==============================\n"
            "  DEM Logger Started: %s\n"
            "==============================\n", ts);
        fflush(log_txt);
    }
    printf("[LOG] Initialized -> %s | %s\n",
           DEM_LOG_DB_PATH, DEM_LOG_TXT_PATH);
}

void DemLog_Write(Dem_EventIdType eventId, Dem_DTCType dtc,
                  DemLog_EventType logEvent, uint8_t udsStatus,
                  uint8_t occurrences, const char *source)
{
    char ts[32], dtc_str[12], uds_str[8], sql[512];
    const char *name;

    get_timestamp(ts, sizeof(ts));
    snprintf(dtc_str, sizeof(dtc_str), "0x%06X", dtc);
    snprintf(uds_str, sizeof(uds_str), "0x%02X",  udsStatus);
    name = (eventId < 16U) ? event_names[eventId] : "UNKNOWN";

    if (db) {
        snprintf(sql, sizeof(sql),
            "INSERT INTO dem_events "
            "(timestamp,event_id,event_name,dtc,log_event,"
            "uds_status,occurrences,source) "
            "VALUES ('%s',%d,'%s','%s','%s','%s',%d,'%s');",
            ts, eventId, name, dtc_str,
            log_event_str[logEvent],
            uds_str, occurrences, source ? source : "");
        sqlite3_exec(db, sql, NULL, NULL, NULL);
    }
    if (log_txt) {
        fprintf(log_txt,
            "[%s] %-9s | EventId=0x%04X | %-22s | "
            "DTC=%s | UDS=%s | occ=%d | src=%s\n",
            ts, log_event_str[logEvent],
            eventId, name, dtc_str,
            uds_str, occurrences, source ? source : "");
        fflush(log_txt);
    }
    printf("[LOG] %-9s EventId=0x%04X %-22s DTC=%s\n",
           log_event_str[logEvent], eventId, name, dtc_str);
}

void DemLog_WriteKavach(uint32_t kavach_msg_id,
                        const uint8_t *payload,
                        uint8_t len, const char *direction)
{
    char ts[32], hex[64]={0}, msg_id_str[12], sql[512];
    uint8_t i;

    get_timestamp(ts, sizeof(ts));
    snprintf(msg_id_str, sizeof(msg_id_str), "0x%08X", kavach_msg_id);
    for (i=0U; i<len && i<8U; i++)
        snprintf(hex+(i*3), sizeof(hex)-(i*3), "%02X ", payload[i]);

    if (db) {
        snprintf(sql, sizeof(sql),
            "INSERT INTO kavach_messages "
            "(timestamp,msg_id,direction,payload_hex,length) "
            "VALUES ('%s','%s','%s','%s',%d);",
            ts, msg_id_str, direction, hex, len);
        sqlite3_exec(db, sql, NULL, NULL, NULL);
    }
    if (log_txt) {
        fprintf(log_txt, "[%s] KAVACH %-4s | ID=%s | [%s]\n",
                ts, direction, msg_id_str, hex);
        fflush(log_txt);
    }
    printf("[LOG] KAVACH %-4s ID=%s [%s]\n",
           direction, msg_id_str, hex);
}

void DemLog_Close(void)
{
    char ts[32];
    get_timestamp(ts, sizeof(ts));
    if (log_txt) {
        fprintf(log_txt, "[%s] Logger closed\n"
                "==============================\n", ts);
        fclose(log_txt); log_txt = NULL;
    }
    if (db) { sqlite3_close(db); db = NULL; }
    printf("[LOG] Logger closed\n");
}
