#include "Kavach_Conditions.h"
#include "Dem.h"
#include "EvtLog.h"
#include <stdio.h>
#include <string.h>

Kavach_LiveDataType Kavach_LiveData = {0};
static uint8_t prev_failed[0x00C0U] = {0};

static void set_event(Dem_EventIdType id, Dem_DTCType dtc,
                      uint8_t condition_met, const char *name)
{
    Dem_EventStatusType uds = 0;
    if (condition_met)
    {
        Dem_SetEventStatus(id, DEM_EVENT_STATUS_FAILED);
        Dem_GetEventStatus(id, &uds);
        if (!prev_failed[id])
        {
            printf("[COND] FAILED: %s\n", name);
            EvtLog_Write(id, dtc, "FAILED", uds, 1U, "Kavach_Conditions");
        }
        prev_failed[id] = 1U;
    }
    else
    {
        Dem_SetEventStatus(id, DEM_EVENT_STATUS_PASSED);
        if (prev_failed[id])
        {
            Dem_GetEventStatus(id, &uds);
            printf("[COND] PASSED: %s\n", name);
            EvtLog_Write(id, dtc, "PASSED", uds, 0U, "Kavach_Conditions");
        }
        prev_failed[id] = 0U;
    }
}

void Kavach_EvalConditions(void)
{
    uint32_t radio_silent_ms;

    /* Over Speeding */
    set_event(KAVACH_EVT_OVERSPEED, 0x00A101U,
              Kavach_LiveData.actual_speed_kmh >
              Kavach_LiveData.permitted_speed_kmh,
              "Over_Speeding");

    /* SPAD: passed red signal while moving */
    set_event(KAVACH_EVT_SPAD, 0x00A201U,
              (Kavach_LiveData.signal_aspect == 0U) &&
              (Kavach_LiveData.actual_speed_kmh > 0U),
              "SPAD");

    /* SOS: driver pressed button */
    set_event(KAVACH_EVT_SOS, 0x00A301U,
              Kavach_LiveData.sos_button_pressed == 1U,
              "SOS_Received");

    /* Roll Back */
    set_event(KAVACH_EVT_ROLLBACK, 0x00A401U,
              (Kavach_LiveData.direction_forward == 0U) &&
              (Kavach_LiveData.actual_speed_kmh > 0U),
              "Roll_Back");

    /* Radio Loss: no packet for more than 5000ms */
    radio_silent_ms = Kavach_LiveData.current_time_ms
                    - Kavach_LiveData.last_radio_rx_ms;
    set_event(KAVACH_EVT_RADIO_LOSS, 0x00A501U,
              radio_silent_ms > 5000U,
              "Radio_Loss");

    /* Brake Command: low pressure while moving */
    set_event(KAVACH_EVT_BRAKE_CMD, 0x00A601U,
              (Kavach_LiveData.brake_pressure_bar < 2U) &&
              (Kavach_LiveData.actual_speed_kmh > 0U),
              "Brake_Command");

    /* RFID: tag not read OR read at wrong location */
    set_event(KAVACH_EVT_RFID, 0x00B101U,
              (Kavach_LiveData.rfid_tag_read == 0U) ||
              (Kavach_LiveData.rfid_location_valid == 0U),
              "RFID_Tag_Miss_Or_Invalid");

    /* Signal: RED aspect while moving */
    set_event(KAVACH_EVT_SIG, 0x00B201U,
              (Kavach_LiveData.signal_aspect == 0U) &&
              (Kavach_LiveData.actual_speed_kmh > 0U),
              "Signal_RED_While_Moving");

    /* Mode: Trip */
    set_event(KAVACH_EVT_MODE_TR, 0x010701U,
              Kavach_LiveData.current_mode == 0x07U,
              "Mode_Trip");

    /* Mode: System Failure */
    set_event(KAVACH_EVT_MODE_SF, 0x010F01U,
              Kavach_LiveData.current_mode == 0x0FU,
              "Mode_SystemFailure");
}

void Kavach_SetSpeed(uint8_t actual, uint8_t permitted)
{
    Kavach_LiveData.actual_speed_kmh    = actual;
    Kavach_LiveData.permitted_speed_kmh = permitted;
}

void Kavach_SetSignal(uint8_t aspect)
{
    Kavach_LiveData.signal_aspect = aspect;
}

void Kavach_SetSignalAspect(uint8_t aspect)
{
    const char *names[] = {"RED","YELLOW","GREEN"};
    Kavach_LiveData.signal_aspect = aspect;
    printf("[KAVACH] Signal: %s\n", aspect < 3U ? names[aspect] : "UNKNOWN");
}

void Kavach_SetRadio(uint8_t ok)
{
    Kavach_LiveData.radio_ok = ok;
    if (ok) Kavach_LiveData.last_radio_rx_ms = Kavach_LiveData.current_time_ms;
}

void Kavach_SetMode(uint8_t mode)
{
    Kavach_LiveData.current_mode = mode;
}

void Kavach_SetBrake(uint8_t pressure_bar)
{
    Kavach_LiveData.brake_pressure_bar = pressure_bar;
}

void Kavach_SetSOS(uint8_t button_pressed)
{
    Kavach_LiveData.sos_button_pressed = button_pressed;
    if (button_pressed)
        printf("[KAVACH] SOS button pressed\n");
}

void Kavach_SetRFID(uint8_t tag_read, uint8_t location_valid)
{
    Kavach_LiveData.rfid_tag_read       = tag_read;
    Kavach_LiveData.rfid_location_valid = location_valid;
    if (!tag_read)
        printf("[KAVACH] RFID: tag NOT read\n");
    else if (!location_valid)
        printf("[KAVACH] RFID: tag at UNEXPECTED location\n");
}
