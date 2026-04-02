#ifndef KAVACH_CONDITIONS_H
#define KAVACH_CONDITIONS_H

#include "Std_Types.h"
#include "Dem.h"
#include "Dem_EventConfig.h"

typedef struct {
    uint8_t  actual_speed_kmh;
    uint8_t  permitted_speed_kmh;
    uint8_t  signal_aspect;        /* 0=RED 1=YELLOW 2=GREEN */
    uint8_t  radio_ok;
    uint8_t  direction_forward;    /* 1=forward 0=reverse */
    uint8_t  brake_pressure_bar;
    uint8_t  current_mode;
    uint8_t  sos_button_pressed;   /* 1=driver pressed SOS */
    uint8_t  rfid_tag_read;        /* 1=tag read OK 0=missed */
    uint8_t  rfid_location_valid;  /* 1=expected 0=unexpected */
    uint32_t last_radio_rx_ms;
    uint32_t current_time_ms;
} Kavach_LiveDataType;

extern Kavach_LiveDataType Kavach_LiveData;

void Kavach_EvalConditions(void);
void Kavach_SetSpeed(uint8_t actual, uint8_t permitted);
void Kavach_SetSignal(uint8_t aspect);
void Kavach_SetRadio(uint8_t ok);
void Kavach_SetMode(uint8_t mode);
void Kavach_SetBrake(uint8_t pressure_bar);
void Kavach_SetSOS(uint8_t button_pressed);
void Kavach_SetRFID(uint8_t tag_read, uint8_t location_valid);
void Kavach_SetSignalAspect(uint8_t aspect);

#endif
