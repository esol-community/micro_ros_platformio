#include <stdio.h>
#include <stdbool.h>

#include <Arduino.h>

#include "rmw_zenoh_transport.h"

static char _serial_device[32];

bool set_rmw_zenoh_serial_transports(const char *serial_device) {

  // clear option value
  memset(_serial_device, 0, sizeof(_serial_device));

  // set option value
  snprintf(_serial_device, sizeof(_serial_device) -1, "%s", serial_device);

  // set option to rmw_zenoh_pico
  rmw_zenoh_pico_set_serial_device(_serial_device);

  return true;
}
