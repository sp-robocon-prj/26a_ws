cmake_minimum_required(VERSION 3.8)
project(ros2_joystick_input)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(joy_input src/joy_input.cpp)
target_include_directories(joy_input PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(joy_input PUBLIC c_std_99 cxx_std_17)
ament_target_dependencies(joy_input "rclcpp" "sensor_msgs" "geometry_msgs" "std_msgs")

add_executable(btn_input src/btn_input.cpp)
target_include_directories(btn_input PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(btn_input PUBLIC c_std_99 cxx_std_17)
ament_target_dependencies(btn_input "rclcpp" "sensor_msgs" "std_msgs")

add_executable(d_pad_input src/d_pad_input.cpp)
target_include_directories(d_pad_input PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(d_pad_input PUBLIC c_std_99 cxx_std_17)
ament_target_dependencies(d_pad_input "rclcpp" "sensor_msgs" "std_msgs")

add_executable(trigger_input src/trigger_input.cpp)
target_include_directories(trigger_input PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_features(trigger_input PUBLIC c_std_99 cxx_std_17)
ament_target_dependencies(trigger_input "rclcpp" "sensor_msgs" "std_msgs")

install(TARGETS joy_input btn_input d_pad_input trigger_input
  DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
