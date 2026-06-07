from setuptools import find_packages, setup

package_name = 'rs_labs_um7'

setup(
    name=package_name,
    version='0.0.1',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='user',
    maintainer_email='user@todo.todo',
    description='ROS2 Jazzy driver for RSX-UM7 IMU sensor',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'um7_driver_node = rs_labs_um7.um7_driver_node:main'
        ],
    },
)