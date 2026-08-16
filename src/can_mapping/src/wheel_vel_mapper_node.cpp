#include <rclcpp/rclcpp.hpp>
#include <controller/msg/wheel_velocity_command.hpp>
#include <ros2can/msg/udp_can_frame.hpp>
#include <vector>
#include <cstring>

using namespace std;

class WheelVelMapperNode : public rclcpp::Node
{
public:
  WheelVelMapperNode() : Node("wheel_vel_mapper")
  {
    // 各車輪に対応するBoard Num
    this->declare_parameter<std::vector<int64_t>>("board_nums", {0, 1, 2, 3});
    // 優先度
    this->declare_parameter<int>("priority", 3);
    // Data Type (4250_BLDC_DRIVER)
    this->declare_parameter<int>("data_type", 0x0A);
    // Register ID (RPS_TARGET)
    this->declare_parameter<int>("register_id", 0x0013);

    board_nums_ = this->get_parameter("board_nums").as_integer_array();
    priority_ = this->get_parameter("priority").as_int();
    data_type_ = this->get_parameter("data_type").as_int();
    register_id_ = this->get_parameter("register_id").as_int();

    publisher_ = this->create_publisher<ros2can::msg::UdpCanFrame>("/udp_can_tx", 10);
    subscriber_ = this->create_subscription<controller::msg::WheelVelocityCommand>(
      "wheel_vel", 10, std::bind(&WheelVelMapperNode::wheel_vel_callback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "Wheel Velocity to CAN mapper started.");
  }

private:
  void wheel_vel_callback(const controller::msg::WheelVelocityCommand::SharedPtr msg)
  {
    for (size_t i = 0; i < msg->velocities.size() && i < board_nums_.size(); ++i) {
      ros2can::msg::UdpCanFrame frame;
      frame.priority = priority_;
      frame.data_type = data_type_;
      frame.board_num = board_nums_[i];
      frame.register_id = register_id_;
      frame.dlc = 4;

      float target_rps = static_cast<float>(msg->velocities[i]);
      
      std::memcpy(frame.data.data(), &target_rps, sizeof(float));

      publisher_->publish(frame);
    }
  }

  rclcpp::Subscription<controller::msg::WheelVelocityCommand>::SharedPtr subscriber_;
  rclcpp::Publisher<ros2can::msg::UdpCanFrame>::SharedPtr publisher_;

  std::vector<int64_t> board_nums_;
  int priority_;
  int data_type_;
  int register_id_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(make_shared<WheelVelMapperNode>());
  rclcpp::shutdown();
  return 0;
}
