#ifndef DEM_LOGGER_H
#define DEM_LOGGER_H

#include "Std_Types.h"
#include "Dem.h"

#define DEM_LOG_DB_PATH   "logs/dem_events.db"
#define DEM_LOG_TXT_PATH  "logs/dem_events.log"

typedef enum {
    LOG_EVENT_FAILED   = 0U,
    LOG_EVENT_PASSED   = 1U,
    LOG_EVENT_CLEARED  = 2U,
    LOG_EVENT_RESTORED = 3U,
    LOG_EVENT_KAVACH   = 4U,
} DemLog_EventType;

void DemLog_Init(void);
void DemLog_Write(Dem_EventIdType eventId,
                  Dem_DTCType dtc,
                  DemLog_EventType logEvent,
                  uint8_t udsStatus,
                  uint8_t occurrences,
                  const char *source);
void DemLog_WriteKavach(uint32_t kavach_msg_id,
                        const uint8_t *payload,
                        uint8_t len,
                        const char *direction);
void DemLog_Close(void);

#endif
