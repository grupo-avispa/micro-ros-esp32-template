"""Strip the host ROS 2 environment before PlatformIO configures the project.

The micro_ros_espidf_component builds micro-ROS from source with colcon/ament. A sourced
host ROS 2 environment (env vars and /opt/ros entries in PATH) leaks into that isolated
build and breaks it. This runs for every PlatformIO entry point (CLI and the VS Code
extension), so it does not depend on using build.sh.
"""
import os
import re

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

ROS_VARS = (
    "ROS_DISTRO", "ROS_VERSION", "ROS_PYTHON_VERSION", "ROS_DOMAIN_ID",
    "ROS_AUTOMATIC_DISCOVERY_RANGE", "ROS_LOCALHOST_ONLY", "AMENT_PREFIX_PATH",
    "CMAKE_PREFIX_PATH", "COLCON_PREFIX_PATH", "ROS_PACKAGE_PATH", "PYTHONPATH",
    "LD_LIBRARY_PATH", "_colcon_cd_root",
)

for var in ROS_VARS:
    os.environ.pop(var, None)

os.environ["PATH"] = os.pathsep.join(
    p for p in os.environ.get("PATH", "").split(os.pathsep)
    if not re.search(r"/opt/ros/|_ws/install/|/ros2/", p)
)
# Without this, rmw_implementation defaults to rmw_fastrtps_cpp, which is not built.
os.environ["RMW_IMPLEMENTATION"] = "rmw_microxrcedds"
