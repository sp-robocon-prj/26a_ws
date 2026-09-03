#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <std_msgs/msg/bool.hpp>

using namespace std;

class JoyInputNode : public rclcpp::Node
{
public:
  const string send_topic = "/cmd_vel";
  JoyInputNode() : Node("joy_input")
  { 
    this->declare_parameter<int>("linear_x_axis", 1); // 左スティック 縦
    this->declare_parameter<int>("linear_y_axis", 0); // 左スティック 横
    this->declare_parameter<int>("angular_z_axis", 2); // 右スティック 横
    this->declare_parameter<double>("linear_scale", 1.0);
    this->declare_parameter<double>("angular_scale", 1.0);
    
    linear_x_axis_ = this->get_parameter("linear_x_axis").as_int();
    linear_y_axis_ = this->get_parameter("linear_y_axis").as_int();
    angular_z_axis_ = this->get_parameter("angular_z_axis").as_int();
    linear_scale_ = this->get_parameter("linear_scale").as_double();
    angular_scale_ = this->get_parameter("angular_scale").as_double();

    publisher_ = this->create_publisher<geometry_msgs::msg::Twist>(send_topic, 10);
    subscriber_ = this->create_subscription<sensor_msgs::msg::Joy>(
      "joy", 10, std::bind(&JoyInputNode::joy_callback, this, std::placeholders::_1));
      
    RCLCPP_INFO(this->get_logger(), "Joy input node started. Publishing to %s.", send_topic.c_str());
  }

private:
  void joy_callback(const sensor_msgs::msg::Joy::SharedPtr msg)
  {
    int x_axis = this->get_parameter("linear_x_axis").as_int();
    int y_axis = this->get_parameter("linear_y_axis").as_int();
    int z_axis = this->get_parameter("angular_z_axis").as_int();
    double l_scale = this->get_parameter("linear_scale").as_double();
    double a_scale = this->get_parameter("angular_scale").as_double();

    auto twist = geometry_msgs::msg::Twist();

    if (x_axis >= 0 && static_cast<size_t>(x_axis) < msg->axes.size()) {
      twist.linear.x = msg->axes[x_axis] * l_scale;
    }

    if (y_axis >= 0 && static_cast<size_t>(y_axis) < msg->axes.size()) {
      twist.linear.y = msg->axes[y_axis] * l_scale;
    }

    if (z_axis >= 0 && static_cast<size_t>(z_axis) < msg->axes.size()) {
      twist.angular.z = msg->axes[z_axis] * a_scale;
    }

    publisher_->publish(twist);
  }

  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr subscriber_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;

  int linear_x_axis_;
  int linear_y_axis_;
  int angular_z_axis_;
  double linear_scale_;
  double angular_scale_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(make_shared<JoyInputNode>());
  rclcpp::shutdown();
  return 0;
}
