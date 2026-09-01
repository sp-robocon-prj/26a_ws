#include <rclcpp/rclcpp.hpp>
#include <ros2can/msg/udp_can_frame.hpp>

#include <ros2can/msg/bldctx.hpp>
#include <ros2can/msg/bldcrx.hpp>

#include <ros2can/msg/pwr_manager_tx.hpp>
#include <ros2can/msg/pwr_manager_rx.hpp>

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

#pragma pack(push, 1)
struct BLDCPacket {
    uint8_t mode;
    int16_t rps_target;          
    float  angle_target;
    bool monitor_peirod;
    uint16_t monitor_freq;
    uint8_t motor_type;
    float gear_ratio;
    uint32_t encoder_pulse;
    bool en_limit_sw;
};
#pragma pack(pop)


#pragma pack(push, 1)
struct PWRTXPacket {
    bool pwrstatus;
    uint8_t ledstatus;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PWRXPacket {
    float current;
    float battery1_voltage;
    float battery2_voltage;
    float output_voltage;
};
#pragma pack(pop)



enum class DataType : uint8_t {
    COMMON_COMAND = 0x01,
    POWERBOARD_COMANND = 0x02,
    BLCD_COMANND = 0x03,
    MOTORBOARDC_COMAND = 0x04,
    // Add other data types as needed
};

union ID {
    uint16_t id;
    struct {
        uint16_t board_num : 4;
        DataType data_type : 4;
        uint16_t priority : 3;
    } fields;
};

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

    void tx_packet();
    void tx_callback(const ros2can::msg::UdpCanFrame::SharedPtr msg);
    void BLDC_callback(const ros2can::msg::BLDCTX::SharedPtr msg);
    void PWR_callback(const ros2can::msg::PWRManagerTX::SharedPtr msg);
    void rx_thread_func();
};