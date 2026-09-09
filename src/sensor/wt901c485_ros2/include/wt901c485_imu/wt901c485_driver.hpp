#pragma once
// WT901C-485 low-level Modbus RTU driver (C++)
// Communicates over /dev/ttyUSB* via RS485 adapter.
//
// Register map (16-bit signed, read function 0x03):
//   0x34  AX AY AZ   – raw accel   ÷ 32768 × 16 g
//   0x37  GX GY GZ   – raw gyro    ÷ 32768 × 2000 °/s
//   0x3D  RollH PitchH YawH  – angle ÷ 32768 × 180 °
//   0x51  Q0 Q1 Q2 Q3        – quaternion ÷ 32768

#include <string>
#include <array>
#include <vector>
#include <cstdint>
#include <termios.h>  // POSIX serial

namespace wt901c485 {

// Raw sensor data block
struct ImuRaw {
  // Acceleration [m/s²]
  double acc_x{0}, acc_y{0}, acc_z{0};
  // Angular velocity [rad/s]
  double gyro_x{0}, gyro_y{0}, gyro_z{0};
  // Euler angles [rad] (roll, pitch, yaw)
  double roll{0}, pitch{0}, yaw{0};
  // Quaternion (w, x, y, z)
  double qw{1}, qx{0}, qy{0}, qz{0};
  // Magnetic field [Tesla]
  double mag_x{0}, mag_y{0}, mag_z{0};
};

class WT901C485Driver {
public:
  // port    : e.g. "/dev/ttyUSB0"
  // baud    : 9600 (default)
  // address : Modbus slave address, default 0x50
  explicit WT901C485Driver(const std::string& port,
                           int baud    = 9600,
                           uint8_t address = 0x50);
  ~WT901C485Driver();

  bool open();
  void close();
  bool isOpen() const { return fd_ >= 0; }

  // Read all sensor data in one shot.
  // Returns false if communication fails.
  bool read(ImuRaw& out);

private:
  // Build a Modbus RTU read request (function 0x03)
  std::array<uint8_t, 8> buildReadRequest(uint16_t reg_start,
                                          uint16_t reg_count) const;

  // Send request, receive response; returns parsed 16-bit register values.
  // reg_count  : number of registers requested
  // out_regs   : output vector (size == reg_count)
  bool transact(const std::array<uint8_t, 8>& request,
                uint16_t reg_count,
                std::vector<int16_t>& out_regs);

  // Modbus CRC-16
  static uint16_t crc16(const uint8_t* data, size_t len);

  std::string port_;
  int         baud_;
  uint8_t     address_;
  int         fd_{-1};
};

}  // namespace wt901c485
