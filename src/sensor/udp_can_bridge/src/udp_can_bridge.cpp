#include "udp_can_bridge/udp_can_bridge.hpp"

UDPCANBridge::UDPCANBridge(std::shared_ptr<UdpManager> udp_manager)
    : Node("udp_can_bridge"), udp_manager_(udp_manager)
{
    RCLCPP_INFO(this->get_logger(), "UDPCANBridge node initialized and active!");
    // ここで将来的なROS2トピックのサブスクライブやパブリッシュ、タイマー処理を初期化します。
}