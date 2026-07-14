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
 * @file main.cpp
 * @brief Minimal micro-ROS example: an std_msgs/Int32 publisher driven by a
 *        timer, running inside a dedicated FreeRTOS task.
 *
 * The node connects to the micro-ROS Agent over WiFi (UDP). WiFi credentials and
 * the Agent IP/port are defined in include/config_transport.hpp; the firmware
 * brings up the WiFi station itself instead of relying on the ESP-IDF menuconfig.
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
#include <rmw_microros/rmw_microros.h>
#endif

#include "config_ros.hpp"
#include "config_transport.hpp"
#include "macros.hpp"
#include "wifi.hpp"

/// Stack size (in bytes) for the micro-ROS task. micro-ROS needs a large stack.
static constexpr uint32_t MICRO_ROS_TASK_STACK = 16384;

/// FreeRTOS priority for the micro-ROS task.
static constexpr UBaseType_t MICRO_ROS_TASK_PRIORITY = 5;

/// Publisher handle shared between the setup and the timer callback.
static rcl_publisher_t publisher;

/// Message reused on every publication.
static std_msgs__msg__Int32 msg;

/**
 * @brief Timer callback. Publishes the current counter value and increments it.
 * @param timer Timer that triggered the callback.
 * @param last_call_time Time of the previous call (unused).
 * @param arg User argument (unused).
 */
static void timer_callback(rcl_timer_t* timer, int64_t last_call_time, uintptr_t arg) {
  RCLC_UNUSED(last_call_time);
  RCLC_UNUSED(arg);
  if (timer != nullptr) {
    ESP_LOGI(MACROS_TAG, "Publishing: %d", static_cast<int>(msg.data));
    RCSOFTCHECK(rcl_publish(&publisher, &msg, nullptr));
    msg.data++;
  }
}

/**
 * @brief micro-ROS task: initializes the node, publisher, timer and executor,
 *        then spins forever.
 * @param arg Unused task argument.
 */
static void micro_ros_task(void* arg) {
  RCLC_UNUSED(arg);
  rcl_allocator_t allocator = rcl_get_default_allocator();
  rclc_support_t support;

  rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
  RCCHECK(rcl_init_options_init(&init_options, allocator));
  RCCHECK(rcl_init_options_set_domain_id(&init_options, ROS_DOMAIN_ID));

#ifdef CONFIG_MICRO_ROS_ESP_XRCE_DDS_MIDDLEWARE
  // Set the static Agent address (from config_transport.hpp) instead of discovery.
  rmw_init_options_t* rmw_options = rcl_init_options_get_rmw_init_options(&init_options);
  RCCHECK(rmw_uros_options_set_udp_address(AGENT_IP, AGENT_PORT, rmw_options));
#endif

  // Create the support structure (context) with the configured options.
  RCCHECK(rclc_support_init_with_options(&support, 0, nullptr, &init_options, &allocator));

  // Create the node.
  rcl_node_t node;
  RCCHECK(rclc_node_init_default(&node, ROS_NODE_NAME, "", &support));

  // Create the publisher on the "esp32/counter" topic.
  RCCHECK(rclc_publisher_init_default(
      &publisher,
      &node,
      ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
      "esp32/counter"));

  // Create a 1 Hz timer that triggers the publication (autostart enabled).
  rcl_timer_t timer;
  const unsigned int timer_period_ms = 1000;
  RCCHECK(rclc_timer_init_default2(
      &timer, &support, RCL_MS_TO_NS(timer_period_ms), timer_callback, true));

  // Create the executor and register the timer.
  rclc_executor_t executor;
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  msg.data = 0;

  ESP_LOGI(MACROS_TAG, "micro-ROS node '%s' ready, publishing on /esp32/counter", ROS_NODE_NAME);

  // Spin forever, servicing the timer.
  while (true) {
    rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  // Unreachable, kept for completeness.
  RCCHECK(rcl_publisher_fini(&publisher, &node));
  RCCHECK(rcl_node_fini(&node));
  vTaskDelete(nullptr);
}

extern "C" void app_main(void) {
  // Bring up WiFi using the credentials from config_transport.hpp.
  wifi_connect(WIFI_SSID, WIFI_PASSWORD, WIFI_MAXIMUM_RETRY);

  // Pin the micro-ROS task to the APP CPU so the PRO CPU can handle WiFi.
  xTaskCreate(
      micro_ros_task,
      "uros_task",
      MICRO_ROS_TASK_STACK,
      nullptr,
      MICRO_ROS_TASK_PRIORITY,
      nullptr);
}
