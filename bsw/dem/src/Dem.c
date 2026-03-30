#include "Dem.h"
#include "Dem_Cfg.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/* Private state                                                        */
/* ------------------------------------------------------------------ */
static Dem_StateType            Dem_State = DEM_UNINITIALIZED;
static Dem_EventMemoryEntryType Dem_EventMemory[DEM_MAX_EVENT_MEMORY];
static uint8_t                  Dem_EventMemoryCount = 0U;

/* Per-event runtime table (indexed by EventId - 1) */
static uint8_t  Dem_UdsStatusTable[DEM_NUM_EVENTS];
static uint8_t  Dem_DebounceCounter[DEM_NUM_EVENTS];
static uint8_t  Dem_DebounceTimeCounter[DEM_NUM_EVENTS];
static Dem_EventStatusType Dem_CurrentStatus[DEM_NUM_EVENTS];

/* ------------------------------------------------------------------ */
/* Private helpers                                                      */
/* ------------------------------------------------------------------ */

static uint8_t Dem_GetCfgIndex(Dem_EventIdType EventId)
{
    uint8_t i;
    for (i = 0U; i < DEM_NUM_EVENTS; i++)
    {
        if (Dem_EventCfgTable[i].eventId == EventId) return i;
    }
    return 0xFFU; /* not found */
}

static Dem_EventMemoryEntryType *Dem_FindMemoryEntry(Dem_EventIdType EventId)
{
    uint8_t i;
    for (i = 0U; i < Dem_EventMemoryCount; i++)
    {
        if (Dem_EventMemory[i].eventId == EventId)
            return &Dem_EventMemory[i];
    }
    return NULL_PTR;
}

static Dem_EventMemoryEntryType *Dem_AllocMemoryEntry(Dem_EventIdType EventId,
                                                       uint8_t cfgIdx)
{
    Dem_EventMemoryEntryType *entry = NULL_PTR;

    if (Dem_EventMemoryCount < DEM_MAX_EVENT_MEMORY)
    {
        entry = &Dem_EventMemory[Dem_EventMemoryCount];
        Dem_EventMemoryCount++;
    }
    else
    {
        /* Overflow: displace oldest entry (index 0), shift up */
        uint8_t i;
        for (i = 0U; i < (DEM_MAX_EVENT_MEMORY - 1U); i++)
            Dem_EventMemory[i] = Dem_EventMemory[i + 1U];
        entry = &Dem_EventMemory[DEM_MAX_EVENT_MEMORY - 1U];
    }

    memset(entry, 0, sizeof(Dem_EventMemoryEntryType));
    entry->eventId = EventId;
    entry->dtc     = Dem_EventCfgTable[cfgIdx].dtc;
    return entry;
}

