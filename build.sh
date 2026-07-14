#!/usr/bin/env bash
# Build helper for micro-ROS ESP-IDF projects on a machine that sources ROS 2.
#
# The micro_ros_espidf_component builds micro-ROS from source with colcon/ament.
# If a host ROS 2 environment is sourced, its env vars AND its entries in PATH
# leak into that isolated build (CMake's find_package searches the parent of
# every PATH entry), pulling in /opt/ros packages and breaking the build.
# This wrapper strips the ROS environment before invoking PlatformIO.

set -euo pipefail

# Rebuild PATH without any ROS / colcon workspace entries.
CLEAN_PATH="$(printf '%s' "${PATH:-}" | tr ':' '\n' \
    | grep -vE '/opt/ros/|_ws/install/|/ros2/' | paste -sd:)"

exec env \
    -u RMW_IMPLEMENTATION -u ROS_DISTRO -u ROS_VERSION -u ROS_PYTHON_VERSION \
    -u ROS_DOMAIN_ID -u ROS_AUTOMATIC_DISCOVERY_RANGE -u ROS_LOCALHOST_ONLY \
    -u AMENT_PREFIX_PATH -u CMAKE_PREFIX_PATH -u COLCON_PREFIX_PATH \
    -u ROS_PACKAGE_PATH -u PYTHONPATH -u LD_LIBRARY_PATH -u _colcon_cd_root \
    PATH="$CLEAN_PATH" \
    ~/.platformio/penv/bin/pio "${@:-run}"
