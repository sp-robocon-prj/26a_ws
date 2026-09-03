#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <std_msgs/msg/float32.hpp>

using namespace std;

class TriggerInputNode : public rclcpp::Node
{
public:
  TriggerInputNode() : Node("trigger_input")
  {
    this->declare_parameter<int>("lt_axis", 5);
    this->declare_parameter<int>("rt_axis", 4);

    lt_axis_ = this->get_parameter("lt_axis").as_int();
    rt_axis_ = this->get_parameter("rt_axis").as_int();

    pub_lt_ = this->create_publisher<std_msgs::msg::Float32>("/joy/trigger_lt", 10);
    pub_rt_ = this->create_publisher<std_msgs::msg::Float32>("/joy/trigger_rt", 10);

    subscriber_ = this->create_subscription<sensor_msgs::msg::Joy>(
      "joy", 10, std::bind(&TriggerInputNode::joy_callback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "Trigger input node started (Analog axis mode).");
  }

private:
  void joy_callback(const sensor_msgs::msg::Joy::SharedPtr msg)
  {
    std_msgs::msg::Float32 msg_lt, msg_rt;
    msg_lt.data = 0.0f;
    msg_rt.data = 0.0f;

    if (lt_axis_ >= 0 && static_cast<size_t>(lt_axis_) < msg->axes.size()) {
      msg_lt.data = (msg->axes[lt_axis_] - 1.0f) / 2.0f;
    }

    if (rt_axis_ >= 0 && static_cast<size_t>(rt_axis_) < msg->axes.size()) {
      msg_rt.data = (msg->axes[rt_axis_] - 1.0f) / 2.0f;
    }

    pub_lt_->publish(msg_lt);
    pub_rt_->publish(msg_rt);
  }

  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr subscriber_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr pub_lt_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr pub_rt_;

  int lt_axis_;
  int rt_axis_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(make_shared<TriggerInputNode>());
  rclcpp::shutdown();
  return 0;
}
