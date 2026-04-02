#include "EvtLog.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

static FILE *csv_file  = NULL;
static FILE *json_file = NULL;
static FILE *txt_file  = NULL;
static int   first_json_entry = 1;
static int   event_count      = 0;

/* Kavach event name lookup by event ID */
static const char *get_kavach_event_name(uint16_t id)
{
    switch(id) {
        case 0x0001: return "Stand_By";
        case 0x0002: return "Full_Supervision";
        case 0x0003: return "Limited_Supervision";
        case 0x0004: return "Staff_Responsible";
        case 0x0005: return "Shunting";
        case 0x0006: return "On_Sight";
        case 0x0007: return "Trip";
        case 0x0008: return "Post_Trip";
        case 0x0009: return "Reverse";
        case 0x000F: return "System_Failure";
        case 0x00A1: return "Over_Speeding";
        case 0x00A2: return "SPAD";
        case 0x00A3: return "SOS_Received";
        case 0x00A4: return "Roll_Back";
        case 0x00A5: return "Radio_Loss";
        case 0x00A6: return "Brake_Command";
        case 0x00B1: return "RFID_Tag_Read";
        case 0x00B2: return "Aspect_Change";
        case 0x00B3: return "MA_Update";
        default:     return "UNKNOWN";
    }
}

static const char *severity_str[16] = {
    "UNKNOWN",
    "HIGH","HIGH","MEDIUM","HIGH","MEDIUM",
    "MEDIUM","LOW","HIGH","HIGH","HIGH",
    "MEDIUM","HIGH","MEDIUM","MEDIUM","HIGH"
};

static const char *transport_str[16] = {
    "UNKNOWN",
    "CAN","CAN","CAN","CAN","CAN",
    "CAN","CAN","CAN","CAN","CAN",
    "CAN","CAN","Ethernet","Ethernet","CAN"
};

static void get_ts(char *buf, size_t len)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", t);
}

void EvtLog_Init(void)
{
    mkdir("logs", 0755);

    /* CSV file */
    csv_file = fopen(EVT_LOG_PATH, "w");
    if (csv_file)
    {
        fprintf(csv_file,
            "No,Timestamp,EventId,EventName,DTC,"
            "EventType,UDS_Status,Occurrences,"
            "Severity,Transport,Source\n");
    }

    /* JSON file */
    json_file = fopen(EVT_LOG_JSON, "w");
    if (json_file)
    {
        fprintf(json_file, "[\n");
        first_json_entry = 1;
    }

    /* Plain text file */
    txt_file = fopen(EVT_LOG_TXT, "a");
    if (txt_file)
    {
        char ts[32];
        get_ts(ts, sizeof(ts));
        fprintf(txt_file,
            "\n"
            "╔══════════════════════════════════════════════════════╗\n"
            "║         Railway DEM Event Log                        ║\n"
            "║         Session Started: %-28s║\n"
            "╚══════════════════════════════════════════════════════╝\n",
            ts);
        fprintf(txt_file,
            "%-4s %-20s %-10s %-26s %-10s %-8s %-6s %-8s\n",
            "No","Timestamp","DTC","Event Name",
            "Type","UDS","Occ","Source");
        fprintf(txt_file,
            "%-4s %-20s %-10s %-26s %-10s %-8s %-6s %-8s\n",
            "----","--------------------","----------",
            "--------------------------","----------",
            "--------","------","--------");
    }

    printf("[EvtLog] Initialized\n"
           "  CSV:  %s\n"
           "  JSON: %s\n"
           "  TXT:  %s\n",
           EVT_LOG_PATH, EVT_LOG_JSON, EVT_LOG_TXT);
}

void EvtLog_Write(Dem_EventIdType eventId,
                  Dem_DTCType     dtc,
                  const char     *event_type,
                  uint8_t         uds_status,
                  uint8_t         occurrences,
                  const char     *source)
{
    char ts[32];
    char dtc_str[12];
    char uds_str[8];
    const char *name, *sev, *trans;

    event_count++;
    get_ts(ts, sizeof(ts));
    snprintf(dtc_str, sizeof(dtc_str), "0x%06X", dtc);
    snprintf(uds_str, sizeof(uds_str), "0x%02X",  uds_status);

    name  = get_kavach_event_name((uint16_t)eventId);
    sev   = (eventId >= 0x00A1U && eventId <= 0x00A6U) ? "HIGH" :
        (eventId == 0x00A5U) ? "MEDIUM" :
        (eventId >= 0x00B1U) ? "LOW" :
        (eventId <= 0x0009U || eventId == 0x000FU) ? "MEDIUM" : "UNKNOWN";
    trans = "Ethernet";

    /* Write to CSV */
    if (csv_file)
    {
        fprintf(csv_file,
            "%d,%s,0x%04X,%s,%s,%s,%s,%d,%s,%s,%s\n",
            event_count, ts, eventId, name, dtc_str,
            event_type, uds_str, occurrences,
            sev, trans, source ? source : "");
        fflush(csv_file);
    }

    /* Write to JSON */
    if (json_file)
    {
        if (!first_json_entry) fprintf(json_file, ",\n");
        fprintf(json_file,
            "  {\n"
            "    \"no\": %d,\n"
            "    \"timestamp\": \"%s\",\n"
            "    \"event_id\": \"0x%04X\",\n"
            "    \"event_name\": \"%s\",\n"
            "    \"dtc\": \"%s\",\n"
            "    \"event_type\": \"%s\",\n"
            "    \"uds_status\": \"%s\",\n"
            "    \"occurrences\": %d,\n"
            "    \"severity\": \"%s\",\n"
            "    \"transport\": \"%s\",\n"
            "    \"source\": \"%s\"\n"
            "  }",
            event_count, ts,
            eventId, name, dtc_str,
            event_type, uds_str, occurrences,
            sev, trans, source ? source : "");
        fflush(json_file);
        first_json_entry = 0;
    }

    /* Write to TXT */
    if (txt_file)
    {
        fprintf(txt_file,
            "%-4d %-20s %-10s %-26s %-10s %-8s %-6d %-8s\n",
            event_count, ts, dtc_str, name,
            event_type, uds_str, occurrences,
            source ? source : "");
        fflush(txt_file);
    }

    printf("[EvtLog] #%d %-9s EventId=0x%04X %-22s DTC=%s\n",
           event_count, event_type, eventId, name, dtc_str);
}

void EvtLog_Close(void)
{
    char ts[32];
    get_ts(ts, sizeof(ts));

    if (csv_file)  { fclose(csv_file);  csv_file  = NULL; }

    if (json_file)
    {
        fprintf(json_file, "\n]\n");
        fclose(json_file);
        json_file = NULL;
    }

    if (txt_file)
    {
        fprintf(txt_file,
            "\n"
            "Session Ended: %s  |  Total Events: %d\n"
            "══════════════════════════════════════════════════════\n",
            ts, event_count);
        fclose(txt_file);
        txt_file = NULL;
    }

    printf("[EvtLog] Closed. Total events logged: %d\n", event_count);
}
