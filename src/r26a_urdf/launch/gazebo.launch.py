import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration
import xacro

def generate_launch_description():
    # Directories
    pkg_r26a_urdf = get_package_share_directory('r26a_urdf')
    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')

    # Process Xacro
    xacro_file = os.path.join(pkg_r26a_urdf, 'urdf', 'r26a_urdf.xacro')
    robot_description_config = xacro.process_file(xacro_file)
    robot_description = {'robot_description': robot_description_config.toxml()}

    # Robot State Publisher Node
    node_robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        output='screen',
        parameters=[robot_description, {'use_sim_time': True}]
    )

    # Gazebo Sim
    world_file = os.path.join(pkg_r26a_urdf, 'worlds', 'empty.world')
    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pkg_ros_gz_sim, 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={'gz_args': f'-r {world_file}'}.items(),
    )

    # Spawn Robot in Gazebo
    spawn = Node(
        package='ros_gz_sim',
        executable='create',
        arguments=[
            '-name', 'r26a',
            '-topic', 'robot_description',
            '-z', '0.5' # Spawn a bit above ground to avoid collision
        ],
        output='screen'
    )

    # Bridge between ROS and Gazebo (Optional, for clock or joints)
    bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
            '/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
        ],
        output='screen'
    )

    return LaunchDescription([
        node_robot_state_publisher,
        gazebo,
        spawn,
        bridge
    ])
