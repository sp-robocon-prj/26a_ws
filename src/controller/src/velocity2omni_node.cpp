#include <rclcpp/rclcpp.hpp>
#include <controller/msg/robot_velocity_command.hpp>
#include <controller/msg/wheel_velocity_command.hpp>

#include <cmath>

class Velocity2OmniNode : public rclcpp::Node
{
public:
  Velocity2OmniNode() : Node("velocity2omni_node")
  {
    this->declare_parameter<std::string>("chassis_type", "omni4_x");

    publisher_ = this->create_publisher<controller::msg::WheelVelocityCommand>("wheel_vel", 10);
    subscription_ = this->create_subscription<controller::msg::RobotVelocityCommand>(
      "robot_vel", 10, std::bind(&Velocity2OmniNode::robot_vel_callback, this, std::placeholders::_1));
    RCLCPP_INFO(this->get_logger(), "Velocity2Omni Node started. chassis_type can be changed via parameter.");
  }

private:
  void robot_vel_callback(const controller::msg::RobotVelocityCommand::SharedPtr msg)
  {
    controller::msg::WheelVelocityCommand cmd;
    
    // Scale : vx = 1.0 => wheels = 100
    double scale = 100.0;
    
    std::string chassis_type = this->get_parameter("chassis_type").as_string();
    
    if (chassis_type == "omni3") {
      cmd.velocities.resize(3);
      cmd.velocities[0] = scale * (-msg->vx * 0.0 + msg->vy * 1.0 + msg->omega);
      cmd.velocities[1] = scale * (msg->vx * (std::sqrt(3.0)/2.0) - msg->vy * 0.5 + msg->omega);
      cmd.velocities[2] = scale * (-msg->vx * (std::sqrt(3.0)/2.0) - msg->vy * 0.5 + msg->omega);
    } 
    else if (chassis_type == "omni4_x" || chassis_type == "mecanum") {
      cmd.velocities.resize(4);
      cmd.velocities[0] = scale * (msg->vx - msg->vy - msg->omega); // fl
      cmd.velocities[1] = scale * (msg->vx + msg->vy + msg->omega); // fr
      cmd.velocities[2] = scale * (msg->vx + msg->vy - msg->omega); // rl
      cmd.velocities[3] = scale * (msg->vx - msg->vy + msg->omega); // rr
    }
    else {
      RCLCPP_WARN_ONCE(this->get_logger(), "Unknown chassis_type: %s. Using omni4_x as fallback.", chassis_type.c_str());
      cmd.velocities.resize(4);
      cmd.velocities[0] = scale * (msg->vx - msg->vy - msg->omega);
      cmd.velocities[1] = scale * (msg->vx + msg->vy + msg->omega);
      cmd.velocities[2] = scale * (msg->vx + msg->vy - msg->omega);
      cmd.velocities[3] = scale * (msg->vx - msg->vy + msg->omega);
    }
    
    publisher_->publish(cmd);
  }

  rclcpp::Publisher<controller::msg::WheelVelocityCommand>::SharedPtr publisher_;
  rclcpp::Subscription<controller::msg::RobotVelocityCommand>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Velocity2OmniNode>());
  rclcpp::shutdown();
  return 0;
}
