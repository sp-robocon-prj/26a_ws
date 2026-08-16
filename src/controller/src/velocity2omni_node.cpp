#include <rclcpp/rclcpp.hpp>
#include <controller/msg/robot_velocity_command.hpp>
#include <controller/msg/wheel_velocity_command.hpp>
#include <std_msgs/msg/float64.hpp>

#include <cmath>

class Velocity2OmniNode : public rclcpp::Node
{
public:
  Velocity2OmniNode() : Node("velocity2omni_node")
  {
    this->declare_parameter<std::string>("chassis_type", "omni4_x");
    this->declare_parameter<double>("scale", 10.0);
    this->declare_parameter<bool>("publish_gazebo", true);

    publisher_ = this->create_publisher<controller::msg::WheelVelocityCommand>("wheel_vel", 10);

    pub_omni1_ = this->create_publisher<std_msgs::msg::Float64>("omni_1/cmd_vel", 10);
    pub_omni2_ = this->create_publisher<std_msgs::msg::Float64>("omni_2/cmd_vel", 10);
    pub_omni3_ = this->create_publisher<std_msgs::msg::Float64>("omni_3/cmd_vel", 10);
    pub_omni4_ = this->create_publisher<std_msgs::msg::Float64>("omni_4/cmd_vel", 10);

    subscription_ = this->create_subscription<controller::msg::RobotVelocityCommand>(
      "robot_vel", 10, std::bind(&Velocity2OmniNode::robot_vel_callback, this, std::placeholders::_1));
    RCLCPP_INFO(this->get_logger(), "Velocity2Omni Node started. chassis_type and scale can be configured.");
  }

private:
  void robot_vel_callback(const controller::msg::RobotVelocityCommand::SharedPtr msg)
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
    }
    else {
      RCLCPP_WARN_ONCE(this->get_logger(), "Unknown chassis_type: %s. Using omni4_x as fallback.", chassis_type.c_str());
      cmd.velocities.resize(4);
      cmd.velocities[0] = scale * (-msg->vx + msg->vy + msg->omega);
      cmd.velocities[1] = scale * ( msg->vx + msg->vy + msg->omega);
      cmd.velocities[2] = scale * ( msg->vx - msg->vy + msg->omega);
      cmd.velocities[3] = scale * (-msg->vx - msg->vy + msg->omega);
    }
    
    publisher_->publish(cmd);

    if (publish_gazebo && cmd.velocities.size() >= 4) {
      std_msgs::msg::Float64 v1, v2, v3, v4;
      v1.data = cmd.velocities[0]; // omni_1: FL (前左)
      v2.data = cmd.velocities[1]; // omni_2: FR (前右)
      v3.data = cmd.velocities[2]; // omni_3: RR (後右)
      v4.data = cmd.velocities[3]; // omni_4: RL (後左)

      pub_omni1_->publish(v1);
      pub_omni2_->publish(v2);
      pub_omni3_->publish(v3);
      pub_omni4_->publish(v4);
    }
  }

  rclcpp::Publisher<controller::msg::WheelVelocityCommand>::SharedPtr publisher_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr pub_omni1_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr pub_omni2_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr pub_omni3_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr pub_omni4_;
  rclcpp::Subscription<controller::msg::RobotVelocityCommand>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Velocity2OmniNode>());
  rclcpp::shutdown();
  return 0;
}
