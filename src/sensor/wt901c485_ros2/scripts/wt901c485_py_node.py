#!/usr/bin/env python3
"""
wt901c485_py_node.py
ROS2 Humble Python node for WT901C-485 IMU.

Publishes:
  ~/imu/data       sensor_msgs/Imu
  ~/imu/mag        sensor_msgs/MagneticField

Parameters:
  port             string   /dev/ttyUSB0
  baud             int      9600
  modbus_address   int      80  (0x50)
  frame_id         string   imu_link
  rate_hz          float    50.0
  publish_tf       bool     true
"""

import math
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu, MagneticField
from geometry_msgs.msg import TransformStamped
from tf2_ros import TransformBroadcaster

from wt901c485_imu.driver import WT901C485Driver


class WT901C485PyNode(Node):
    def __init__(self):
        super().__init__('wt901c485_py_node')

        # ── Parameters ──────────────────────────────────────────────────────
        self.declare_parameter('port',           '/dev/ttyUSB0')
        self.declare_parameter('baud',           9600)
        self.declare_parameter('modbus_address', 0x50)
        self.declare_parameter('frame_id',       'imu_link')
        self.declare_parameter('rate_hz',        50.0)
        self.declare_parameter('publish_tf',     True)

        port       = self.get_parameter('port').value
        baud       = self.get_parameter('baud').value
        addr       = self.get_parameter('modbus_address').value
        self._fid  = self.get_parameter('frame_id').value
        rate_hz    = self.get_parameter('rate_hz').value
        self._pub_tf = self.get_parameter('publish_tf').value

        # ── Driver ──────────────────────────────────────────────────────────
        self._drv = WT901C485Driver(port, baud, addr)
        try:
            self._drv.open()
        except Exception as e:
            self.get_logger().fatal(f'Cannot open {port}: {e}')
            raise

        self.get_logger().info(
            f'Opened {port} at {baud} baud (Modbus addr 0x{addr:02X})')

        # ── Publishers ───────────────────────────────────────────────────────
        self._imu_pub = self.create_publisher(Imu,          'imu/data', 10)
        self._mag_pub = self.create_publisher(MagneticField, 'imu/mag',  10)

        if self._pub_tf:
            self._tf_bc = TransformBroadcaster(self)

        # ── Timer ────────────────────────────────────────────────────────────
        self.create_timer(1.0 / rate_hz, self._cb)
        self.get_logger().info(f'WT901C-485 Python node started at {rate_hz} Hz')

    def _cb(self):
        try:
            d = self._drv.read()
        except Exception as e:
            self.get_logger().warn(f'IMU read error: {e}', throttle_duration_sec=2.0)
            return

        stamp = self.get_clock().now().to_msg()

        # ── sensor_msgs/Imu ──────────────────────────────────────────────────
        imu = Imu()
        imu.header.stamp    = stamp
        imu.header.frame_id = self._fid

        imu.orientation.w, imu.orientation.x, \
            imu.orientation.y, imu.orientation.z = d.quat

        # 0.05° static accuracy → rad
        ov = (0.05 * math.pi / 180.0) ** 2
        imu.orientation_covariance        = [ov, 0, 0, 0, ov, 0, 0, 0, ov]
        gv = (0.05 * math.pi / 180.0) ** 2
        imu.angular_velocity_covariance   = [gv, 0, 0, 0, gv, 0, 0, 0, gv]
        av = (0.01 * 9.80665) ** 2
        imu.linear_acceleration_covariance = [av, 0, 0, 0, av, 0, 0, 0, av]

        imu.angular_velocity.x,  \
            imu.angular_velocity.y,  \
            imu.angular_velocity.z  = d.gyro
        imu.linear_acceleration.x, \
            imu.linear_acceleration.y, \
            imu.linear_acceleration.z = d.acc

        self._imu_pub.publish(imu)

        # ── sensor_msgs/MagneticField ────────────────────────────────────────
        mag = MagneticField()
        mag.header = imu.header
        mag.magnetic_field.x, \
            mag.magnetic_field.y, \
            mag.magnetic_field.z = d.mag
        self._mag_pub.publish(mag)

        # ── TF ───────────────────────────────────────────────────────────────
        if self._pub_tf:
            tf = TransformStamped()
            tf.header.stamp    = stamp
            tf.header.frame_id = 'base_link'
            tf.child_frame_id  = self._fid
            tf.transform.rotation = imu.orientation
            self._tf_bc.sendTransform(tf)

    def destroy_node(self):
        self._drv.close()
        super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = WT901C485PyNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
