/*
 * Copyright(C) 2025 eSOL Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <M5CoreS3.h>

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>

#if defined(USE_RMW_ZENOH_PICO)
#include <rmw_zenoh_transport.h>
#include "private.h"
#elif defined(USE_RMW_MICROXRCEDDS)
#include <micro_ros_platformio.h>
#endif /* defined(USE_RMW_ZENOH_PICO) */

// ============================================================
// for ROS/ROS2/Micro-ROS
// ============================================================
#define RCCHECK(fn) { \
    rcl_ret_t temp_rc = fn; \
    if((temp_rc != RCL_RET_OK)) { \
        display.printf("Failed status on line %d: %d. Aborting.\n", \
            __LINE__,(int)temp_rc); \
    } \
  }

#define RCSOFTCHECK(fn) { \
    rcl_ret_t temp_rc = fn; \
    if((temp_rc != RCL_RET_OK)) { \
        display.printf("Failed status on line %d: %d. Continuing.\n", \
            __LINE__,(int)temp_rc); \
    } \
  }

rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_init_options_t init_options;
rcl_node_t node;
rcl_publisher_t publisher;
rcl_timer_t timer;

std_msgs__msg__Int32 msg;

// ==================================================
// for M5Stack Core S3 Target
// ==================================================
#define LCD_TEXT_SIZE 1
#define DEBUG_SERIAL Serial

// Display Library
M5GFX display;                        // Main Display

// ------------------------------------------------------------
// functions
// ------------------------------------------------------------
static void timer_callback(rcl_timer_t * timer, int64_t last_call_time);
static void init_target(void);
static bool init_transports(void);
static void init_wifi(void);
static void init_display(void);

// --------------------------------------------------
// Arduino fundamental functions
// --------------------------------------------------
void setup()
{
    // setup M5Stack device
    init_target();

    // create allocator
    allocator = rcl_get_default_allocator();

    // create init_options
    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

#if defined(USE_ROS_DOMAIN_ID)
    init_options = rcl_get_zero_initialized_init_options();
    RCCHECK(rcl_init_options_init(&init_options, allocator));
    RCCHECK(rcl_init_options_set_domain_id(&init_options, ROS_DOMAIN_ID));
    RCCHECK(rclc_support_init_with_options(
        &support,
        0,
        NULL,
        &init_options, &allocator));
#endif /* defined(USE_ROS_DOMAIN_ID) */

    // create node
    while(1) {
        rcl_ret_t temp_rc = rclc_node_init_default( &node,
            "zenoh_pico_publish_node",
            "",
            &support);

        if ((temp_rc != RCL_RET_OK)) {
            display.setTextColor(TFT_RED);
            display.printf("ERR rclc_node_init_default().\n");
            display.printf("Please Check ...\n");
            display.printf("- Is zenohd running?\n");
            display.printf("- zenohd IP address correct? (%s)\n", ZENOHD_IP);
#if defined(ROS_DOMAIN_ID)
            display.printf("- ROS_DOMAIN_ID? (%d)\n", ROS_DOMAIN_ID);
#else
            display.printf("- ROS_DOMAIN_ID? (none)\n");
#endif
            while(1) {
                // Infinite Loop...
                delay(1000);
            };
        } else {
            display.setTextColor(TFT_SKYBLUE);
            display.printf("Success rclc_node_init_default()\n");
            break;
        }
    }

    // create publisher
    RCCHECK(rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "/Int32_inc_num"));

    // create timer,
    const unsigned int timer_timeout = 1000;
    RCCHECK(rclc_timer_init_default2(
        &timer,
        &support,
        RCL_MS_TO_NS(timer_timeout),
        timer_callback, true));

    // create executor
    executor = rclc_executor_get_zero_initialized_executor();
    RCCHECK(rclc_executor_init(
        &executor,
        &support.context,
        1,
        &allocator));

    RCCHECK(rclc_executor_add_timer(&executor, &timer));
    msg.data = 0;
}

void loop()
{
    delay(100);
    RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
}

// --------------------------------------------------
// Application specific local functions
// --------------------------------------------------
static void timer_callback(rcl_timer_t * timer, int64_t last_call_time) {
    RCLC_UNUSED(last_call_time);
    if (timer != NULL) {
        RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
        msg.data++;
    }
}

static void init_target(void)
{
    // initialize M5stack target
    auto cfg = M5.config();
    M5.begin(cfg);
    DEBUG_SERIAL.begin(115200);

    // initialize LCD Display
    init_display();

    // initialize Wi-Fi, micro-ROS transport setting
    if (!init_transports()) {
        display.printf("Unable to start communication.\n");
        while(1) {
            // Infinite Loop...
            delay(1000);
        }
    }
}

