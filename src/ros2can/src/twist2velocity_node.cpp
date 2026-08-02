#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <ros2can/msg/robot_velocity_command.hpp>

class Twist2VelocityNode : public rclcpp::Node
{
public:
  Twist2VelocityNode() : Node("twist2velocity_node")
  {
    publisher_ = this->create_publisher<ros2can::msg::RobotVelocityCommand>("robot_vel", 10);
    subscription_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "cmd_vel", 10, std::bind(&Twist2VelocityNode::twist_callback, this, std::placeholders::_1));
    RCLCPP_INFO(this->get_logger(), "Twist2Velocity Node started, listening to cmd_vel.");
  }

private:
  void twist_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    ros2can::msg::RobotVelocityCommand cmd;
    cmd.vx = msg->linear.x;
    cmd.vy = msg->linear.y;
    cmd.omega = msg->angular.z;
    publisher_->publish(cmd);
  }

  rclcpp::Publisher<ros2can::msg::RobotVelocityCommand>::SharedPtr publisher_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr subscription_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Twist2VelocityNode>());
  rclcpp::shutdown();
  return 0;
}
