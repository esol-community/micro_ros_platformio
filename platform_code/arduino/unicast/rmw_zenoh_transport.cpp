#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#include <Arduino.h>

#include "rmw_zenoh_transport.h"

static char _scout_addr[32];
static char _scout_port[16];

static char _listen_addr[32];
static char _listen_port[16];

bool set_rmw_zenoh_unicast_transports(const char *scout_ip, int scout_port,
                                      const char *listen_ip, int listen_port) {

  // clear option value
  memset(_scout_addr, 0, sizeof(_scout_addr));
  memset(_scout_port, 0, sizeof(_scout_port));
  memset(_listen_addr, 0, sizeof(_listen_addr));
  memset(_listen_port, 0, sizeof(_listen_port));

  // set option value
  snprintf(_scout_addr, sizeof(_scout_addr) -1, "%s", scout_ip);
  snprintf(_scout_port, sizeof(_scout_port) -1, "%d", scout_port);
  snprintf(_listen_addr, sizeof(_listen_addr) -1, "%s", listen_ip);
  snprintf(_listen_port, sizeof(_listen_port) -1, "%d", listen_port);

  // set option to rmw_zenoh_pico
  rmw_zenoh_pico_set_unicast(_scout_addr, _scout_port, _listen_addr, _listen_port);

  return true;
}
