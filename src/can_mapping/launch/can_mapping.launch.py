import os
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='can_mapping',
            executable='wheel_vel_mapper_node',
            name='wheel_vel_mapper',
            output='screen',
            parameters=[
                # 車輪 (FL, FR, RL, RR等) の順序に合わせて、モータドライバ基板の Board Num を設定
                {
                    'board_nums': [0, 1, 2, 3],
                    'priority': 3,
                    'data_type': 0x0A,  # 4250_BLDC_DRIVER
                    'register_id': 0x0013  # RPS_TARGETVV
                 }
            ]
        )
    ])
