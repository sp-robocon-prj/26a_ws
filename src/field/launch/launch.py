import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, AppendEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
import xacro

def generate_launch_description():
    pkg_field_share = get_package_share_directory("field")
    pkg_r26a_urdf_share = get_package_share_directory("r26a_urdf")
    
    world = os.path.join(
        pkg_field_share,
        "worlds",
        "field.world"
    )

    # URDF
    xacro_file = os.path.join(pkg_r26a_urdf_share, "urdf", "r26a_urdf.xacro")
    robot_description_config = xacro.process_file(xacro_file)
    robot_description = {"robot_description": robot_description_config.toxml()}

    # robot_state_publisherの起動 (tfとrobot_descriptionを配信)
    node_robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        output="screen",
        parameters=[robot_description, {"use_sim_time": True}]
    )

    # Gazeboにロボットを召喚
    spawn_robot = Node(
        package="ros_gz_sim",
        executable="create",
        arguments=[
            "-name", "r26a",
            "-topic", "robot_description",
            "x",  "10"
            "y",  "4"
            "-z", "0.5"
        ],
        output="screen"
    )
    
    # clockを同期
    bridge = Node(
        package="ros_gz_bridge",
        executable="parameter_bridge",
        arguments=[
            "/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock",
        ],
        output="screen"
    )

    return LaunchDescription([
        AppendEnvironmentVariable(
            name="GZ_SIM_RESOURCE_PATH",
            value=os.path.join(pkg_field_share, "models")
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(
                    get_package_share_directory("ros_gz_sim"),
                    "launch",
                    "gz_sim.launch.py",
                )
            ),
            launch_arguments={
                "gz_args": f"-r {world}"
            }.items(),
        ),
        node_robot_state_publisher,
        spawn_robot,
        bridge
    ])
