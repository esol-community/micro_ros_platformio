#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <Arduino.h>

#include "rmw_zenoh_transport.h"

static char _mcast_ip[32];
static char _mcast_port[16];

static char _mcast_device[32];

bool set_rmw_zenoh_mcast_transports(const char *mcast_ip,
                                    unsigned int mcast_port,
                                    const char *mcast_device) {
  // clear option value
  memset(_mcast_ip, 0, sizeof(_mcast_ip));
  memset(_mcast_port, 0, sizeof(_mcast_port));
  memset(_mcast_device, 0, sizeof(_mcast_device));

  // set option value
  snprintf(_mcast_ip, sizeof(_mcast_ip) -1, "%s", mcast_ip);
  snprintf(_mcast_port, sizeof(_mcast_port) -1, "%d", mcast_port);
  snprintf(_mcast_device, sizeof(_mcast_device) -1, "%s", mcast_device);

  // set option to rmw_zenoh_pico
  rmw_zenoh_pico_set_mcast(_mcast_ip, _mcast_port, _mcast_device);

  return true;
}
