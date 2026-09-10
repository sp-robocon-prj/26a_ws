#include "velocity2omni_node.hpp"

#include <cmath>

Velocity2OmniNode::Velocity2OmniNode() : Node("velocity2omni_node", rclcpp::NodeOptions().use_intra_process_comms(true))
{
  this->declare_parameter<std::string>("chassis_type", "omni4_x");
  this->declare_parameter<double>("scale", 1.0);
  this->declare_parameter<bool>("publish_gazebo", true);

  publisher_ = this->create_publisher<controller::msg::WheelVelocityCommand>("wheel_vel", 10);

  bldc_tx_ = this->create_publisher<ros2can::msg::BLDCTX>("BLDC/TX", 10);

  subscription_ = this->create_subscription<controller::msg::RobotVelocityCommand>(
    "robot_vel", 10, std::bind(&Velocity2OmniNode::robot_vel_callback, this, std::placeholders::_1));
  RCLCPP_INFO(this->get_logger(), "Velocity2Omni Node started. chassis_type and scale can be configured.");
}


void Velocity2OmniNode::robot_vel_callback(const controller::msg::RobotVelocityCommand::SharedPtr msg)
{
  controller::msg::WheelVelocityCommand cmd;
    
  double scale = this->get_parameter("scale").as_double();
  std::string chassis_type = this->get_parameter("chassis_type").as_string();
  bool publish_gazebo = this->get_parameter("publish_gazebo").as_bool();
    
  if (chassis_type == "omni3") {
    cmd.velocities.resize(3);
    cmd.velocities[0] = scale * (-msg->vx * 0.0 + msg->vy * 1.0 + msg->omega);
    cmd.velocities[1] = scale * (msg->vx * (std::sqrt(3.0)/2.0) - msg->vy * 0.5 + msg->omega);
    cmd.velocities[2] = scale * (-msg->vx * (std::sqrt(3.0)/2.0) - msg->vy * 0.5 + msg->omega);
  } 
  else if (chassis_type == "omni4_x" || chassis_type == "mecanum") {
    cmd.velocities.resize(4);
    // Wheel Order: 1=FL(前左), 2=FR(前右), 3=RR(後右), 4=RL(後左)
    cmd.velocities[0] = scale * (-msg->vx + msg->vy + msg->omega); // omni_1: FL (前左)
    cmd.velocities[1] = scale * ( msg->vx + msg->vy + msg->omega); // omni_2: FR (前右)
    cmd.velocities[2] = scale * ( msg->vx - msg->vy + msg->omega); // omni_3: RR (後右)
    cmd.velocities[3] = scale * (-msg->vx - msg->vy + msg->omega); // omni_4: RL (後左)
  } else {
    RCLCPP_WARN_ONCE(this->get_logger(), "Unknown chassis_type: %s. Using omni4_x as fallback.", chassis_type.c_str());
    cmd.velocities.resize(4);
    cmd.velocities[0] = scale * (-msg->vx + msg->vy + msg->omega);
    cmd.velocities[1] = scale * ( msg->vx + msg->vy + msg->omega);
    cmd.velocities[2] = scale * ( msg->vx - msg->vy + msg->omega);
    cmd.velocities[3] = scale * (-msg->vx - msg->vy + msg->omega);
  }
    
  publisher_->publish(cmd);

  if (publish_gazebo && cmd.velocities.size() >= 4) {
    ros2can::msg::BLDCTX TX;
    TX.priority = 1; // Set priority as needed
    TX.gear_ratio = 19.2;
    TX.encoder_resolution = 4096;
    
    TX.board_num = 0; // Set board number as needed
    TX.rps_target = cmd.velocities[0]; // omni_1: FL (前左)
    bldc_tx_->publish(TX);

    TX.board_num = 1; // Set board number as needed
    TX.rps_target = cmd.velocities[1]; // omni_2: FR (前右)
    bldc_tx_->publish(TX);

    TX.board_num = 2; // Set board number as needed
    TX.rps_target = cmd.velocities[2]; // omni_3: RR (後右)
    bldc_tx_->publish(TX);

    TX.board_num = 3; // Set board number as needed
    TX.rps_target = cmd.velocities[3]; // omni_4: RL (後左)
    bldc_tx_->publish(TX);
  }
}

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Velocity2OmniNode>());
  rclcpp::shutdown();
  return 0;
}
