#include <ros2can_node.hpp>


ROS2CAN_Node::ROS2CAN_Node() : Node("udp_bridge_node"), sockfd_(-1), is_running_(true) {
    this->declare_parameter<std::string>("local_ip", "0.0.0.0");
    this->declare_parameter<int>("local_port", 4001);
    this->declare_parameter<std::string>("remote_ip", "192.168.10.103");
    this->declare_parameter<int>("remote_port", 4001);

    std::string local_ip = this->get_parameter("local_ip").as_string();
    int local_port = this->get_parameter("local_port").as_int();
    remote_ip_ = this->get_parameter("remote_ip").as_string();
    remote_port_ = this->get_parameter("remote_port").as_int();

    can_rx_publisher_ = this->create_publisher<ros2can::msg::UdpCanFrame>("udp_can_rx", 10);

    can_tx_subscription_ = this->create_subscription<ros2can::msg::UdpCanFrame>(
        "udp_can_tx", 10, std::bind(&ROS2CAN_Node::tx_callback, this, std::placeholders::_1)
    );

    bldc_rx_publisher_ = this->create_publisher<ros2can::msg::BLDCDriver>("BLDC_RX", 10);
    bldc_tx_subscription_ = this->create_subscription<ros2can::msg::BLDCDriver>(
            "BLDC_TX", 10, std::bind(&ROS2CAN_Node::BLDC_callback, this, std::placeholders::_1)
    );

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
    rx_thread_ = std::thread(&ROS2CAN_Node::rx_thread_func, this);
}

ROS2CAN_Node::~ROS2CAN_Node() {
    is_running_ = false;
    if (rx_thread_.joinable()) {
        rx_thread_.join();
    }
    if (sockfd_ >= 0) {
        close(sockfd_);
    }
}

void ROS2CAN_Node::tx_callback(const ros2can::msg::UdpCanFrame::SharedPtr msg) {
    if (sockfd_ < 0) return;

    UdpPacket packet;
    std::memset(&packet, 0, sizeof(packet));
    packet.id = msg->id;
    packet.size = msg->size;
    memcpy(packet.data, msg->data.data(), std::min(static_cast<size_t>(msg->size), sizeof(packet.data)));
    struct sockaddr_in remote_addr;
    std::memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port_);
    remote_addr.sin_addr.s_addr = inet_addr(remote_ip_.c_str());

    ssize_t sent_bytes = sendto(sockfd_, &packet, sizeof(packet), 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr));
        
    if (sent_bytes < 0) {
        RCLCPP_WARN(this->get_logger(), "Failed to send UDP packet");
    }
}

void ROS2CAN_Node::BLDC_callback(const ros2can::msg::BLDCDriver::SharedPtr msg) {
  RCLCPP_WARN(this->get_logger(), "Received BLDCDriver message: board_num=%d, rps_target=%d, angle_target=%d",
        msg->board_num, msg->rps_target, msg->angle_target);
}

void ROS2CAN_Node::rx_thread_func() {
    UdpPacket packet;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (is_running_ && rclcpp::ok()) {
        ssize_t n = recvfrom(sockfd_, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &client_len);
        if (n == sizeof(packet)) {
            ros2can::msg::UdpCanFrame msg;
            msg.id = packet.id;
            msg.size = packet.size;
            memcpy(msg.data.data(), packet.data, 32);
            can_rx_publisher_->publish(msg);
        } else if (n > 0) {
            RCLCPP_WARN(this->get_logger(), "Received packet of unexpected size: %zd bytes (expected %zu)", n, sizeof(packet));
        }
    }
}

void ROS2CAN_Node::tx_packet() 
{
        // This function is not used in the current implementation.
}

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ROS2CAN_Node>());
    rclcpp::shutdown();
    return 0;
}
