#ifndef RMW_ZENOH_TRANSPORT_H
#define RMW_ZENOH_TRANSPORT_H

#define RMW_ZENOH_PICO_TRANSPORT_MULTICAST

#include "rmw_zenoh_pico/rmw_zenoh_pico_options.h"

#include "rmw_zenoh_common.h"

extern bool set_rmw_zenoh_mcast_transports(const char *mcast_ip,
					   unsigned int mcast_port,
					   const char *mcast_device);
#endif