static void Dem_UpdateUdsStatus(uint8_t cfgIdx,
                                 Dem_EventStatusType newStatus)
{
    uint8_t *uds = &Dem_UdsStatusTable[cfgIdx];

    if (newStatus == DEM_EVENT_STATUS_FAILED)
    {
        *uds |= DEM_UDS_STATUS_TF;
        *uds |= DEM_UDS_STATUS_TFTOC;
        *uds |= DEM_UDS_STATUS_PDTC;
        *uds |= DEM_UDS_STATUS_CDTC;
        *uds |= DEM_UDS_STATUS_TFSLC;
        *uds &= (uint8_t)~DEM_UDS_STATUS_TNCTOC;
    }
    else if (newStatus == DEM_EVENT_STATUS_PASSED)
    {
        *uds &= (uint8_t)~DEM_UDS_STATUS_TF;
        *uds &= (uint8_t)~DEM_UDS_STATUS_TFTOC;
        *uds &= (uint8_t)~DEM_UDS_STATUS_PDTC;
        *uds &= (uint8_t)~DEM_UDS_STATUS_TNCTOC;
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

void Dem_Init(void)
{
    memset(Dem_EventMemory,       0, sizeof(Dem_EventMemory));
    memset(Dem_UdsStatusTable,    0, sizeof(Dem_UdsStatusTable));
    memset(Dem_DebounceCounter,   0, sizeof(Dem_DebounceCounter));
    memset(Dem_DebounceTimeCounter, 0, sizeof(Dem_DebounceTimeCounter));
    memset(Dem_CurrentStatus,     0, sizeof(Dem_CurrentStatus));

    Dem_EventMemoryCount = 0U;
    Dem_State            = DEM_INITIALIZED;

    printf("[DEM] Initialized - %u event slots, %u events configured\n",
           DEM_MAX_EVENT_MEMORY, DEM_NUM_EVENTS);
}

void Dem_MainFunction(void)
{
    uint8_t i;
    if (Dem_State != DEM_INITIALIZED) return;

    /* Increment time-based debounce counters for PREFAILED events */
    for (i = 0U; i < DEM_NUM_EVENTS; i++)
    {
        if ((Dem_EventCfgTable[i].debounceType == DEM_DEBOUNCE_TIME) &&
            (Dem_CurrentStatus[i] == DEM_EVENT_STATUS_PREFAILED))
        {
            Dem_DebounceTimeCounter[i]++;
            if (Dem_DebounceTimeCounter[i] >= DEM_DEBOUNCE_COUNTER_THRESHOLD)
            {
                /* Promote to FAILED */
                (void)Dem_SetEventStatus(Dem_EventCfgTable[i].eventId,
                                         DEM_EVENT_STATUS_FAILED);
                Dem_DebounceTimeCounter[i] = 0U;
            }
        }
    }
}

Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId,
                                   Dem_EventStatusType EventStatus)
{
    uint8_t cfgIdx;
    Dem_EventMemoryEntryType *entry;

    if (Dem_State != DEM_INITIALIZED)
    {
        printf("[DEM] ERROR: Not initialized\n");
        return E_NOT_OK;
    }

    if ((EventId < DEM_FIRST_EVENT_ID) || (EventId > DEM_LAST_EVENT_ID))
    {
        printf("[DEM] ERROR: Invalid EventId %d\n", EventId);
        return E_NOT_OK;
    }

    cfgIdx = Dem_GetCfgIndex(EventId);
    if (cfgIdx == 0xFFU) return E_NOT_OK;

    /* Counter-based debounce */
    if (Dem_EventCfgTable[cfgIdx].debounceType == DEM_DEBOUNCE_COUNTER)
    {
        if (EventStatus == DEM_EVENT_STATUS_PREFAILED)
        {
            Dem_DebounceCounter[cfgIdx]++;
            if (Dem_DebounceCounter[cfgIdx] < DEM_DEBOUNCE_COUNTER_THRESHOLD)
            {
                Dem_CurrentStatus[cfgIdx] = DEM_EVENT_STATUS_PREFAILED;
                return E_OK; /* not yet confirmed */
            }
            EventStatus = DEM_EVENT_STATUS_FAILED; /* promote */
        }
        else if (EventStatus == DEM_EVENT_STATUS_PASSED)
        {
            Dem_DebounceCounter[cfgIdx] = 0U;
        }
    }

    /* Time-based debounce - just store PREFAILED, MainFunction promotes */
    if ((Dem_EventCfgTable[cfgIdx].debounceType == DEM_DEBOUNCE_TIME) &&
        (EventStatus == DEM_EVENT_STATUS_PREFAILED))
    {
        Dem_CurrentStatus[cfgIdx] = DEM_EVENT_STATUS_PREFAILED;
        return E_OK;
    }

    Dem_CurrentStatus[cfgIdx] = EventStatus;
    Dem_UpdateUdsStatus(cfgIdx, EventStatus);

    /* Store in event memory on FAILED */
    if (EventStatus == DEM_EVENT_STATUS_FAILED)
    {
        entry = Dem_FindMemoryEntry(EventId);
        if (entry == NULL_PTR)
        {
            entry = Dem_AllocMemoryEntry(EventId, cfgIdx);
        }
        if (entry != NULL_PTR)
        {
            entry->udsStatusByte = Dem_UdsStatusTable[cfgIdx];
            entry->occurrenceCounter++;
            entry->currentStatus = EventStatus;
            printf("[DEM] Event 0x%04X FAILED - DTC=0x%06X occurrences=%d\n",
                   EventId,
                   Dem_EventCfgTable[cfgIdx].dtc,
                   entry->occurrenceCounter);
        }
    }
    else if (EventStatus == DEM_EVENT_STATUS_PASSED)
    {
        entry = Dem_FindMemoryEntry(EventId);
        if (entry != NULL_PTR)
        {
            entry->udsStatusByte  = Dem_UdsStatusTable[cfgIdx];
            entry->currentStatus  = EventStatus;
            entry->agingCounter++;
            printf("[DEM] Event 0x%04X PASSED - aging=%d\n",
                   EventId, entry->agingCounter);
        }
    }

    return E_OK;
}

Std_ReturnType Dem_GetEventStatus(Dem_EventIdType EventId,
                                   Dem_EventStatusType *EventStatusByte)
{
    uint8_t cfgIdx;

    if (Dem_State != DEM_INITIALIZED) return E_NOT_OK;
    if (EventStatusByte == NULL_PTR)  return E_NOT_OK;
    if ((EventId < DEM_FIRST_EVENT_ID) || (EventId > DEM_LAST_EVENT_ID))
        return E_NOT_OK;

    cfgIdx = Dem_GetCfgIndex(EventId);
    if (cfgIdx == 0xFFU) return E_NOT_OK;

    *EventStatusByte = Dem_UdsStatusTable[cfgIdx];
    return E_OK;
}

Std_ReturnType Dem_ReportErrorStatus(Dem_EventIdType EventId,
                                      Dem_EventStatusType EventStatus)
{
    /* AUTOSAR legacy wrapper - forwards to SetEventStatus */
    return Dem_SetEventStatus(EventId, EventStatus);
}

Std_ReturnType Dem_ClearDTC(Dem_DTCType DTC)
{
    uint8_t i;
    uint8_t cfgIdx;

    if (Dem_State != DEM_INITIALIZED) return E_NOT_OK;

    if (DTC == DEM_DTC_GROUP_ALL)
    {
        /* Clear all */
        memset(Dem_EventMemory,    0, sizeof(Dem_EventMemory));
        memset(Dem_UdsStatusTable, 0, sizeof(Dem_UdsStatusTable));
        memset(Dem_DebounceCounter, 0, sizeof(Dem_DebounceCounter));
        memset(Dem_CurrentStatus,  0, sizeof(Dem_CurrentStatus));
        Dem_EventMemoryCount = 0U;
        printf("[DEM] ClearDTC - ALL DTCs cleared\n");
        return E_OK;
    }

    /* Clear specific DTC */
    for (i = 0U; i < DEM_NUM_EVENTS; i++)
    {
        if (Dem_EventCfgTable[i].dtc == DTC)
        {
            cfgIdx = i;
            Dem_UdsStatusTable[cfgIdx]   = 0U;
            Dem_DebounceCounter[cfgIdx]  = 0U;
            Dem_CurrentStatus[cfgIdx]    = DEM_EVENT_STATUS_PASSED;

            /* Remove from event memory */
            uint8_t j;
            for (j = 0U; j < Dem_EventMemoryCount; j++)
            {
                if (Dem_EventMemory[j].eventId ==
                    Dem_EventCfgTable[cfgIdx].eventId)
                {
                    /* Shift remaining entries down */
                    uint8_t k;
                    for (k = j; k < (Dem_EventMemoryCount - 1U); k++)
                        Dem_EventMemory[k] = Dem_EventMemory[k + 1U];
                    Dem_EventMemoryCount--;
                    break;
                }
            }
            printf("[DEM] ClearDTC - DTC 0x%06X cleared\n", DTC);
            return E_OK;
        }
    }
    return E_NOT_OK;
}

Std_ReturnType Dem_SetDTCFilter(Dem_DTCStatusMaskType StatusMask,
                                 Dem_FilterType *Filter)
{
    if (Filter == NULL_PTR) return E_NOT_OK;
    Filter->statusMask    = StatusMask;
    Filter->currentIndex  = 0U;
    return E_OK;
}

Std_ReturnType Dem_GetNextFilteredDTC(Dem_FilterType *Filter,
                                       Dem_DTCType *DTC,
                                       Dem_DTCStatusMaskType *DTCStatus)
{
    uint8_t cfgIdx;

    if ((Filter == NULL_PTR) || (DTC == NULL_PTR) || (DTCStatus == NULL_PTR))
        return E_NOT_OK;

    while (Filter->currentIndex < DEM_NUM_EVENTS)
    {
        cfgIdx = Filter->currentIndex;
        Filter->currentIndex++;

        if ((Dem_UdsStatusTable[cfgIdx] & Filter->statusMask) != 0U)
        {
            *DTC       = Dem_EventCfgTable[cfgIdx].dtc;
            *DTCStatus = Dem_UdsStatusTable[cfgIdx];
            return E_OK;
        }
    }
    return E_NOT_OK; /* no more matching DTCs */
}

Std_ReturnType Dem_GetNumberOfFilteredDTC(Dem_FilterType *Filter,
                                           uint16_t *NumberOfFilteredDTC)
{
    uint8_t  i;
    uint16_t count = 0U;

    if ((Filter == NULL_PTR) || (NumberOfFilteredDTC == NULL_PTR))
        return E_NOT_OK;

    for (i = 0U; i < DEM_NUM_EVENTS; i++)
    {
        if ((Dem_UdsStatusTable[i] & Filter->statusMask) != 0U)
            count++;
    }
    *NumberOfFilteredDTC = count;
    return E_OK;
}

#include "NvM.h"

void Dem_NvM_StoreEventMemory(void)
{
    uint8_t  buffer[NVM_MAX_BLOCK_SIZE];
    uint16_t offset = 0U;
    uint8_t  i;

    buffer[offset++] = Dem_EventMemoryCount;
    for (i = 0U; i < Dem_EventMemoryCount; i++)
    {
        buffer[offset++] = (uint8_t)(Dem_EventMemory[i].eventId & 0xFFU);
        buffer[offset++] = (uint8_t)((Dem_EventMemory[i].dtc >> 16U) & 0xFFU);
        buffer[offset++] = (uint8_t)((Dem_EventMemory[i].dtc >>  8U) & 0xFFU);
        buffer[offset++] = (uint8_t)( Dem_EventMemory[i].dtc         & 0xFFU);
        buffer[offset++] = Dem_EventMemory[i].udsStatusByte;
        buffer[offset++] = Dem_EventMemory[i].occurrenceCounter;
        if (offset >= (uint16_t)(NVM_MAX_BLOCK_SIZE - 6U)) break;
    }
    NvM_WriteBlock(NVM_BLOCK_DEM_PRIMARY, buffer, offset);
    printf("[DEM] Event memory stored (%d entries)\n", Dem_EventMemoryCount);
}

void Dem_NvM_RestoreEventMemory(void)
{
    uint8_t  buffer[NVM_MAX_BLOCK_SIZE];
    uint16_t offset = 0U;
    uint8_t  count;
    uint8_t  i;
    uint8_t  cfgIdx;

    if (NvM_ReadBlock(NVM_BLOCK_DEM_PRIMARY, buffer, NVM_MAX_BLOCK_SIZE) != E_OK)
        return;

    count = buffer[offset++];
    if (count > DEM_MAX_EVENT_MEMORY) count = DEM_MAX_EVENT_MEMORY;

    for (i = 0U; i < count; i++)
    {
        uint16_t eventId = buffer[offset++];
        uint32_t dtc     = ((uint32_t)buffer[offset]   << 16U) |
                           ((uint32_t)buffer[offset+1] <<  8U) |
                            (uint32_t)buffer[offset+2];
        offset += 3U;
        uint8_t uds_byte = buffer[offset++];
        uint8_t occ      = buffer[offset++];

        Dem_EventMemory[i].eventId           = eventId;
        Dem_EventMemory[i].dtc               = dtc;
        Dem_EventMemory[i].udsStatusByte     = uds_byte;
        Dem_EventMemory[i].occurrenceCounter = occ;
        Dem_EventMemory[i].currentStatus     = DEM_EVENT_STATUS_FAILED;

        cfgIdx = Dem_GetCfgIndex((Dem_EventIdType)eventId);
        if (cfgIdx != 0xFFU)
            Dem_UdsStatusTable[cfgIdx] = uds_byte;

        Dem_EventMemoryCount++;
        printf("[DEM] Restored DTC=0x%06X occurrences=%d\n", dtc, occ);
    }
}
