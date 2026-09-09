// wt901c485_driver.cpp
// Low-level Modbus RTU driver for WT901C-485 over POSIX serial.

#include "wt901c485_imu/wt901c485_driver.hpp"

#include <cstring>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

namespace {
// Scale factors from WT901C-485 datasheet
constexpr double kAccScale  = 16.0 * 9.80665 / 32768.0;  // m/s²
constexpr double kGyroScale = 2000.0 * M_PI / (32768.0 * 180.0); // rad/s
constexpr double kAngScale  = 180.0 * M_PI / (32768.0 * 180.0);  // rad
constexpr double kQuatScale = 1.0 / 32768.0;
constexpr double kMagScale  = 1.0e-6;  // μT → T (raw is in μT * 100 approx)

// Register start addresses
constexpr uint16_t kRegAcc   = 0x0034;  // AX AY AZ  (3 regs)
constexpr uint16_t kRegGyro  = 0x0037;  // GX GY GZ  (3 regs)
constexpr uint16_t kRegAngle = 0x003D;  // Roll Pitch Yaw (3 regs)
constexpr uint16_t kRegQuat  = 0x0051;  // Q0 Q1 Q2 Q3 (4 regs)
constexpr uint16_t kRegMag   = 0x003A;  // HX HY HZ  (3 regs)
}  // namespace

namespace wt901c485 {

WT901C485Driver::WT901C485Driver(const std::string& port,
                                 int baud,
                                 uint8_t address)
    : port_(port), baud_(baud), address_(address) {}

WT901C485Driver::~WT901C485Driver() { close(); }

// ─────────────────────────────────────────────────────────────────────────────
bool WT901C485Driver::open() {
  fd_ = ::open(port_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
  if (fd_ < 0) return false;

  struct termios tty{};
  if (tcgetattr(fd_, &tty) != 0) { ::close(fd_); fd_ = -1; return false; }

  // Baud rate mapping
  speed_t speed = B9600;
  if (baud_ == 4800)   speed = B4800;
  else if (baud_ == 19200)  speed = B19200;
  else if (baud_ == 38400)  speed = B38400;
  else if (baud_ == 57600)  speed = B57600;
  else if (baud_ == 115200) speed = B115200;

  cfsetispeed(&tty, speed);
  cfsetospeed(&tty, speed);

  // 8N1
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag |= CREAD | CLOCAL;
  tty.c_lflag  = 0;
  tty.c_oflag  = 0;
  tty.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL);

  // Read timeout: 100 ms
  tty.c_cc[VMIN]  = 0;
  tty.c_cc[VTIME] = 1;   // ×100 ms

  if (tcsetattr(fd_, TCSANOW, &tty) != 0) {
    ::close(fd_); fd_ = -1; return false;
  }
  tcflush(fd_, TCIOFLUSH);
  return true;
}

void WT901C485Driver::close() {
  if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

// ─────────────────────────────────────────────────────────────────────────────
bool WT901C485Driver::read(ImuRaw& out) {
  std::vector<int16_t> regs;

  // 1) Acceleration (3 registers)
  auto req = buildReadRequest(kRegAcc, 3);
  if (!transact(req, 3, regs)) return false;
  out.acc_x = regs[0] * kAccScale;
  out.acc_y = regs[1] * kAccScale;
  out.acc_z = regs[2] * kAccScale;

  // 2) Angular velocity (3 registers)
  req = buildReadRequest(kRegGyro, 3);
  if (!transact(req, 3, regs)) return false;
  out.gyro_x = regs[0] * kGyroScale;
  out.gyro_y = regs[1] * kGyroScale;
  out.gyro_z = regs[2] * kGyroScale;

  // 3) Euler angles (3 registers) – kept for reference, not published directly
  req = buildReadRequest(kRegAngle, 3);
  if (!transact(req, 3, regs)) return false;
  out.roll  = regs[0] * kAngScale;
  out.pitch = regs[1] * kAngScale;
  out.yaw   = regs[2] * kAngScale;

  // 4) Quaternion (4 registers)
  req = buildReadRequest(kRegQuat, 4);
  if (!transact(req, 4, regs)) return false;
  out.qw = regs[0] * kQuatScale;
  out.qx = regs[1] * kQuatScale;
  out.qy = regs[2] * kQuatScale;
  out.qz = regs[3] * kQuatScale;

  // 5) Magnetic field (3 registers)
  req = buildReadRequest(kRegMag, 3);
  if (!transact(req, 3, regs)) return false;
  out.mag_x = regs[0] * kMagScale;
  out.mag_y = regs[1] * kMagScale;
  out.mag_z = regs[2] * kMagScale;

  return true;
}

// ─────────────────────────────────────────────────────────────────────────────
std::array<uint8_t, 8>
WT901C485Driver::buildReadRequest(uint16_t reg_start,
                                  uint16_t reg_count) const {
  std::array<uint8_t, 8> req{};
  req[0] = address_;
  req[1] = 0x03;                         // Function: Read Holding Registers
  req[2] = static_cast<uint8_t>(reg_start >> 8);
  req[3] = static_cast<uint8_t>(reg_start & 0xFF);
  req[4] = static_cast<uint8_t>(reg_count >> 8);
  req[5] = static_cast<uint8_t>(reg_count & 0xFF);
  uint16_t crc = crc16(req.data(), 6);
  req[6] = static_cast<uint8_t>(crc & 0xFF);   // CRC low byte first
  req[7] = static_cast<uint8_t>(crc >> 8);
  return req;
}

bool WT901C485Driver::transact(const std::array<uint8_t, 8>& request,
                                uint16_t reg_count,
                                std::vector<int16_t>& out_regs) {
  tcflush(fd_, TCIFLUSH);

  // Write request
  ssize_t written = ::write(fd_, request.data(), 8);
  if (written != 8) return false;

  // Expected response length: addr(1) + func(1) + byte_count(1) + data(reg_count*2) + crc(2)
  size_t expected = 3 + reg_count * 2 + 2;
  std::vector<uint8_t> buf(expected);

  size_t received = 0;
  int retries = 10;
  while (received < expected && retries-- > 0) {
    ssize_t n = ::read(fd_, buf.data() + received, expected - received);
    if (n > 0) received += static_cast<size_t>(n);
  }

  if (received < expected) return false;

  // Validate CRC
  uint16_t crc_calc = crc16(buf.data(), received - 2);
  uint16_t crc_recv = static_cast<uint16_t>(buf[received - 2]) |
                      (static_cast<uint16_t>(buf[received - 1]) << 8);
  if (crc_calc != crc_recv) return false;

  // Validate address + function
  if (buf[0] != address_ || buf[1] != 0x03) return false;

  // Parse register values (big-endian signed 16-bit)
  out_regs.resize(reg_count);
  for (uint16_t i = 0; i < reg_count; ++i) {
    uint16_t raw = (static_cast<uint16_t>(buf[3 + i * 2]) << 8) |
                    static_cast<uint16_t>(buf[4 + i * 2]);
    out_regs[i] = static_cast<int16_t>(raw);
  }
  return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Modbus CRC-16 (polynomial 0xA001)
uint16_t WT901C485Driver::crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= static_cast<uint16_t>(data[i]);
    for (int j = 0; j < 8; ++j) {
      if (crc & 0x0001) { crc >>= 1; crc ^= 0xA001; }
      else              { crc >>= 1; }
    }
  }
  return crc;
}

}  // namespace wt901c485
