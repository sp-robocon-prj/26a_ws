// wt901c485_node.cpp
// ROS2 Humble C++ node for WT901C-485 IMU.
//
// Publishes:
//   ~/imu/data       sensor_msgs/Imu          (quaternion + angular vel + accel)
//   ~/imu/mag        sensor_msgs/MagneticField
// Parameters:
//   port             string  "/dev/ttyUSB0"
//   baud             int     9600
//   modbus_address   int     0x50 (80)
//   frame_id         string  "imu_link"
//   rate_hz          double  50.0
//   publish_tf       bool    true

#include <memory>
#include <string>
#include <chrono>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/magnetic_field.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "tf2_ros/transform_broadcaster.h"
#include "tf2/LinearMath/Quaternion.h"

#include "wt901c485_imu/wt901c485_driver.hpp"

using namespace std::chrono_literals;

class WT901C485Node : public rclcpp::Node {
public:
  WT901C485Node() : Node("wt901c485_node") {

    // ── Declare + get parameters ───────────────────────────────────────────
    declare_parameter<std::string>("port",           "/dev/ttyUSB0");
    declare_parameter<int>        ("baud",           9600);
    declare_parameter<int>        ("modbus_address", 0x50);
    declare_parameter<std::string>("frame_id",       "imu_link");
    declare_parameter<double>     ("rate_hz",        50.0);
    declare_parameter<bool>       ("publish_tf",     true);

    const auto port    = get_parameter("port").as_string();
    const auto baud    = get_parameter("baud").as_int();
    const auto addr    = static_cast<uint8_t>(get_parameter("modbus_address").as_int());
    frame_id_          = get_parameter("frame_id").as_string();
    const auto rate_hz = get_parameter("rate_hz").as_double();
    publish_tf_        = get_parameter("publish_tf").as_bool();

    // ── Driver ────────────────────────────────────────────────────────────
    driver_ = std::make_unique<wt901c485::WT901C485Driver>(port, baud, addr);
    if (!driver_->open()) {
      RCLCPP_FATAL(get_logger(),
        "Cannot open serial port '%s'. Check cable and permissions.", port.c_str());
      throw std::runtime_error("Failed to open serial port");
    }
    RCLCPP_INFO(get_logger(), "Opened %s at %ld baud (Modbus addr 0x%02X)",
                port.c_str(), baud, addr);

    // ── Publishers ────────────────────────────────────────────────────────
    imu_pub_ = create_publisher<sensor_msgs::msg::Imu>("imu/data", 10);
    mag_pub_ = create_publisher<sensor_msgs::msg::MagneticField>("imu/mag", 10);

    if (publish_tf_) {
      tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
    }

    // ── Timer ─────────────────────────────────────────────────────────────
    auto period = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::duration<double>(1.0 / rate_hz));
    timer_ = create_wall_timer(period, std::bind(&WT901C485Node::timerCb, this));

    RCLCPP_INFO(get_logger(), "WT901C485 driver started at %.1f Hz.", rate_hz);
  }

private:
  void timerCb() {
    wt901c485::ImuRaw raw;
    if (!driver_->read(raw)) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000,
                           "IMU read failed – retrying…");
      return;
    }

    auto stamp = now();

    // ── sensor_msgs/Imu ───────────────────────────────────────────────────
    auto imu_msg = sensor_msgs::msg::Imu();
    imu_msg.header.stamp    = stamp;
    imu_msg.header.frame_id = frame_id_;

    imu_msg.orientation.w = raw.qw;
    imu_msg.orientation.x = raw.qx;
    imu_msg.orientation.y = raw.qy;
    imu_msg.orientation.z = raw.qz;
    // Covariance: device spec 0.05° static ≈ 8.7e-4 rad
    constexpr double kOrientVar = 8.7e-4 * 8.7e-4;
    imu_msg.orientation_covariance[0] = kOrientVar;
    imu_msg.orientation_covariance[4] = kOrientVar;
    imu_msg.orientation_covariance[8] = kOrientVar;

    imu_msg.angular_velocity.x = raw.gyro_x;
    imu_msg.angular_velocity.y = raw.gyro_y;
    imu_msg.angular_velocity.z = raw.gyro_z;
    constexpr double kGyroVar = 8.7e-4;
    imu_msg.angular_velocity_covariance[0] = kGyroVar;
    imu_msg.angular_velocity_covariance[4] = kGyroVar;
    imu_msg.angular_velocity_covariance[8] = kGyroVar;

    imu_msg.linear_acceleration.x = raw.acc_x;
    imu_msg.linear_acceleration.y = raw.acc_y;
    imu_msg.linear_acceleration.z = raw.acc_z;
    constexpr double kAccVar = 0.01 * 0.01 * 9.80665 * 9.80665;
    imu_msg.linear_acceleration_covariance[0] = kAccVar;
    imu_msg.linear_acceleration_covariance[4] = kAccVar;
    imu_msg.linear_acceleration_covariance[8] = kAccVar;

    imu_pub_->publish(imu_msg);

    // ── sensor_msgs/MagneticField ─────────────────────────────────────────
    auto mag_msg = sensor_msgs::msg::MagneticField();
    mag_msg.header = imu_msg.header;
    mag_msg.magnetic_field.x = raw.mag_x;
    mag_msg.magnetic_field.y = raw.mag_y;
    mag_msg.magnetic_field.z = raw.mag_z;
    mag_pub_->publish(mag_msg);

    // ── TF: base_link → imu_link ──────────────────────────────────────────
    if (publish_tf_ && tf_broadcaster_) {
      geometry_msgs::msg::TransformStamped tf;
      tf.header.stamp    = stamp;
      tf.header.frame_id = "base_link";
      tf.child_frame_id  = frame_id_;
      // Sensor mounted at origin with identity offset
      tf.transform.translation.x = 0.0;
      tf.transform.translation.y = 0.0;
      tf.transform.translation.z = 0.0;
      tf.transform.rotation      = imu_msg.orientation;
      tf_broadcaster_->sendTransform(tf);
    }
  }

  std::unique_ptr<wt901c485::WT901C485Driver> driver_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr            imu_pub_;
  rclcpp::Publisher<sensor_msgs::msg::MagneticField>::SharedPtr  mag_pub_;
  std::unique_ptr<tf2_ros::TransformBroadcaster>                 tf_broadcaster_;
  rclcpp::TimerBase::SharedPtr                                   timer_;

  std::string frame_id_;
  bool        publish_tf_{true};
};

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<WT901C485Node>());
  } catch (const std::exception& e) {
    RCLCPP_FATAL(rclcpp::get_logger("main"), "%s", e.what());
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
