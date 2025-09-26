#ifndef RMW_ZENOH_TRANSPORT_H
#define RMW_ZENOH_TRANSPORT_H

#define RMW_ZENOH_PICO_TRANSPORT_SERIAL

#include "rmw_zenoh_pico/rmw_zenoh_pico_options.h"

#include "rmw_zenoh_common.h"

extern bool set_rmw_zenoh_serial_transports(const char *serial_device);

#endif
