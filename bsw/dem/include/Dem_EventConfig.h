#ifndef DEM_EVENT_CONFIG_H
#define DEM_EVENT_CONFIG_H

#include "Std_Types.h"

#define RAIL_EVT_BRAKE_SENSORLOSS    ((Dem_EventIdType)0x0001U)
#define RAIL_EVT_DOOR_LOCKFAIL       ((Dem_EventIdType)0x0002U)
#define RAIL_EVT_TEMP_OVERTEMP       ((Dem_EventIdType)0x0003U)
#define RAIL_EVT_MOTOR_OVERCURRENT   ((Dem_EventIdType)0x0004U)
#define RAIL_EVT_CAN_TIMEOUT         ((Dem_EventIdType)0x0005U)
#define RAIL_EVT_SIGNAL_LOSS         ((Dem_EventIdType)0x0006U)
#define RAIL_EVT_HVAC_FAIL           ((Dem_EventIdType)0x0007U)
#define RAIL_EVT_POWER_UNDERVOLT     ((Dem_EventIdType)0x0008U)
#define RAIL_EVT_BRAKE_PRESSURE_LOW  ((Dem_EventIdType)0x0009U)
#define RAIL_EVT_DOOR_OPEN_SPEED     ((Dem_EventIdType)0x000AU)
#define RAIL_EVT_MOTOR_STALL         ((Dem_EventIdType)0x000BU)
#define RAIL_EVT_CAN_BUS_OFF         ((Dem_EventIdType)0x000CU)
#define RAIL_EVT_ETH_LINK_DOWN       ((Dem_EventIdType)0x000DU)
#define RAIL_EVT_ETH_TIMEOUT         ((Dem_EventIdType)0x000EU)
#define RAIL_EVT_SPEED_SENSOR_FAIL   ((Dem_EventIdType)0x000FU)

#define DEM_NUM_EVENTS               (15U)
#define DEM_FIRST_EVENT_ID           (0x0001U)
#define DEM_LAST_EVENT_ID            (0x000FU)

#endif /* DEM_EVENT_CONFIG_H */
