#include <rclcpp/rclcpp.hpp>
#include <ros2can/msg/udp_can_frame.hpp>
#include <ros2can/msg/bldc_driver.hpp>


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
    uint8_t  size;          
    uint8_t  data[64];     
};
#pragma pack(pop)

class ROS2CAN_Node : public rclcpp::Node {
public:
    ROS2CAN_Node();
    ~ROS2CAN_Node();
private:
    int sockfd_;
    bool is_running_;
    std::string remote_ip_;
    int remote_port_;
    std::thread rx_thread_;
    rclcpp::Subscription<ros2can::msg::UdpCanFrame>::SharedPtr can_tx_subscription_;
    rclcpp::Publisher<ros2can::msg::UdpCanFrame>::SharedPtr can_rx_publisher_;

    rclcpp::Subscription<ros2can::msg::BLDCDriver>::SharedPtr bldc_tx_subscription_;
    rclcpp::Publisher<ros2can::msg::BLDCDriver>::SharedPtr bldc_rx_publisher_;

    void tx_packet();
    void tx_callback(const ros2can::msg::UdpCanFrame::SharedPtr msg);
    void BLDC_callback(const ros2can::msg::BLDCDriver::SharedPtr msg);
    void rx_thread_func();
};