import os

from ament_index_python.packages import get_package_share_directory

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, AppendEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    pkg_share = get_package_share_directory("field")
    world = os.path.join(
        pkg_share,
        "worlds",
        "field.world"
    )

    return LaunchDescription([
        AppendEnvironmentVariable(
            name='GZ_SIM_RESOURCE_PATH',
            value=os.path.join(pkg_share, 'models')
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
        )
    ])