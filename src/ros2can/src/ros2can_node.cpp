#include <ros2can_node.hpp>


ROS2CAN_Node::ROS2CAN_Node() : Node("ROS2CAN_Node"), sockfd_(-1), is_running_(true) {
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

    bldc_rx_publisher_ = this->create_publisher<ros2can::msg::BLDCRX>("BLDC/RX", 10);
    bldc_tx_subscription_ = this->create_subscription<ros2can::msg::BLDCTX>(
            "BLDC/TX", 10, std::bind(&ROS2CAN_Node::BLDC_callback, this, std::placeholders::_1)
    );
    
    pwr_rx_publisher_ = this->create_publisher<ros2can::msg::PWRManagerRX>("PWRManager/RX", 10);
    pwr_tx_subscription_ = this->create_subscription<ros2can::msg::PWRManagerTX>(
            "PWRManager/TX", 10, std::bind(&ROS2CAN_Node::PWR_callback, this, std::placeholders::_1)
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
    std::memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port_);
    remote_addr.sin_addr.s_addr = inet_addr(remote_ip_.c_str());
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
    ssize_t sent_bytes = sendto(sockfd_, &packet, sizeof(packet), 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr));
        
    if (sent_bytes < 0) {
        RCLCPP_WARN(this->get_logger(), "Failed to send UDP packet");
    }
}

void ROS2CAN_Node::BLDC_callback(const ros2can::msg::BLDCTX::SharedPtr msg)  {
    UdpPacket packet;
    std::memset(&packet, 0, sizeof(packet));

    ID id;
    id.fields.priority = msg->priority;
    id.fields.data_type = DataType::BLDC_COMMAND;
    id.fields.board_num = msg->board_num;

    BLDC_CANPacket bldc_packet;
    bldc_packet.mode = msg->mode;
    bldc_packet.rps_target = msg->rps_target*10;
    bldc_packet.angle_target = msg->angle_target;

    packet.id = id.id;
    packet.size = 32;
    
    memcpy(packet.data, &bldc_packet, 9);
    ssize_t sent_bytes = sendto(sockfd_, &packet, sizeof(packet), 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr));

    if (sent_bytes < 0) {
        RCLCPP_WARN(this->get_logger(), "Failed to send UDP packet");
    }
}

void ROS2CAN_Node::PWR_callback(const ros2can::msg::PWRManagerTX::SharedPtr msg)  {
    UdpPacket packet;
    std::memset(&packet, 0, sizeof(packet));

    ID id;
    id.fields.priority = msg->priority;
    id.fields.data_type = DataType::POWERBOARD_COMANND;
    id.fields.board_num = msg->board_num;

    PWRTX_CANPacket pwr_packet;
    pwr_packet.pwrstatus = msg->powerstatus;
    pwr_packet.ledstatus = msg->ledstatus;

    packet.id = id.id;
    packet.size = 32;
    
    memcpy(packet.data, &pwr_packet, sizeof(pwr_packet));
    ssize_t sent_bytes = sendto(sockfd_, &packet, sizeof(packet), 0, (const struct sockaddr *)&remote_addr, sizeof(remote_addr));

    if (sent_bytes < 0) {
        RCLCPP_WARN(this->get_logger(), "Failed to send UDP packet");
    }
}


void ROS2CAN_Node::rx_thread_func() {
    UdpPacket packet;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (is_running_ && rclcpp::ok()) {
        ssize_t n = recvfrom(sockfd_, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &client_len);
        if (n == sizeof(packet)) {
            ID id;
            id.id = packet.id;
            if (id.fields.data_type == DataType::POWERBOARD_COMANND) {
                PWRX_CANPacket pwr_packet;
                ros2can::msg::PWRManagerRX msg;
                memcpy(&pwr_packet, packet.data, sizeof(pwr_packet));
                msg.board_num = id.fields.board_num;
                msg.current = pwr_packet.current;
                msg.battery1_voltage = pwr_packet.battery1_voltage;
                msg.battery2_voltage = pwr_packet.battery2_voltage;
                msg.output_voltage = pwr_packet.output_voltage;
                pwr_rx_publisher_->publish(msg);
            } else {
                ros2can::msg::UdpCanFrame msg;
                msg.id = packet.id;
                msg.size = packet.size;
                memcpy(msg.data.data(), packet.data, 32);
            can_rx_publisher_->publish(msg);
            }

        } else if (n > 0) {
            RCLCPP_WARN(this->get_logger(), "Received packet of unexpected size: %zd bytes (expected %zu)", n, sizeof(packet));
        }
    }
}

int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ROS2CAN_Node>());
    rclcpp::shutdown();
    return 0;
}
