#ifndef _PRIVATE_H_
#define _PRIVATE_H_

#if defined(USE_ROS_DOMAIN_ID)
#define ROS_DOMAIN_ID 64
#endif

#if defined(USE_ESP32_WIFI_SOFT_AP)
// Use M5StackCoreS3 as a Wi-Fi access point
#define WIFI_SSID "M5Stack_Soft_AP_SSID"
#define WIFI_PASS "M5Stack_Soft_AP_PASS"
#else
// Use an external Wi-Fi router
#define WIFI_SSID "Your_Wi-Fi_SSID"
#define WIFI_PASS "Your_Wi-Fi_PASS"
#endif

#if defined(USE_RMW_ZENOH_PICO)
/* Use zenoh-pico & rmw_zenoh_pico */
#if defined(RMW_ZENOH_PICO_TRANSPORT_UNICAST)
/* Use unicast(server/client) */
#define ZENOHD_IP                   "192.168.1.1" /* Set the IP address of the device on which zenohd is running.  */
#define ZENOHD_PORT                 7447          /* Set the port number used by zenoh. */
#elif defined(RMW_ZENOH_PICO_TRANSPORT_MULTICAST)
/* Use multicast(p2p) */
#define ZENOHD_IP                   "224.0.0.123"
#define ZENOHD_PORT                 8000
#define ZENOH_PEAR_MULTICAST_DEVICE "lo"
#endif /* defined(RMW_ZENOH_PICO_TRANSPORT_UNICAST) */

#elif defined(USE_RMW_MICROXRCEDDS)
/* Use Micro-XRCE-DDS-Client & rmw-microxrcedds */
#define MICRROS_AGENT_IP             "192.168.2.2"
#define MICROROS_AGENT_PORT          8888

#endif /* defined(USE_RMW_ZENOH_PICO) */

#endif /* _PRIVATE_H_ */
