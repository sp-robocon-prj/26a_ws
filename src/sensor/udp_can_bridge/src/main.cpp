#include <rclcpp/rclcpp.hpp>
#include <memory>
#include <iostream>
#include "udp_can_bridge/udp_manager.hpp"
#include "udp_can_bridge/udp_can_bridge.hpp"

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);

    // ハードコードされた接続先IPとポート
    const std::string target_ip = "127.0.0.1"; // h723のIPアドレス
    const uint16_t target_port = 8888;         // h723のポート番号
    const uint16_t local_port = 0;             // ローカルポート（0の場合はOSが自動割り当て）

    std::shared_ptr<UdpManager> udp_manager;
    try {
        udp_manager = std::make_shared<UdpManager>(target_ip, target_port, local_port);
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize UdpManager: " << e.what() << std::endl;
        rclcpp::shutdown();
        return 1;
    }

    udp_manager->wait_for_connection();

    // 接続確認後、ノードを登録・実行
    auto node = std::make_shared<UDPCANBridge>(udp_manager);
    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}
