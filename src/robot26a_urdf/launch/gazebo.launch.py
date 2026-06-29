import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction, AppendEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
import xacro
from os.path import join

def generate_launch_description():
    pkg_ros_gz_sim = get_package_share_directory('ros_gz_sim')
    pkg_ros_gz_rbot = get_package_share_directory('robot26a_urdf')

    robot_description_file = os.path.join(pkg_ros_gz_rbot, 'urdf', 'ros26a_urdf.xacro')
    ros_gz_bridge_config = os.path.join(pkg_ros_gz_rbot, 'config', 'ros_gz_bridge_gazebo.yaml')
    
    robot_description_config = xacro.process_file(robot_description_file)
    robot_description = {'robot_description': robot_description_config.toxml()}
   
    robot_state_publisher = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[robot_description],
    )

    world_file = os.path.join(pkg_ros_gz_rbot, 'worlds', 'field.sdf')
    
    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(join(pkg_ros_gz_sim, "launch", "gz_sim.launch.py")),
        launch_arguments={"gz_args": f"-r -v 4 {world_file}"}.items()
    )

    spawn_robot = TimerAction(
        period=3.0,  
        actions=[Node(
            package='ros_gz_sim',
            executable='create',
            arguments=[
                "-topic", "/robot_description",
                "-name", "ros26a_urdf",
                "-allow_renaming", "false",
                "-x", "0.0", "-y", "0.0", "-z", "0.32", "-Y", "0.0"
            ],
            output='screen'
        )]
    )

    ros_gz_bridge = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        parameters=[{'config_file': ros_gz_bridge_config}],
        output='screen'
    )

    set_env = AppendEnvironmentVariable(
        'GZ_SIM_RESOURCE_PATH',
        os.path.join(get_package_share_directory('robot26a_urdf'), '..')
    )

    # Controller Spawners
    spawn_jsb_controller = TimerAction(
        period=7.0,
        actions=[Node(
            package='controller_manager',
            executable='spawner',
            arguments=['joint_state_broadcaster', '--controller-manager', '/controller_manager'],
            output='screen'
        )]
    )

    spawn_omni_controller = TimerAction(
        period=9.0,
        actions=[Node(
            package='controller_manager',
            executable='spawner',
            arguments=['omni_wheel_controller', '--controller-manager', '/controller_manager'],
            output='screen'
        )]
    )

    # Joy nodes
    joy_node = Node(
        package='joy',
        executable='joy_node',
        name='joy_node',
        parameters=[{'deadzone': 0.05, 'autorepeat_rate': 20.0}]
    )
    
    # Teleop Twist Joy
    teleop_node = Node(
        package='teleop_twist_joy',
        executable='teleop_node',
        name='teleop_twist_joy_node',
        parameters=[{
            'require_enable_button': False,
            'axis_linear.x': 1, # Left stick up/down
            'axis_linear.y': 0, # Left stick left/right
            'axis_angular.yaw': 3, # Right stick left/right
            'scale_linear.x': 1.0,
            'scale_linear.y': 1.0,
            'scale_angular.yaw': 1.0
        }],
        remappings=[('/cmd_vel', '/cmd_vel')]
    )

    # Custom Kinematics Node
    kinematics_node = Node(
        package='robot26a_urdf',
        executable='omni_kinematics_node',
        name='omni_kinematics_node',
        output='screen'
    )

    return LaunchDescription([
        set_env,
        gazebo,
        spawn_robot,
        ros_gz_bridge,
        robot_state_publisher,
        spawn_jsb_controller,
        spawn_omni_controller,
        joy_node,
        teleop_node,
        kinematics_node
    ])
