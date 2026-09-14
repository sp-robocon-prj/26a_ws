#include <rclcpp/rclcpp.hpp>

#include <ros2can/msg/udp_can_frame.hpp>
#include <ros2can/msg/udp_can_frame.hpp>

#include <ros2can/msg/bldctx.hpp>
#include <ros2can/msg/bldcrx.hpp>

#include <ros2can/msg/pwr_manager_tx.hpp>
#include <ros2can/msg/pwr_manager_rx.hpp>

#include <ros2can/msg/servo_tx.hpp>
#include <ros2can/msg/motor_board_tx.hpp>

#include <BLDC_format.h>
#include <ID_format.h>
#include <PWRManager_format.h>
#include <UDPPacket_format.h>
#include <Servo_format.h>
#include <MotorBoard_format.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <iostream>

class ROS2CAN_Node : public rclcpp::Node {
public:
    ROS2CAN_Node();
    ~ROS2CAN_Node();
private:
    int sockfd_;
    bool is_running_;
    std::string remote_ip_;
    int remote_port_;
    struct sockaddr_in remote_addr;
    std::thread rx_thread_;

    rclcpp::Subscription<ros2can::msg::UdpCanFrame>::SharedPtr can_tx_subscription_;
    rclcpp::Publisher<ros2can::msg::UdpCanFrame>::SharedPtr can_rx_publisher_;

    rclcpp::Subscription<ros2can::msg::BLDCTX>::SharedPtr bldc_tx_subscription_;
    rclcpp::Publisher<ros2can::msg::BLDCRX>::SharedPtr bldc_rx_publisher_;

    rclcpp::Subscription<ros2can::msg::PWRManagerTX>::SharedPtr pwr_tx_subscription_;
    rclcpp::Publisher<ros2can::msg::PWRManagerRX>::SharedPtr pwr_rx_publisher_;

    rclcpp::Subscription<ros2can::msg::MotorBoardTX>::SharedPtr motorboard_tx_subscription_;
    rclcpp::Subscription<ros2can::msg::ServoTX>::SharedPtr servo_tx_subscription_;


    void tx_packet();
    void tx_callback(const ros2can::msg::UdpCanFrame::SharedPtr msg);
    void BLDC_callback(const ros2can::msg::BLDCTX::SharedPtr msg);
    void PWR_callback(const ros2can::msg::PWRManagerTX::SharedPtr msg);
    void MotorBoard_callback(const ros2can::msg::MotorBoardTX::SharedPtr msg);
    void Servo_callback(const ros2can::msg::ServoTX::SharedPtr msg);
    void rx_thread_func();
};