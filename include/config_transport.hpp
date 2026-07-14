// Copyright (c) 2026 Alberto J. Tudela Roldán
// Copyright (c) 2026 Grupo Avispa, DTE, Universidad de Málaga
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file config_transport.hpp
 * @brief Transport and network configuration for micro-ROS WiFi communication.
 *
 * WiFi credentials and the micro-ROS Agent address are defined here and used
 * directly by the firmware (see main.cpp). This keeps the network configuration
 * in a single, version-controlled file instead of the ESP-IDF menuconfig.
 *
 * @warning This file contains sensitive information (WiFi credentials).
 *          Do not commit real credentials to public repositories.
 */

#ifndef CONFIG_TRANSPORT_HPP
#define CONFIG_TRANSPORT_HPP

/**
 * @defgroup TransportConfig WiFi and micro-ROS Agent configuration
 * @{
 */

/// WiFi network SSID (network name).
static constexpr char WIFI_SSID[] = "YOUR_SSID";

/// WiFi network password (WPA/WPA2).
static constexpr char WIFI_PASSWORD[] = "YOUR_PASSWORD";

/// Maximum number of WiFi connection retries before giving up.
static constexpr int WIFI_MAXIMUM_RETRY = 10;

/// IP address of the micro-ROS Agent (dotted-decimal string).
static constexpr char AGENT_IP[] = "192.168.0.0";

/// UDP port of the micro-ROS Agent (string, as required by the micro-ROS API).
static constexpr char AGENT_PORT[] = "9999";

/** @} */

#endif // CONFIG_TRANSPORT_HPP
