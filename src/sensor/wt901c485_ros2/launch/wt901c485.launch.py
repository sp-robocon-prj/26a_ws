"""
launch/wt901c485.launch.py
Launches the WT901C-485 driver node and RViz2.

Usage:
  ros2 launch wt901c485_imu wt901c485.launch.py            # C++ node
  ros2 launch wt901c485_imu wt901c485.launch.py use_python:=true  # Python node
  ros2 launch wt901c485_imu wt901c485.launch.py port:=/dev/ttyUSB1 rate_hz:=100.0
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg = 'wt901c485_imu'

    # ── Shared launch arguments ───────────────────────────────────────────────
    args = [
        DeclareLaunchArgument('use_python',      default_value='false',
                              description='Use Python node instead of C++'),
        DeclareLaunchArgument('port',            default_value='/dev/ttyUSB0',
                              description='Serial port of USB-RS485 adapter'),
        DeclareLaunchArgument('baud',            default_value='9600'),
        DeclareLaunchArgument('modbus_address',  default_value='80',
                              description='Modbus slave address (decimal, default 80=0x50)'),
        DeclareLaunchArgument('frame_id',        default_value='imu_link'),
        DeclareLaunchArgument('rate_hz',         default_value='50.0'),
        DeclareLaunchArgument('publish_tf',      default_value='true'),
        DeclareLaunchArgument('launch_rviz',     default_value='true',
                              description='Whether to launch RViz2'),
    ]

    # ── Shared parameters dict ───────────────────────────────────────────────
    params = {
        'port':           LaunchConfiguration('port'),
        'baud':           LaunchConfiguration('baud'),
        'modbus_address': LaunchConfiguration('modbus_address'),
        'frame_id':       LaunchConfiguration('frame_id'),
        'rate_hz':        LaunchConfiguration('rate_hz'),
        'publish_tf':     LaunchConfiguration('publish_tf'),
    }

    rviz_config = PathJoinSubstitution(
        [FindPackageShare(pkg), 'rviz', 'wt901c485.rviz'])

    # ── C++ node ─────────────────────────────────────────────────────────────
    cpp_node = Node(
        package    = pkg,
        executable = 'wt901c485_node',
        name       = 'wt901c485_node',
        output     = 'screen',
        parameters = [params],
        condition  = UnlessCondition(LaunchConfiguration('use_python')),
    )

    # ── Python node ───────────────────────────────────────────────────────────
    py_node = Node(
        package    = pkg,
        executable = 'wt901c485_py_node.py',
        name       = 'wt901c485_py_node',
        output     = 'screen',
        parameters = [params],
        condition  = IfCondition(LaunchConfiguration('use_python')),
    )

    # ── Static TF: world → base_link (identity) ───────────────────────────────
    static_tf = Node(
        package    = 'tf2_ros',
        executable = 'static_transform_publisher',
        name       = 'world_to_base_link',
        arguments  = ['0', '0', '0', '0', '0', '0', 'world', 'base_link'],
    )

    # ── RViz2 ─────────────────────────────────────────────────────────────────
    rviz_node = Node(
        package   = 'rviz2',
        executable= 'rviz2',
        name      = 'rviz2',
        arguments = ['-d', rviz_config],
        condition = IfCondition(LaunchConfiguration('launch_rviz')),
    )

    return LaunchDescription(args + [static_tf, cpp_node, py_node, rviz_node])
