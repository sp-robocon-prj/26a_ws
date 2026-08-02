cmake_minimum_required(VERSION 3.8)
project(ros2can)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/RobotVelocityCommand.msg"
  "msg/WheelVelocityCommand.msg"
  "msg/UdpCanFrame.msg"
)

# Layer 3: twist2velocity_node
add_executable(twist2velocity_node src/twist2velocity_node.cpp)
ament_target_dependencies(twist2velocity_node rclcpp geometry_msgs)
rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")
target_link_libraries(twist2velocity_node "${cpp_typesupport_target}")

# Layer 2: velocity2omni_node
add_executable(velocity2omni_node src/velocity2omni_node.cpp)
ament_target_dependencies(velocity2omni_node rclcpp)
rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")
target_link_libraries(velocity2omni_node "${cpp_typesupport_target}")

# UDP Bridge: udp_bridge_node
add_executable(udp_bridge_node src/udp_bridge_node.cpp)
ament_target_dependencies(udp_bridge_node rclcpp)
rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")
target_link_libraries(udp_bridge_node "${cpp_typesupport_target}")

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

install(
  TARGETS
  twist2velocity_node
  velocity2omni_node
  udp_bridge_node
  DESTINATION
  lib/${PROJECT_NAME}
)

install(
  DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
