#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <cmath>
#include <vector>

class UnitreeL2SimNode : public rclcpp::Node {
public:
    UnitreeL2SimNode() : Node("unitree_l2_sim_node") {
        // Parameters
        this->declare_parameter<double>("update_rate", 10.0);
        this->declare_parameter<int>("points_per_sec", 64000);
        this->declare_parameter<std::string>("input_topic", "/gazebo/lidar_dense");
        this->declare_parameter<std::string>("output_topic", "/scan");

        double update_rate = this->get_parameter("update_rate").as_double();
        points_per_sec_ = this->get_parameter("points_per_sec").as_int();
        points_per_frame_ = points_per_sec_ / update_rate;

        sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
            this->get_parameter("input_topic").as_string(), 10,
            std::bind(&UnitreeL2SimNode::cloud_callback, this, std::placeholders::_1));

        pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
            this->get_parameter("output_topic").as_string(), 10);
            
        RCLCPP_INFO(this->get_logger(), "Unitree L2 Sim Node started.");
    }

private:
    void cloud_callback(const sensor_msgs::msg::PointCloud2::SharedPtr msg) {
        // Assuming input is 100 Hz, we accumulate 10 messages for a 10 Hz output
        if (accumulated_points_.empty()) {
            frame_start_time_ = msg->header.stamp;
            frame_header_ = msg->header;
        }

        // Validate organized cloud
        if (msg->height <= 1 || msg->width <= 1) {
            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "Input cloud is not organized!");
            return;
        }

        // L2 specifications (from URDF)
        double min_azimuth = -M_PI;
        double max_azimuth = M_PI;
        double min_elevation = -M_PI / 2.0; // -90 deg
        double max_elevation = 0.0;         // 0 deg

        double az_res = (max_azimuth - min_azimuth) / (msg->width - 1);
        double el_res = (max_elevation - min_elevation) / (msg->height - 1);

        // Calculate how many points to extract for this 10ms interval
        // 100 Hz means 0.01s per message. We need points_per_sec_ * 0.01 points
        int points_this_msg = points_per_sec_ / 100;
        double dt = 0.01 / points_this_msg;

        sensor_msgs::PointCloud2ConstIterator<float> iter_x(*msg, "x");
        sensor_msgs::PointCloud2ConstIterator<float> iter_y(*msg, "y");
        sensor_msgs::PointCloud2ConstIterator<float> iter_z(*msg, "z");

        rclcpp::Time msg_time(msg->header.stamp);
        double t_base = msg_time.seconds();

        for (int i = 0; i < points_this_msg; ++i) {
            double t = t_base + i * dt;
            
            // Unitree L2 Non-repetitive pattern (Lissajous approximation)
            // Spin horizontally at 11 Hz, wobble vertically at 47 Hz
            double azimuth = min_azimuth + std::fmod(2.0 * M_PI * 11.0 * t, 2.0 * M_PI);
            double elevation = min_elevation + (max_elevation - min_elevation) * 0.5 * (1.0 + std::sin(2.0 * M_PI * 47.0 * t));

            if (azimuth < min_azimuth) azimuth += 2.0 * M_PI;
            if (azimuth > max_azimuth) azimuth -= 2.0 * M_PI;

            // Map to grid
            int col = std::round((azimuth - min_azimuth) / az_res);
            int row = std::round((elevation - min_elevation) / el_res);

            col = std::clamp(col, 0, (int)msg->width - 1);
            row = std::clamp(row, 0, (int)msg->height - 1);

            int index = row * msg->width + col;

            // Extract point
            float x = *(iter_x + index);
            float y = *(iter_y + index);
            float z = *(iter_z + index);

            // Ignore points too close or far (handled by Gazebo but good to check)
            if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) continue;

            PointWithTime p;
            p.x = x; p.y = y; p.z = z;
            p.time_offset = static_cast<float>(t - rclcpp::Time(frame_start_time_).seconds());
            accumulated_points_.push_back(p);
        }

        msgs_accumulated_++;

        // Publish if we have accumulated enough (100 Hz input, 10 Hz output = 10 msgs)
        if (msgs_accumulated_ >= 10) {
            publish_cloud();
            accumulated_points_.clear();
            msgs_accumulated_ = 0;
        }
    }

    struct PointWithTime {
        float x, y, z;
        float time_offset;
    };

    void publish_cloud() {
        sensor_msgs::msg::PointCloud2 out_msg;
        out_msg.header = frame_header_;
        out_msg.height = 1;
        out_msg.width = accumulated_points_.size();
        out_msg.is_dense = true;
        out_msg.is_bigendian = false;

        sensor_msgs::PointCloud2Modifier modifier(out_msg);
        modifier.setPointCloud2Fields(4,
            "x", 1, sensor_msgs::msg::PointField::FLOAT32,
            "y", 1, sensor_msgs::msg::PointField::FLOAT32,
            "z", 1, sensor_msgs::msg::PointField::FLOAT32,
            "time", 1, sensor_msgs::msg::PointField::FLOAT32);

        sensor_msgs::PointCloud2Iterator<float> out_x(out_msg, "x");
        sensor_msgs::PointCloud2Iterator<float> out_y(out_msg, "y");
        sensor_msgs::PointCloud2Iterator<float> out_z(out_msg, "z");
        sensor_msgs::PointCloud2Iterator<float> out_t(out_msg, "time");

        for (const auto& p : accumulated_points_) {
            *out_x = p.x;
            *out_y = p.y;
            *out_z = p.z;
            *out_t = p.time_offset;
            ++out_x; ++out_y; ++out_z; ++out_t;
        }

        pub_->publish(out_msg);
    }

    rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr sub_;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_;
    
    int points_per_sec_;
    int points_per_frame_;
    int msgs_accumulated_ = 0;
    builtin_interfaces::msg::Time frame_start_time_;
    std::vector<PointWithTime> accumulated_points_;
    std_msgs::msg::Header frame_header_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<UnitreeL2SimNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
