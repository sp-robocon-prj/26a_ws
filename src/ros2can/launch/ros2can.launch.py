import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    # yamlファイルのパスを取得
    config = os.path.join(
        get_package_share_directory('ros2can'),
        'config',
        'ros2can_params.yaml'
    )

    return LaunchDescription([
        Node(
            package='ros2can',
            executable='twist2velocity_node',
            name='twist2velocity_node',
            output='screen'
        ),
        Node(
            package='ros2can',
            executable='velocity2omni_node',
            name='velocity2omni_node',
            output='screen',
            parameters=[config]
        ),
        Node(
            package='ros2can',
            executable='udp_bridge_node',
            name='udp_bridge_node',
            output='screen'
        )
    ])
