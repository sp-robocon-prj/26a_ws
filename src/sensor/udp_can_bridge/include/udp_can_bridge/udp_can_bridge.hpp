#pragma once

#include <rclcpp/rclcpp.hpp>
#include <memory>
#include "udp_can_bridge/udp_manager.hpp"

class UDPCANBridge : public rclcpp::Node
{
public:
    UDPCANBridge(std::shared_ptr<UdpManager> udp_manager);
    ~UDPCANBridge() override = default;

private:
    std::shared_ptr<UdpManager> udp_manager_;
    
    // 将来の拡張用：
    // rclcpp::Subscription<...>::SharedPtr sub_;
    // rclcpp::Publisher<...>::SharedPtr pub_;
    // rclcpp::TimerBase::SharedPtr timer_;
};
