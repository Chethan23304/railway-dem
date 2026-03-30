#ifndef DEM_H
#define DEM_H

#include "Std_Types.h"
#include "Dem_EventConfig.h"

/* DEM module state */
typedef uint8_t Dem_StateType;
#define DEM_UNINITIALIZED  ((Dem_StateType)0x00U)
#define DEM_INITIALIZED    ((Dem_StateType)0x01U)

/* DTC format */
typedef uint32_t Dem_DTCType;
typedef uint8_t  Dem_DTCStatusMaskType;

/* UDS Status byte bits */
#define DEM_UDS_STATUS_TF      (0x01U)  /* Bit 0: testFailed */
#define DEM_UDS_STATUS_TFTOC   (0x02U)  /* Bit 1: testFailedThisOperationCycle */
#define DEM_UDS_STATUS_PDTC    (0x04U)  /* Bit 2: pendingDTC */
#define DEM_UDS_STATUS_CDTC    (0x08U)  /* Bit 3: confirmedDTC */
#define DEM_UDS_STATUS_TNCSLC  (0x10U)  /* Bit 4: testNotCompletedSinceLastClear */
#define DEM_UDS_STATUS_TFSLC   (0x20U)  /* Bit 5: testFailedSinceLastClear */
#define DEM_UDS_STATUS_TNCTOC  (0x40U)  /* Bit 6: testNotCompletedThisOperationCycle */
#define DEM_UDS_STATUS_WIR     (0x80U)  /* Bit 7: warningIndicatorRequested */

/* DTC group for ClearDTC */
#define DEM_DTC_GROUP_ALL      (0xFFFFFFU)

/* Event memory size */
#define DEM_MAX_EVENT_MEMORY   (8U)

/* Debounce thresholds */
#define DEM_DEBOUNCE_COUNTER_THRESHOLD  (3U)
#define DEM_AGED_THRESHOLD              (10U)

/* DTC config entry - one per fault event */
typedef struct {
    Dem_EventIdType  eventId;
    Dem_DTCType      dtc;
    uint8_t          severity;      /* 0=LOW 1=MEDIUM 2=HIGH */
    uint8_t          debounceType;  /* 0=counter 1=time */
} Dem_EventCfgType;

/* DTC severity levels */
#define DEM_SEVERITY_LOW     (0U)
#define DEM_SEVERITY_MEDIUM  (1U)
#define DEM_SEVERITY_HIGH    (2U)

/* Debounce types */
#define DEM_DEBOUNCE_COUNTER (0U)
#define DEM_DEBOUNCE_TIME    (1U)

/* Internal event entry stored in event memory */
typedef struct {
    Dem_EventIdType       eventId;
    Dem_DTCType           dtc;
    uint8_t               udsStatusByte;
    uint8_t               occurrenceCounter;
    uint8_t               agingCounter;
    uint8_t               debounceCounter;
    Dem_EventStatusType   currentStatus;
    uint8_t               freezeFrame[4];  /* speed, door, brake, temp */
} Dem_EventMemoryEntryType;

/* DTC filter for DCM/UDS 0x19 */
typedef struct {
    Dem_DTCStatusMaskType statusMask;
    uint8_t               currentIndex;
} Dem_FilterType;

/* Public API */
void           Dem_Init(void);
void           Dem_MainFunction(void);
void Dem_NvM_StoreEventMemory(void);
void Dem_NvM_RestoreEventMemory(void);

Std_ReturnType Dem_SetEventStatus(Dem_EventIdType EventId,
                                   Dem_EventStatusType EventStatus);

Std_ReturnType Dem_GetEventStatus(Dem_EventIdType EventId,
                                   Dem_EventStatusType *EventStatusByte);

Std_ReturnType Dem_ReportErrorStatus(Dem_EventIdType EventId,
                                      Dem_EventStatusType EventStatus);

Std_ReturnType Dem_ClearDTC(Dem_DTCType DTC);

Std_ReturnType Dem_SetDTCFilter(Dem_DTCStatusMaskType StatusMask,
                                 Dem_FilterType *Filter);

Std_ReturnType Dem_GetNextFilteredDTC(Dem_FilterType *Filter,
                                       Dem_DTCType *DTC,
                                       Dem_DTCStatusMaskType *DTCStatus);

Std_ReturnType Dem_GetNumberOfFilteredDTC(Dem_FilterType *Filter,
                                           uint16_t *NumberOfFilteredDTC);

#endif /* DEM_H */