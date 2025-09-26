#ifndef RMW_ZENOH_TRANSPORT_H
#define RMW_ZENOH_TRANSPORT_H

#define RMW_ZENOH_PICO_TRANSPORT_UNICAST

#include "rmw_zenoh_pico/rmw_zenoh_pico_options.h"

#include "rmw_zenoh_common.h"

extern bool set_rmw_zenoh_unicast_transports(const char *scout_ip, int scout_port,
					     const char *listen_ip, int listen_port);

#endif
