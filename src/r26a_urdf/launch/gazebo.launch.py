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

    # Bridge between ROS and Gazebo
    bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
            '/joint_states@sensor_msgs/msg/JointState[gz.msgs.Model',
            '/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry',
            '/tf@tf2_msgs/msg/TFMessage[gz.msgs.Pose_V',
            '/gazebo/lidar_dense/points@sensor_msgs/msg/PointCloud2[gz.msgs.PointCloudPacked',
            '/omni_1/cmd_vel@std_msgs/msg/Float64]gz.msgs.Double',
            '/omni_2/cmd_vel@std_msgs/msg/Float64]gz.msgs.Double',
            '/omni_3/cmd_vel@std_msgs/msg/Float64]gz.msgs.Double',
            '/omni_4/cmd_vel@std_msgs/msg/Float64]gz.msgs.Double',
        ],
        output='screen'
    )

    # Inverse Kinematics Controller Nodes
    twist2vel_node = Node(
        package='controller',
        executable='twist2velocity_node',
        name='twist2velocity_node',
        output='screen'
    )

    vel2omni_node = Node(
        package='controller',
        executable='velocity2omni_node',
        name='velocity2omni_node',
        output='screen',
        parameters=[{
            'chassis_type': 'omni4_x',
            'scale': 10.0,
            'publish_gazebo': True
        }]
    )

    # Unitree L2 Non-Repetitive Scan Simulator Node
    l2_sim_node = Node(
        package='unitree_l2_sim',
        executable='unitree_l2_node',
        name='unitree_l2_sim_node',
        output='screen',
        parameters=[
            {'input_topic': '/gazebo/lidar_dense/points'},
            {'output_topic': '/scan_l2_points'},
            {'update_rate': 10.0},
            {'points_per_sec': 64000}
        ]
    )


    return LaunchDescription([
        node_robot_state_publisher,
        gazebo,
        spawn,
        bridge,
        twist2vel_node,
        vel2omni_node,
        l2_sim_node
    ])
