#include <rclcpp/rclcpp.hpp>
#include <ros2can/msg/udp_can_frame.hpp>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <iostream>

#pragma pack(push, 1)
struct UdpPacket {
    uint32_t id;
    uint32_t size;          
    uint8_t  data[64];     
};
#pragma pack(pop)

class UdpBridgeNode : public rclcpp::Node {
public:
    UdpBridgeNode();
    ~UdpBridgeNode();
private:
    int sockfd_;
    bool is_running_;
    std::string remote_ip_;
    int remote_port_;
    std::thread rx_thread_;
    rclcpp::Publisher<ros2can::msg::UdpCanFrame>::SharedPtr publisher_;
    rclcpp::Subscription<ros2can::msg::UdpCanFrame>::SharedPtr subscription_;

    void tx_packet();
    void tx_callback(const ros2can::msg::UdpCanFrame::SharedPtr msg);
    void rx_thread_func();
};