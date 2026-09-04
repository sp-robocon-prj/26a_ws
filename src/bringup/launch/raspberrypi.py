import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    # yamlファイルのパスを取得
    config = os.path.join(
        get_package_share_directory('controller'),
        'config',
        'controller_params.yaml'
    )

    return LaunchDescription([
        Node(
            package='controller',
            executable='twist2velocity_node',
            name='twist2velocity_node',
            output='screen'
        ),
        Node(
            package='controller',
            executable='velocity2omni_node',
            name='velocity2omni_node',
            output='screen',
            parameters=[config]
        ),
        Node(
            package='ros2can',
            executable='ros2can_node',
            name='ros2can_node',
            output='screen',
            parameters=[{'remote_ip': '192.168.10.103'}]
        ),
    ])
