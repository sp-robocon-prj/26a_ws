import os
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='joy',
            executable='joy_node',
            name='joy_node'
        ),
        Node(
            package='ros2_joystick_input',
            executable='joy_input',
            name='joy_input'
        ),
        Node(
            package='ros2_joystick_input',
            executable='btn_input',
            name='btn_input'
        ),
        Node(
            package='ros2_joystick_input',
            executable='d_pad_input',
            name='d_pad_input'
        ),
        Node(
            package='ros2_joystick_input',
            executable='trigger_input',
            name='trigger_input'
        )
    ])
