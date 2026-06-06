#include <rclcpp/rclcpp.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <cerrno>

#include <vector>

class UM7Node : public rclcpp::Node
{
public:
    UM7Node()
        : Node("um7_node")
    {
        fd_ = open(
            "/dev/ttyUSB0",
            O_RDWR | O_NOCTTY);

        if (fd_ < 0)
        {
            RCLCPP_ERROR(
                this->get_logger(),
                "Cannot open serial : %s",
                strerror(errno));
        }

        struct termios tty;

        tcgetattr(fd_, &tty);

        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);

        tty.c_cflag |= CLOCAL;
        tty.c_cflag |= CREAD;

        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;

        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;

        tcsetattr(
            fd_,
            TCSANOW,
            &tty);

        timer_ =
            create_wall_timer(
                std::chrono::milliseconds(1),
                std::bind(
                    &UM7Node::readSerial,
                    this));
    }

private:
    void readSerial()
    {
        uint8_t c;

        int n = read(
            fd_,
            &c,
            1);

        if (n <= 0)
            return;

        buffer_.push_back(c);

        if (buffer_.size() > 3)
            buffer_.erase(buffer_.begin());

        if (buffer_.size() == 3)
        {
            if (
                buffer_[0] == 's' &&
                buffer_[1] == 'n' &&
                buffer_[2] == 'p')
            {
                RCLCPP_INFO(
                    this->get_logger(),
                    "SNP Packet Found");
            }
        }
    }

    int fd_;

    std::vector<uint8_t> buffer_;

    rclcpp::TimerBase::SharedPtr timer_;
};

int main(
    int argc,
    char *argv[])
{
    rclcpp::init(argc, argv);

    rclcpp::spin(
        std::make_shared<UM7Node>());

    rclcpp::shutdown();

    return 0;
}
