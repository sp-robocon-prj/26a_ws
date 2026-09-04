#pragma once

#include <rclcpp/rclcpp.hpp>
#include <controller/msg/robot_velocity_command.hpp>
#include <controller/msg/wheel_velocity_command.hpp>
#include <ros2can/msg/bldctx.hpp>

class Velocity2OmniNode : public rclcpp::Node
{
public:
  Velocity2OmniNode();
private:
  void robot_vel_callback(const controller::msg::RobotVelocityCommand::SharedPtr msg);

  rclcpp::Publisher<controller::msg::WheelVelocityCommand>::SharedPtr publisher_;
  rclcpp::Publisher<ros2can::msg::BLDCTX>::SharedPtr bldc_tx_;
  rclcpp::Subscription<controller::msg::RobotVelocityCommand>::SharedPtr subscription_;
};