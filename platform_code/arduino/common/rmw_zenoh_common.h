#ifndef RMW_ZENOH_COMMON_H
#define RMW_ZENOH_COMMON_H

#include "rmw_zenoh_pico/rmw_zenoh_pico_options.h"

typedef enum _rmwZenohPicoLogLevel {
  NONE,
  ERROR,
  INFO,
  DEBUG
} RmwZenohPicoLogLevel;

extern void set_rmw_zenoh_transports_log(RmwZenohPicoLogLevel level);
extern void set_rmw_zenoh_peer_mode();

#if defined(__cplusplus)
extern "C"
{
#endif  // if defined(__cplusplus)

  extern int target_print(const char *fmt, ...);

#if defined(__cplusplus)
}
#endif  // if defined(__cplusplus)

#endif
