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
    uint8_t  priority;     
    uint8_t  data_type;    
    uint8_t  board_num;    
    uint16_t register_id;  
    uint8_t  dlc;          
    uint8_t  data[64];     
};
#pragma pack(pop)

class UdpBridgeNode : public rclcpp::Node {
public:
    UdpBridgeNode() : Node("udp_bridge_node"), sockfd_(-1), is_running_(true) {
        std::string local_ip = "192.168.1.102";
        int local_port = 4001;
        remote_ip_ = "192.168.1.103";
        remote_port_ = 4001;

        publisher_ = this->create_publisher<ros2can::msg::UdpCanFrame>("udp_can_rx", 10);
        subscription_ = this->create_subscription<ros2can::msg::UdpCanFrame>(
            "udp_can_tx", 10, std::bind(&UdpBridgeNode::tx_callback, this, std::placeholders::_1));

        // Setup UDP Socket
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ < 0) {
            RCLCPP_ERROR(this->get_logger(), "Failed to create socket.");
            return;
        }

        struct sockaddr_in local_addr;
        std::memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = inet_addr(local_ip.c_str());
        local_addr.sin_port = htons(local_port);

        if (bind(sockfd_, (const struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
            RCLCPP_ERROR(this->get_logger(), "Failed to bind socket to %s:%d", local_ip.c_str(), local_port);
            close(sockfd_);
            sockfd_ = -1;
            return;
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        RCLCPP_INFO(this->get_logger(), "UDP Bridge started. Bound to %s:%d, sending to %s:%d",
                    local_ip.c_str(), local_port, remote_ip_.c_str(), remote_port_);

        // Start receiver thread
        rx_thread_ = std::thread(&UdpBridgeNode::rx_thread_func, this);
    }

    ~UdpBridgeNode() {
        is_running_ = false;
        if (rx_thread_.joinable()) {
            rx_thread_.join();
        }
        if (sockfd_ >= 0) {
            close(sockfd_);
        }
    }

private:
    int sockfd_;
    bool is_running_;
    std::string remote_ip_;
    int remote_port_;
    std::thread rx_thread_;
    rclcpp::Publisher<ros2can::msg::UdpCanFrame>::SharedPtr publisher_;
    rclcpp::Subscription<ros2can::msg::UdpCanFrame>::SharedPtr subscription_;

    void tx_callback(const ros2can::msg::UdpCanFrame::SharedPtr msg) {
        if (sockfd_ < 0) return;

        UdpPacket packet;
        std::memset(&packet, 0, sizeof(packet));
        packet.priority = msg->priority;
        packet.data_type = msg->data_type;
        packet.board_num = msg->board_num;
        packet.register_id = msg->register_id;
        packet.dlc = msg->dlc;
        
        for (size_t i = 0; i < 64 && i < msg->data.size(); ++i) {
            packet.data[i] = msg->data[i];
        }

        struct sockaddr_in remote_addr;
        std::memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(remote_port_);
        remote_addr.sin_addr.s_addr = inet_addr(remote_ip_.c_str());

        ssize_t sent_bytes = sendto(sockfd_, &packet, sizeof(packet), 0,
                                    (const struct sockaddr *)&remote_addr, sizeof(remote_addr));
        
        if (sent_bytes < 0) {
            RCLCPP_WARN(this->get_logger(), "Failed to send UDP packet");
        }
    }

    void rx_thread_func() {
        UdpPacket packet;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        while (is_running_ && rclcpp::ok()) {
            ssize_t n = recvfrom(sockfd_, &packet, sizeof(packet), 0,
                                 (struct sockaddr *)&client_addr, &client_len);
            
            if (n == sizeof(packet)) {
                ros2can::msg::UdpCanFrame msg;
                msg.priority = packet.priority;
                msg.data_type = packet.data_type;
                msg.board_num = packet.board_num;
                msg.register_id = packet.register_id;
                msg.dlc = packet.dlc;
                for (size_t i = 0; i < 64; ++i) {
                    msg.data[i] = packet.data[i];
                }
                publisher_->publish(msg);
            } else if (n > 0) {
                RCLCPP_WARN(this->get_logger(), "Received packet of unexpected size: %zd bytes (expected %zu)", n, sizeof(packet));
            }
        }
    }
};

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<UdpBridgeNode>());
    rclcpp::shutdown();
    return 0;
}
