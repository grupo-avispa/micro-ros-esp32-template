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
 * @file macros.hpp
 * @brief Utility macros for error handling and logging in micro-ROS ESP32 firmware.
 *
 * This header defines macros for checking the return values of ROS2 functions,
 * as well as an error loop function that halts execution if a critical error occurs.
 *
 */

#ifndef MACROS_HPP
#define MACROS_HPP

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/// Log tag used by the micro-ROS firmware.
static const char* MACROS_TAG = "micro_ros";

/**
 * @brief Macro for checking ROS2 function return values.
 *        Logs the failing line and stops the micro-ROS task if the call fails.
 * @param fn The ROS2 function to check.
 */
#define RCCHECK(fn)                                                       \
  {                                                                       \
    rcl_ret_t temp_rc = fn;                                              \
    if ((temp_rc != RCL_RET_OK)) {                                        \
      ESP_LOGE(MACROS_TAG, "Failed status on line %d: %d. Aborting.",     \
               __LINE__, (int)temp_rc);                                   \
      vTaskDelete(NULL);                                                  \
    }                                                                     \
  }

/**
 * @brief Soft version of RCCHECK that logs but doesn't stop execution.
 * @param fn The ROS2 function to check.
 */
#define RCSOFTCHECK(fn)                                                    \
  {                                                                        \
    rcl_ret_t temp_rc = fn;                                               \
    if ((temp_rc != RCL_RET_OK)) {                                         \
      ESP_LOGW(MACROS_TAG, "Failed status on line %d: %d. Continuing.",    \
               __LINE__, (int)temp_rc);                                    \
    }                                                                      \
  }

#endif // MACROS_HPP
