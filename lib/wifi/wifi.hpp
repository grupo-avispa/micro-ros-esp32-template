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
 * @file wifi.hpp
 * @brief Self-contained WiFi station bring-up for ESP-IDF.
 */

#ifndef WIFI_HPP
#define WIFI_HPP

/**
 * @brief Bring up the WiFi station and block until it connects (or fails).
 *
 * Initializes NVS, the network interface and the WiFi driver in station mode,
 * then blocks until an IP address is obtained or all retries are exhausted.
 *
 * @param ssid WiFi network SSID.
 * @param password WiFi network password (WPA/WPA2).
 * @param max_retry Maximum number of connection retries before giving up.
 */
void wifi_connect(const char* ssid, const char* password, int max_retry = 10);

#endif // WIFI_HPP
