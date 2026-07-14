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
 * @file wifi.cpp
 * @brief WiFi station bring-up implementation (see wifi.hpp).
 */

#include "wifi.hpp"

#include <cstring>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

namespace {

/// Log tag for the WiFi module.
constexpr char TAG[] = "wifi";

/// Event group bit set once the station obtains an IP address.
constexpr EventBits_t WIFI_CONNECTED_BIT = BIT0;

/// Event group bit set once all connection retries have been exhausted.
constexpr EventBits_t WIFI_FAIL_BIT = BIT1;

/// Event group used to wait for the WiFi station to connect.
EventGroupHandle_t s_wifi_event_group = nullptr;

/// Number of connection retries attempted so far.
int s_retry_num = 0;

/// Maximum number of connection retries before giving up.
int s_max_retry = 10;

/**
 * @brief WiFi/IP event handler. Drives (re)connection and signals the result.
 * @param arg Unused user argument.
 * @param event_base Event base (WIFI_EVENT or IP_EVENT).
 * @param event_id Event identifier.
 * @param event_data Event payload.
 */
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data) {
  (void)arg;
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < s_max_retry) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "Retrying WiFi connection (%d/%d)", s_retry_num, s_max_retry);
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    auto* event = static_cast<ip_event_got_ip_t*>(event_data);
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

}  // namespace

void wifi_connect(const char* ssid, const char* password, int max_retry) {
  s_max_retry = max_retry;
  s_retry_num = 0;

  // Initialize NVS, required by the WiFi driver to store calibration data.
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, nullptr));

  wifi_config_t wifi_config = {};
  std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), ssid,
               sizeof(wifi_config.sta.ssid));
  std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), password,
               sizeof(wifi_config.sta.password));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "Connecting to WiFi SSID '%s'...", ssid);

  // Block until the station connects or exhausts all retries.
  EventBits_t bits = xEventGroupWaitBits(
      s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "Connected to WiFi SSID '%s'", ssid);
  } else {
    ESP_LOGE(TAG, "Failed to connect to WiFi SSID '%s'", ssid);
  }
}
