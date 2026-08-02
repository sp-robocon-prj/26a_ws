#ifndef ROS2CAN_BASE_HANDLER_HPP_
#define ROS2CAN_BASE_HANDLER_HPP_

#include <rclcpp/rclcpp.hpp>
#include <functional>
#include "ros2can/can_frame_types.hpp"

namespace ros2can {

using CanSendCallback = std::function<void(const CanRegisterFrame&)>;

class BaseHandler {
public:
    virtual ~BaseHandler() = default;

    virtual void init(rclcpp::Node* node, CanSendCallback send_cb) {
        node_ = node;
        send_can_ = send_cb;
    }

    virtual bool handle_can_frame(const CanRegisterFrame& frame) = 0;

protected:
    rclcpp::Node* node_ = nullptr;
    CanSendCallback send_can_;
};

}

#endif
