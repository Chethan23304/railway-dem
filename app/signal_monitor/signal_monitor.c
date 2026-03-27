#include "signal_monitor.h"
#include "Dem_EventConfig.h"
#include "Dem.h"

static uint8 sim_signal_lost  = FALSE;
static uint8 sim_eth_link_down = FALSE;
static uint8 sim_eth_timeout  = FALSE;

void SignalMonitor_Init(void)
{
    sim_signal_lost   = FALSE;
    sim_eth_link_down = FALSE;
    sim_eth_timeout   = FALSE;
}

void SignalMonitor_SetSimulatedValues(uint8 signal_lost, uint8 eth_link_down,
                                      uint8 eth_timeout)
{
    sim_signal_lost   = signal_lost;
    sim_eth_link_down = eth_link_down;
    sim_eth_timeout   = eth_timeout;
}

void SignalMonitor_MainFunction(void)
{
    Dem_EventStatusType signal_status;
    Dem_EventStatusType eth_link_status;
    Dem_EventStatusType eth_timeout_status;

    signal_status = (sim_signal_lost == TRUE)
                    ? DEM_EVENT_STATUS_PREFAILED
                    : DEM_EVENT_STATUS_PASSED;

    eth_link_status = (sim_eth_link_down == TRUE)
                      ? DEM_EVENT_STATUS_FAILED
                      : DEM_EVENT_STATUS_PASSED;

    eth_timeout_status = (sim_eth_timeout == TRUE)
                         ? DEM_EVENT_STATUS_PREFAILED
                         : DEM_EVENT_STATUS_PASSED;

    (void)Dem_SetEventStatus(RAIL_EVT_SIGNAL_LOSS,    signal_status);
    (void)Dem_SetEventStatus(RAIL_EVT_ETH_LINK_DOWN,  eth_link_status);
    (void)Dem_SetEventStatus(RAIL_EVT_ETH_TIMEOUT,    eth_timeout_status);
}