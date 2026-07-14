# ESP32 micro-ROS template using PlatformIO
![ROS2](https://img.shields.io/badge/ros2-jazzy-blue?logo=ros&logoColor=white)
![Espressif](https://img.shields.io/badge/Espressif-ESP32--S3-E7352C?logo=espressif&logoColor=white)
![License](https://img.shields.io/github/license/grupo-avispa/micro-ros-esp32-template)

A professional-grade firmware template for ESP32 microcontrollers running [micro-ROS](https://micro.ros.org/) with FreeRTOS multitasking support. Built with [PlatformIO](https://platformio.org/) on top of the **ESP-IDF** framework, this template provides a solid foundation for building distributed robotics systems with ROS 2 integration.

> **Note:** This template uses the ESP-IDF framework (`framework = espidf`) together with the [`micro_ros_espidf_component`](https://github.com/micro-ROS/micro_ros_espidf_component), which is vendored as a **git submodule** under `components/`. It is **not** the Arduino-based `micro_ros_platformio` library.

## Features

- **micro-ROS Integration**: Full ROS 2 publisher/subscriber capabilities over WiFi
- **ESP-IDF native**: Uses ESP-IDF APIs (`esp_log`, FreeRTOS, `esp_wifi`) directly
- **In-code configuration**: WiFi and Agent settings live in a version-controlled header
- **Modular WiFi library**: Reusable, self-contained station bring-up under `lib/wifi/`
- **FreeRTOS Multitasking**: Concurrent task execution with priority-based scheduling
- **micro-ROS as a submodule**: Reproducible builds, kept in sync with upstream

## Dependencies
- [PlatformIO](https://docs.platformio.org/) (Cross-platform build system)
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) (managed automatically by PlatformIO)
- [Robot Operating System (ROS) 2](https://docs.ros.org/en/jazzy/) (middleware for robotics)
- [micro-ROS](https://micro.ros.org/) (ROS 2 client library for microcontrollers)
- ROS 2 build tools in the ESP-IDF Python environment (see [Setup](#setup))

## Getting the sources

This repository uses a git submodule for the micro-ROS component, so clone it recursively:

```bash
git clone --recurse-submodules <repo-url>
# or, if you already cloned it:
git submodule update --init --recursive
```

To update the micro-ROS component to the latest upstream commit of its tracked branch:

```bash
git submodule update --remote components/micro_ros_espidf_component
```

## Setup

The `micro_ros_espidf_component` **builds micro-ROS from source** on the first build using the ROS 2 build tools (colcon/ament/rosidl). These must be available in the **Python environment that ESP-IDF uses** (PlatformIO ships its own, e.g. `~/.platformio/penv/.espidf-<version>`).

Install them into that interpreter once:

```bash
# Locate the Python used by your PlatformIO ESP-IDF install (adjust the version):
PY=~/.platformio/penv/.espidf-5.5.3/bin/python3

$PY -m pip install catkin_pkg lark-parser "empy==3.3.4" colcon-common-extensions numpy
```

> `empy` must be pinned to `3.3.4`; newer 4.x releases are incompatible with `rosidl`.

### Building on a machine with ROS 2 sourced

If your shell sources a system ROS 2 (`/opt/ros/...` or a colcon workspace), its environment variables **and** its entries in `PATH` leak into the isolated micro-ROS build and break it (CMake's `find_package` searches the parent of every `PATH` entry, pulling in host ROS packages).

Use the provided [`build.sh`](build.sh) wrapper, which strips the ROS environment before invoking PlatformIO:

```bash
./build.sh run              # build
./build.sh run -t upload    # build and flash
./build.sh run -t monitor   # open the serial monitor
```

If your machine does **not** source ROS 2, plain `pio run` works too.


## Configuration

### WiFi and Agent (in code)

WiFi credentials and the micro-ROS Agent address are defined in [`include/config_transport.hpp`](include/config_transport.hpp). The firmware brings up the WiFi station itself (`wifi_connect()` in `main.cpp`), so there is no need to use `menuconfig` for these:

```cpp
/// WiFi network SSID (network name).
static constexpr char WIFI_SSID[] = "YOUR_SSID";

/// WiFi network password (WPA/WPA2).
static constexpr char WIFI_PASSWORD[] = "YOUR_PASSWORD";

/// IP address of the micro-ROS Agent (dotted-decimal string).
static constexpr char AGENT_IP[] = "192.168.0.150";

/// UDP port of the micro-ROS Agent (string, as required by the micro-ROS API).
static constexpr char AGENT_PORT[] = "9999";
```

The transport itself is selected in `platformio.ini` (`board_microros_transport = wifi`).

> **Security:** `config_transport.hpp` holds your WiFi credentials. Avoid committing real credentials to a public repository.

### ROS 2 node settings

Node-level parameters are defined in [`include/config_ros.hpp`](include/config_ros.hpp):

```cpp
/// ROS 2 Domain ID for network isolation (0-232)
static constexpr uint32_t ROS_DOMAIN_ID = 0;

/// ROS 2 Node name
static char ROS_NODE_NAME[] = "esp32_node";
```

## Building and Flashing

> On a machine with ROS 2 sourced, use `./build.sh` instead of `pio` (see [Setup](#setup)).

Build the project:
```bash
./build.sh run
```

Upload to ESP32:
```bash
./build.sh run -t upload
```

Monitor serial output:
```bash
./build.sh run -t monitor
```

## Usage

### 1. Start the micro-ROS Agent

On your ROS 2 enabled machine (WiFi/UDP transport):

```bash
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```

### 2. Verify Connection

Monitor device output:
```bash
./build.sh run -t monitor
```

You should see the node come up and start publishing:
```
I (1234) micro_ros: micro-ROS node 'esp32_node' ready, publishing on /esp32/counter
I (2234) micro_ros: Publishing: 0
I (3234) micro_ros: Publishing: 1
```

And on the ROS 2 side:
```bash
ros2 topic echo /esp32/counter
```

## The example: a timer-driven publisher

[`main/main.cpp`](main/main.cpp) implements a minimal `std_msgs/Int32` publisher running inside a dedicated FreeRTOS task (`micro_ros_task`):

1. `app_main()` brings up WiFi via the `wifi` library and spawns the micro-ROS task.
2. The task initializes the node, a publisher on `/esp32/counter`, a 1 Hz timer and an executor.
3. On every timer tick, `timer_callback()` publishes an incrementing counter.

Error handling uses the `RCCHECK` / `RCSOFTCHECK` macros from [`include/macros.hpp`](include/macros.hpp), which log via `ESP_LOG` and abort the task on fatal errors.

The WiFi station bring-up is isolated in a self-contained, reusable PlatformIO library ([`lib/wifi/`](lib/wifi/)) that takes the credentials as parameters:

```cpp
wifi_connect(WIFI_SSID, WIFI_PASSWORD, WIFI_MAXIMUM_RETRY);
```

To publish your own data, create additional publishers/subscribers in `micro_ros_task()` and add them to the executor:

```cpp
RCCHECK(rclc_publisher_init_default(
    &my_publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "my_topic"));
```

## Task Architecture

The example runs a single micro-ROS task; add your own FreeRTOS tasks (sensor reading, processing, etc.) and communicate with the executor through thread-safe queues.

| Task              | Priority | Purpose                                    |
| ----------------- | -------- | ------------------------------------------ |
| **uros_task**     | 5        | Runs the micro-ROS node, timer & executor  |

## Project structure

```
.
├── main/
│   └── main.cpp                     # micro-ROS node, publisher and FreeRTOS task + app_main
├── include/
│   ├── config_ros.hpp               # ROS 2 node settings (name, domain id)
│   ├── config_transport.hpp         # WiFi credentials and micro-ROS Agent address
│   └── macros.hpp                   # RCCHECK / RCSOFTCHECK error-handling macros
├── lib/
│   └── wifi/                        # self-contained WiFi station library
│       ├── wifi.hpp
│       └── wifi.cpp
├── components/
│   └── micro_ros_espidf_component/  # micro-ROS component (git submodule)
├── build.sh                         # PlatformIO wrapper that strips the host ROS environment
├── custom.meta                      # micro-ROS (rmw) build configuration
└── platformio.ini
```

## Troubleshooting

### `rcl/rcl.h: No such file or directory`
The micro-ROS component is not registered. Ensure the submodule is checked out (`git submodule update --init --recursive`) so it lives under `components/micro_ros_espidf_component/`.

### `ModuleNotFoundError: No module named 'catkin_pkg'` (or `colcon` / `empy`)
The ROS 2 build tools are missing from the ESP-IDF Python environment. See [Setup](#setup).

### `Could not find ROS middleware implementation 'rmw_cyclonedds_cpp'` / host `/opt/ros` packages
A system ROS 2 environment is leaking into the isolated micro-ROS build. Build with [`build.sh`](build.sh), which strips the ROS environment and `PATH`.

### Connection Issues
- Verify WiFi credentials and the Agent IP/port in `include/config_transport.hpp`.
- Ensure the micro-ROS Agent is running and reachable: `ping <agent_ip>`.
- Confirm both device and Agent use the same `ROS_DOMAIN_ID`.

### Rebuilding micro-ROS from scratch
```bash
./build.sh run -t clean-microros   # then rebuild
```
