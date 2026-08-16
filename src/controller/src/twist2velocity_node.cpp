#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/bool.hpp>
#include <std_msgs/msg/empty.hpp>
#include <controller/msg/robot_velocity_command.hpp>
#include <cmath>

class Twist2VelocityNode : public rclcpp::Node
{
public:
  Twist2VelocityNode() : Node("twist2velocity_node")
  {
    this->declare_parameter<bool>("use_global_frame", true);
    this->declare_parameter<std::string>("odom_topic", "/odom");

    use_global_frame_ = this->get_parameter("use_global_frame").as_bool();
    std::string odom_topic = this->get_parameter("odom_topic").as_string();

    publisher_ = this->create_publisher<controller::msg::RobotVelocityCommand>("robot_vel", 10);

    twist_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "cmd_vel", 10, std::bind(&Twist2VelocityNode::twist_callback, this, std::placeholders::_1));

    odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
      odom_topic, 10, std::bind(&Twist2VelocityNode::odom_callback, this, std::placeholders::_1));

    reset_sub_ = this->create_subscription<std_msgs::msg::Empty>(
      "reset_heading", 10, std::bind(&Twist2VelocityNode::reset_heading_callback, this, std::placeholders::_1));

    toggle_mode_sub_ = this->create_subscription<std_msgs::msg::Bool>(
      "toggle_global_frame", 10, std::bind(&Twist2VelocityNode::toggle_mode_callback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "Twist2Velocity Node started (Field-Oriented Control). Global frame mode: %s",
                use_global_frame_ ? "ENABLED" : "DISABLED");
  }

private:
  void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg)
  {
    const auto & q = msg->pose.pose.orientation;
    double siny_cosp = 2.0 * (q.w * q.z + q.x * q.y);
    double cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
    current_yaw_ = std::atan2(siny_cosp, cosy_cosp);

    if (!first_odom_received_) {
      heading_offset_ = current_yaw_;
      first_odom_received_ = true;
      RCLCPP_INFO(this->get_logger(), "Initial heading offset set to: %.2f rad", heading_offset_);
    }
  }

  void reset_heading_callback(const std_msgs::msg::Empty::SharedPtr)
  {
    heading_offset_ = current_yaw_;
    RCLCPP_INFO(this->get_logger(), "Heading offset reset to current yaw: %.2f rad", heading_offset_);
  }

  void toggle_mode_callback(const std_msgs::msg::Bool::SharedPtr msg)
  {
    use_global_frame_ = msg->data;
    RCLCPP_INFO(this->get_logger(), "Global frame mode changed to: %s",
                use_global_frame_ ? "ENABLED" : "DISABLED");
  }

  void twist_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    use_global_frame_ = this->get_parameter("use_global_frame").as_bool();

    double vx_in = msg->linear.x;
    double vy_in = msg->linear.y;
    double omega = msg->angular.z;

    double vx_out = vx_in;
    double vy_out = vy_in;

    if (use_global_frame_ && first_odom_received_) {
      double eff_yaw = current_yaw_ - heading_offset_;
      vx_out =  vx_in * std::cos(eff_yaw) + vy_in * std::sin(eff_yaw);
      vy_out = -vx_in * std::sin(eff_yaw) + vy_in * std::cos(eff_yaw);
    }

    controller::msg::RobotVelocityCommand cmd;
    cmd.vx = vx_out;
    cmd.vy = vy_out;
    cmd.omega = omega;
    publisher_->publish(cmd);
  }

  rclcpp::Publisher<controller::msg::RobotVelocityCommand>::SharedPtr publisher_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr twist_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Subscription<std_msgs::msg::Empty>::SharedPtr reset_sub_;
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr toggle_mode_sub_;

  bool use_global_frame_{true};
  bool first_odom_received_{false};
  double current_yaw_{0.0};
  double heading_offset_{0.0};
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Twist2VelocityNode>());
  rclcpp::shutdown();
  return 0;
}
