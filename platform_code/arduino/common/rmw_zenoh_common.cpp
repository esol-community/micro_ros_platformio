#include <stdio.h>
#include <stdbool.h>

#include <Arduino.h>

#include "rmw_zenoh_common.h"

void set_rmw_zenoh_transports_log(RmwZenohPicoLogLevel level){
  if(level == RmwZenohPicoLogLevel::ERROR)
    rmw_zenoh_pico_debug_level_set(_Z_LOG_LVL_ERROR);
  else if(level == RmwZenohPicoLogLevel::INFO)
    rmw_zenoh_pico_debug_level_set(_Z_LOG_LVL_INFO);
  else if(level == RmwZenohPicoLogLevel::DEBUG)
    rmw_zenoh_pico_debug_level_set(_Z_LOG_LVL_DEBUG);
}

int target_print(const char *fmt, ...) {
  static char msg[128];

  va_list args;
  va_start(args , fmt);
  int ret = vsnprintf(msg, sizeof(msg) -1, fmt, args);
  va_end(args);

  Serial.print(msg);

  return ret;
}

void set_rmw_zenoh_peer_mode(){
  rmw_zenoh_pico_set_mode(RMW_ZENOH_PEER_MODE);
}
