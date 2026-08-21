#ifndef ROS2CAN_ROS2CAN_NODE_HPP_
#define ROS2CAN_ROS2CAN_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <map>
#include <memory>
#include "ros2can/can_frame_types.hpp"
#include "ros2can/base_handler.hpp"

namespace ros2can {

class Ros2CanNode : public rclcpp::Node {
public:
    Ros2CanNode();

    void register_handler(uint8_t data_type, std::shared_ptr<BaseHandler> handler);

    void on_can_received(uint32_t can_id, const std::vector<uint8_t>& data);

private:
    void send_can_frame(const CanRegisterFrame& frame);

    std::map<uint8_t, std::shared_ptr<BaseHandler>> handlers_;
};

}

#endif