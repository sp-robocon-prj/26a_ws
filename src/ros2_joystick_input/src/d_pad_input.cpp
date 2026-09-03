#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <std_msgs/msg/bool.hpp>

using namespace std;

class DPadInputNode : public rclcpp::Node
{
public:
  DPadInputNode() : Node("d_pad_input")
  {
    this->declare_parameter<int>("dpad_x_axis", 6); // 左右
    this->declare_parameter<int>("dpad_y_axis", 7); // 上下

    dpad_x_axis_ = this->get_parameter("dpad_x_axis").as_int();
    dpad_y_axis_ = this->get_parameter("dpad_y_axis").as_int();

    pub_up_ = this->create_publisher<std_msgs::msg::Bool>("/joy/dpad_up", 10);
    pub_down_ = this->create_publisher<std_msgs::msg::Bool>("/joy/dpad_down", 10);
    pub_left_ = this->create_publisher<std_msgs::msg::Bool>("/joy/dpad_left", 10);
    pub_right_ = this->create_publisher<std_msgs::msg::Bool>("/joy/dpad_right", 10);

    subscriber_ = this->create_subscription<sensor_msgs::msg::Joy>(
      "joy", 10, std::bind(&DPadInputNode::joy_callback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "D-Pad input node started.");
  }

private:
  void joy_callback(const sensor_msgs::msg::Joy::SharedPtr msg)
  {
    std_msgs::msg::Bool msg_up, msg_down, msg_left, msg_right;
    msg_up.data = false;
    msg_down.data = false;
    msg_left.data = false;
    msg_right.data = false;

    if (dpad_y_axis_ >= 0 && static_cast<size_t>(dpad_y_axis_) < msg->axes.size()) {
      float y_val = msg->axes[dpad_y_axis_];
      msg_up.data = (y_val > 0.5);
      msg_down.data = (y_val < -0.5);
    }

    if (dpad_x_axis_ >= 0 && static_cast<size_t>(dpad_x_axis_) < msg->axes.size()) {
      float x_val = msg->axes[dpad_x_axis_];
      msg_left.data = (x_val > 0.5);
      msg_right.data = (x_val < -0.5);
    }

    pub_up_->publish(msg_up);
    pub_down_->publish(msg_down);
    pub_left_->publish(msg_left);
    pub_right_->publish(msg_right);
  }

  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr subscriber_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr pub_up_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr pub_down_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr pub_left_;
  rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr pub_right_;

  int dpad_x_axis_;
  int dpad_y_axis_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(make_shared<DPadInputNode>());
  rclcpp::shutdown();
  return 0;
}
