# Zenoh-Pico and Micro-ROS Examples for PlatformIO

This workspace has sample configurations with `platformio.ini`.

> [!NOTE]
> Currently only some ESP32 boards are supported.

## Target Board

Open the platformio.ini file and change the value of "default_envs" in `[platformio]`, according to the table below.

| Target board to use | Value of default_envs to set |
| ---- |---- |
| M5Stack Basic (V2.7) | m5stack-basic |
| M5Stack Core2 | m5stack-core2 |
| M5Stack CoreS3 | m5stack-cores3 |
| AtomS3 (see Note) | m5stack-atoms3 |

> [!NOTE]
> You can also specify AtomS3 as `m5stack-atoms3`, but replace the display functions in `Main.cpp` because it is not available.

## RMW Implementation

For subsequent changes, edit the item of `[env: xxxxx]` corresponding to the selected value of `default_envs`.

Set `board_microros_rmw_impl` according to the following table.

| DDS to be used | Value of board_microros_rmw_impl to be set | Description |
| ---- | ---- |---- |
| Zenoh-Pico | rmw_zenoh_pico | Indicates the RMW to be used. Set this value when using Zenoh-Pico as the DDS. |
| Micro-XRCE-DDS | rmw_microxrcedds | Indicates the RMW to be used. Set this value when using Micro-XRCE-DDS as the DDS. |

## DDS Transport Options

Set `board_microros_transport` according to the following table.

| DDS to be used | Communication method to be used | Value of bord_microros_transport to be set | Description |
| ---- | ---- | ---- | ---- |
| Zenoh-Pico | Wi-Fi/UDP unicast | unicast | Set this value when using Wi-Fi to communicate via zenohd. |
| Zenoh-Pico | Wi-Fi/UDP multicast | multicast | Set this value when using Wi-Fi to perform P2P communication between devices without using zenohd. |
| Micro-XRCE-DDS | Wi-Fi/UDP | wifi | Set this value when using Wi-Fi to communicate with micro-ROS-Agent. |

## Build Options and Define Macros

Set the compile option of `build_flags` according to the environment to be used.
The `-D` compile option that can be used for each environment is as follows:

| Compile option | Meaning of the option | Description |
| ---- | ---- | ---- |
| BOARD_CORES3 | Indicates that the target board is a CORES3 version. | This option is required by env: m5stack-cores3. Do not specify it in any other env. |
| BOARD_ATOMS3 | Indicates that the target board is an ATOMS3 version. | This option is required by env: m5stack-atoms3. Do not specify it in any other env. |
| BOARD_BASIC | Indicates that the target board is a Basic (V2.7) version. | This option is required by env: m5stack-basic. Do not specify it in any other env. |
| BOARD_CORE2 | Indicates that the target board is a Core2 version. | This option is required by env: m5stack-core2. Do not specify it in any other env. |
| USE_RMW_ZENOH_PICO | Indicates that rmw_zenoh_pico is to be used. | This option specifies Zenoh-Pico as DDS. Exclusive with USE_RMW_MICROXRCEDDS. |
| USE_RMW_MICROXRCEDDS | Indicates that rmw_microxrcedds is to be used. | This option specifies Micro-XRCE-DDS as DDS. Exclusive with USE_RMW_ZENOH_PICO. |
| USE_ROS_DOMAIN_ID | Indicates that `ROS_DOMAIN_ID` will be specified for ROS communication. | The `ROS_DOMAIN_ID` definition value in `private.h` is enabled, and ROS communication is performed only with nodes with the same `ROS_DOMAIN_ID`. |
| USE_ESP32_WIFI_SOFT_AP | Indicates that the M5 device will be used as a Wi-Fi access point. | Specify whether the M5Stack will be used as an access point for P2P communication with Zenoh-Pico without installing a separate Wi-Fi router. |

## Network Connection Settings

The Wi-Fi SSID, password, and the "IP address: port number" of the connection destination for the Zenoh router are described in `src/private.h`.
Change the definition values according to your environment, the environment of the communication partner, and the settings of platformio.ini.

> [!IMPORTANT]
> The Wi-Fi frequency band that CoreS3 and AtomS3R can connect to is 2.4 GHz. It is not possible to connect to 5 GHz.

| Definition name | Description |
| --- | --- |
| ROS_DOMAIN_ID | When using ROS_DOMAIN_ID, set a common ID value in the system. |
| WIFI_SSID | Set the SSID string of the WiFi to be used. |
| WIFI_PASS | Set the connection password string of the WiFi to be used. |
| ZENOHD_IP | When using Zenoh-Pico as DDS, set the IP address of the PC running the Zenoh router (zenohd) to be connected. |
| ZENOHD_PORT | When using Zenoh-Pico as DDS, set the port number of the Zenoh router (zenohd) to be connected. (Default: 7447) |
| MICRROS_AGENT_IP | When using Micro-XRCE-DDS as DDS, set the PC's IP address running the micro-ROS Agent to be connected. |
| MICROROS_AGENT_PORT | When using Micro-XRCE-DDS as DDS, set the port number of the micro-ROS Agent to be connected. (Default: 8888) |
| ZENOH_PEAR_MULTICAST_IP | The IP address when using Zenoh-Pico as DDS for P2P multicast communication. This is fixed to "224.0.0.123". |
| ZENOH_PEAR_MULTICAST_PORT | The port number when using Zenoh-Pico as DDS for P2P multicast communication. This is fixed to 8000. |
| ZENOH_PEAR_MULTICAST_DEVICE | The device name when using Zenoh-Pico as DDS for P2P multicast communication. This is fixed to "lo". |

## Build

Open the VSCode terminal and run the build with the following commands:. The entire build takes about 5 to 10 minutes.
> [!NOTE]
> An internet connection is required during the build because the build process involves git cloning various dependent libraries from the internet.

```console
$ pio run
```

If the build completes successfully, you will see "[SUCCESS]" in the terminal.

## Cleanup

For a clean build, run the following three commands.
The first command cleans the generated micro_ros_platformio library, the second command cleans the intermediate micro_ros_platformio objects, and the third command cleans the app objects.

```console
$ pio run -t clean_libmicroros
$ pio run -t clean_microros
$ pio run -t clean
```
