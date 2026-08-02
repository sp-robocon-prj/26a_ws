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

    // デバイスタイプごとにハンドラを登録する
    void register_handler(uint8_t data_type, std::shared_ptr<BaseHandler> handler);

    // 擬似的なCAN受信関数（実際のCAN Socketからのコールバックとして呼ばれる想定）
    void on_can_received(uint32_t can_id, const std::vector<uint8_t>& data);

private:
    // CAN送信関数（各ハンドラにコールバックとして渡す）
    void send_can_frame(const CanRegisterFrame& frame);

    // DataTypeをキーにしてハンドラを保持
    std::map<uint8_t, std::shared_ptr<BaseHandler>> handlers_;
};

} // namespace ros2can

#endif // ROS2CAN_ROS2CAN_NODE_HPP_
