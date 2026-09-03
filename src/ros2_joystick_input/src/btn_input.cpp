#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <std_msgs/msg/bool.hpp>
#include <vector>
#include <string>

using namespace std;

class BtnInputNode : public rclcpp::Node
{
public:
  BtnInputNode() : Node("btn_input")
  {
    this->declare_parameter<int>("btn_a_id", 0);
    this->declare_parameter<int>("btn_b_id", 1);
    this->declare_parameter<int>("btn_x_id", 3);
    this->declare_parameter<int>("btn_y_id", 4);
    this->declare_parameter<int>("btn_lb_id", 6);
    this->declare_parameter<int>("btn_rb_id", 7);
    this->declare_parameter<int>("btn_10_id", 10);
    this->declare_parameter<int>("btn_11_id", 11);
    this->declare_parameter<int>("btn_home_id", 12);

    std::vector<std::string> names = {"a", "b", "x", "y", "lb", "rb", "10", "11", "home"};
    for (const auto& name : names) {
      int btn_id = this->get_parameter("btn_" + name + "_id").as_int();
      auto pub = this->create_publisher<std_msgs::msg::Bool>("/joy/btn_" + name, 10);
      btn_map_.push_back({btn_id, pub});
    }

    subscriber_ = this->create_subscription<sensor_msgs::msg::Joy>(
      "joy", 10, std::bind(&BtnInputNode::joy_callback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "Button input node started with configurable indices.");
  }

private:
  void joy_callback(const sensor_msgs::msg::Joy::SharedPtr msg)
  {
    std_msgs::msg::Bool btn_msg;
    for (const auto& item : btn_map_) {
      int btn_id = item.first;
      if (btn_id >= 0 && static_cast<size_t>(btn_id) < msg->buttons.size()) {
        btn_msg.data = msg->buttons[btn_id] != 0;
        item.second->publish(btn_msg);
      }
    }
  }

  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr subscriber_;
  std::vector<std::pair<int, rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr>> btn_map_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(make_shared<BtnInputNode>());
  rclcpp::shutdown();
  return 0;
}