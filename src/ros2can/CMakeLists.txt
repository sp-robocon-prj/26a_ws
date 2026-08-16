cmake_minimum_required(VERSION 3.8)
project(ros2can)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/UdpCanFrame.msg"
)
ament_export_dependencies(rosidl_default_runtime)

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
  udp_bridge_node
  DESTINATION
  lib/${PROJECT_NAME}
)

ament_package()