static void init_display(void)
{
    // Main Display initialize.
    display.begin();

    display.fillScreen(TFT_BLACK);
    display.setTextWrap(true);
    display.setTextSize(LCD_TEXT_SIZE);
    display.setTextColor(TFT_WHITE);
}

static bool init_transports(void)
{
#if defined(USE_RMW_ZENOH_PICO)
    /* Use zenoh-pico & rmw_zenoh_pico */
    int port = 0;
    IPAddress ip_addr;
    String connect_ip_addr;
    String my_ip_addr;

    init_wifi();

#if defined(RMW_ZENOH_PICO_TRANSPORT_UNICAST)
    port = ZENOHD_PORT;
    ip_addr.fromString(ZENOHD_IP);
    connect_ip_addr = ip_addr.toString();
    my_ip_addr =  WiFi.localIP().toString();

    display.setTextColor(TFT_WHITE);
    display.printf("my IP:%s\n", my_ip_addr.c_str());
    display.printf("connecting ZENOHD_IP=%s:%d\n", connect_ip_addr, port);

    bool ret = set_rmw_zenoh_unicast_transports(
        connect_ip_addr.c_str(),
        port,
        my_ip_addr.c_str(),
        -1);
    if (!ret) {
        display.printf("set_rmw_zenoh_unicast_transports() failed.\n");
        return false;
    }
#elif defined(RMW_ZENOH_PICO_TRANSPORT_MULTICAST)
    port = ZENOH_PEAR_MULTICAST_PORT;
    ip_addr.fromString(ZENOH_PEAR_MULTICAST_IP);
    connect_ip_addr = ip_addr.toString();
#if defined(USE_ESP32_WIFI_SOFT_AP)
    my_ip_addr =  WiFi.softAPIP().toString();
#else  /* defined(USE_ESP32_WIFI_SOFT_AP) */
    my_ip_addr =  WiFi.localIP().toString();
#endif /* defined(USE_ESP32_WIFI_SOFT_AP) */
    String device_name = ZENOH_PEAR_MULTICAST_DEVICE;
    display.setTextColor(TFT_WHITE);
    display.printf("my IP:%s\n", my_ip_addr.c_str());
    display.printf("connecting Multicast IP=%s:%d\n", connect_ip_addr, port);
    bool ret = set_rmw_zenoh_mcast_transports(
        connect_ip_addr.c_str(),
        port,
        device_name.c_str());
    if (!ret) {
        display.printf("set_rmw_zenoh_mcast_transports() failed.\n");
        return false;
    }
#else  /* defined(RMW_ZENOH_PICO_TRANSPORT_UNICAST) */
    display.print("Transport type not specified for zenoh-pico.");
    return false;
#endif /* defined(RMW_ZENOH_PICO_TRANSPORT_UNICAST) */

#elif defined(USE_RMW_MICROXRCEDDS)
    /* Use Micro-XRCE-DDS-Client & rmw-microxrcedds */
    uint16_t agent_port = MICROROS_AGENT_PORT;
    IPAddress agent_ip;

    agent_ip.fromString(MICRROS_AGENT_IP);
    bool ret = set_microros_wifi_transports((char *)WIFI_SSID, (char *)WIFI_PASS, agent_ip, agent_port);
    delay(2000);
    if (!ret) {
        display.printf("set_microros_wifi_transports() failed.\n");
        return false;
    }
#else  /* defined(USE_RMW_ZENOH_PICO) */
    display.print("RMW not specified.");
    return false;
#endif /* defined(USE_RMW_ZENOH_PICO) */
    return true;
}

static void init_wifi(void)
{
#if defined(USE_ESP32_WIFI_SOFT_AP)
    display.setTextColor(TFT_WHITE);
    // set Wi-Fi AP & Station Mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    delay(200);
    WiFi.softAPConfig(IPAddress(192, 168, 241, 1), IPAddress(192, 168, 241, 1), IPAddress(255, 255, 255, 0));
    delay(100);
    display.setTextColor(TFT_SKYBLUE);
    display.println("M5Stack Soft AP Start.");

#else /* USE_ESP32_WIFI_SOFT_AP */
    // Set WiFi in STA mode and trigger attachment
    display.setTextColor(TFT_WHITE);
    display.print("Connecting to WiFi ");
    // set Wi-Fi Station-Mode (Adapter-Mode)
    WiFi.mode(WIFI_STA);
    // connect to Wi-Fi SSID & PASS from platformio.ini build_flag
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait for WiFi to connect to the router
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        display.print(".");
    }
    display.setTextColor(TFT_SKYBLUE);
    display.println(" connect success.");
#endif /* USE_ESP32_WIFI_SOFT_AP */
}
