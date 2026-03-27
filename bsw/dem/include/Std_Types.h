#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef int8_t    sint8;
typedef int16_t   sint16;
typedef int32_t   sint32;
typedef uint8_t   boolean;
typedef uint8_t   Std_ReturnType;

#define E_OK        ((Std_ReturnType)0x00U)
#define E_NOT_OK    ((Std_ReturnType)0x01U)
#define TRUE        (1U)
#define FALSE       (0U)
#define NULL_PTR    ((void *)0)

typedef uint16_t  Dem_EventIdType;
typedef uint8_t   Dem_EventStatusType;

#define DEM_EVENT_STATUS_PASSED    ((Dem_EventStatusType)0x00U)
#define DEM_EVENT_STATUS_FAILED    ((Dem_EventStatusType)0x01U)
#define DEM_EVENT_STATUS_PREPASSED ((Dem_EventStatusType)0x02U)
#define DEM_EVENT_STATUS_PREFAILED ((Dem_EventStatusType)0x03U)

#endif /* STD_TYPES_H */
