#ifndef EVENT_FILE_LOGGER_H
#define EVENT_FILE_LOGGER_H

#include "Std_Types.h"
#include "Dem.h"

#define EVT_LOG_PATH   "logs/events.csv"
#define EVT_LOG_JSON   "logs/events.json"
#define EVT_LOG_TXT    "logs/events.txt"

void EvtLog_Init(void);
void EvtLog_Write(Dem_EventIdType eventId,
                  Dem_DTCType     dtc,
                  const char     *event_type,
                  uint8_t         uds_status,
                  uint8_t         occurrences,
                  const char     *source);
void EvtLog_Close(void);

#endif
