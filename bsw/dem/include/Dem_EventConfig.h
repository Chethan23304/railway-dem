#ifndef DEM_EVENT_CONFIG_H
#define DEM_EVENT_CONFIG_H

#include "Std_Types.h"

/* ================================================================
   Kavach DMI Simulator - Event IDs
   ================================================================ */

/* Kavach Operating Modes */
#define KAVACH_EVT_MODE_SB      ((Dem_EventIdType)0x0001U)  /* Stand By */
#define KAVACH_EVT_MODE_FS      ((Dem_EventIdType)0x0002U)  /* Full Supervision */
#define KAVACH_EVT_MODE_LS      ((Dem_EventIdType)0x0003U)  /* Limited Supervision */
#define KAVACH_EVT_MODE_SR      ((Dem_EventIdType)0x0004U)  /* Staff Responsible */
#define KAVACH_EVT_MODE_SH      ((Dem_EventIdType)0x0005U)  /* Shunting */
#define KAVACH_EVT_MODE_OS      ((Dem_EventIdType)0x0006U)  /* On Sight */
#define KAVACH_EVT_MODE_TR      ((Dem_EventIdType)0x0007U)  /* Trip */
#define KAVACH_EVT_MODE_PT      ((Dem_EventIdType)0x0008U)  /* Post Trip */
#define KAVACH_EVT_MODE_RV      ((Dem_EventIdType)0x0009U)  /* Reverse */
#define KAVACH_EVT_MODE_SF      ((Dem_EventIdType)0x000FU)  /* System Failure */

/* Kavach Critical Safety Events - must log immediately */
#define KAVACH_EVT_OVERSPEED    ((Dem_EventIdType)0x00A1U)  /* Over Speeding */
#define KAVACH_EVT_SPAD         ((Dem_EventIdType)0x00A2U)  /* Signal Passed At Danger */
#define KAVACH_EVT_SOS          ((Dem_EventIdType)0x00A3U)  /* SOS Received */
#define KAVACH_EVT_ROLLBACK     ((Dem_EventIdType)0x00A4U)  /* Roll Back */
#define KAVACH_EVT_RADIO_LOSS   ((Dem_EventIdType)0x00A5U)  /* Radio Loss */
#define KAVACH_EVT_BRAKE_CMD    ((Dem_EventIdType)0x00A6U)  /* Brake Command */

/* Kavach Info Events */
#define KAVACH_EVT_RFID         ((Dem_EventIdType)0x00B1U)  /* Tag Read */
#define KAVACH_EVT_SIG          ((Dem_EventIdType)0x00B2U)  /* Aspect Change */
#define KAVACH_EVT_MA           ((Dem_EventIdType)0x00B3U)  /* MA Update */

/* Total counts */
#define DEM_NUM_MODE_EVENTS      (10U)
#define DEM_NUM_CRITICAL_EVENTS  (6U)
#define DEM_NUM_INFO_EVENTS      (3U)
#define DEM_NUM_EVENTS           (19U)

#define DEM_FIRST_EVENT_ID       (0x0001U)
#define DEM_LAST_EVENT_ID        (0x00B3U)

#endif /* DEM_EVENT_CONFIG_H */
