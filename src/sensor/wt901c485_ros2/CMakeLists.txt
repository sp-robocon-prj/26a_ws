cmake_minimum_required(VERSION 3.8)
project(wt901c485_imu)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Dependencies ────────────────────────────────────────────────────────────
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

# ── C++ IMU driver node ──────────────────────────────────────────────────────
add_executable(wt901c485_node
  src/wt901c485_node.cpp
  src/wt901c485_driver.cpp
)

target_include_directories(wt901c485_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(wt901c485_node PUBLIC cxx_std_17)

ament_target_dependencies(wt901c485_node
  rclcpp
  std_msgs
  sensor_msgs
  geometry_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
)

install(TARGETS wt901c485_node
  DESTINATION lib/${PROJECT_NAME}
)

# ── Python nodes ─────────────────────────────────────────────────────────────
ament_python_install_package(${PROJECT_NAME})

install(PROGRAMS
  scripts/wt901c485_py_node.py
  DESTINATION lib/${PROJECT_NAME}
)

# ── Launch / config / rviz ────────────────────────────────────────────────────
install(DIRECTORY
  launch
  config
  rviz
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
