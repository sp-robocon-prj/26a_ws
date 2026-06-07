import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu
from geometry_msgs.msg import TransformStamped
import tf2_ros
import serial
import struct
import math
import time

class UM7DriverNode(Node):
    def __init__(self):
        super().__init__('um7_driver_node')
        
        # パラメータの宣言と取得
        self.declare_parameter('port', '/dev/serial/by-id/usb-Silicon_Labs_CP2104_USB_to_UART_Bridge_Controller_02J4KT83-if00-port0')
        self.declare_parameter('baudrate', 115200)
        self.declare_parameter('frame_id', 'imu_link')
        
        self.port = self.get_parameter('port').get_parameter_value().string_value
        baudrate = self.get_parameter('baudrate').get_parameter_value().integer_value
        self.frame_id = self.get_parameter('frame_id').get_parameter_value().string_value
        
        # 初期化用変数
        self.is_initialized = False
        self.initial_roll = 0.0
        self.initial_pitch = 0.0
        self.initial_yaw = 0.0
        self.init_start_time = self.get_clock().now()
        self.last_data_time = time.time()
        self.timeout_warned = False

        # パブリッシャーとTFブロードキャスター
        self.imu_pub = self.create_publisher(Imu, 'imu/data', 10)
        self.tf_broadcaster = tf2_ros.TransformBroadcaster(self)

        # シリアルポートオープン
        try:
            self.ser = serial.Serial(self.port, baudrate, timeout=0.01)
            self.get_logger().info(f"Connected to UM7 on {self.port} ({baudrate} bps)")
        except serial.SerialException as e:
            self.get_logger().error(f"Could not open serial port: {e}")
            raise e

        # 【新規】UM7ハードウェアへ「ジャイロのゼロ点校正コマンド (ZERO_GYROS)」を送信
        # コマンドレジスタ: 0xAD / チェックサム: 0x01FE
        try:
            zero_gyros_cmd = b'\x73\x6E\x70\x00\xAD\x01\xFE'
            self.ser.write(zero_gyros_cmd)
            self.get_logger().info("⚠️ UM7へジャイロ自動校正コマンドを送信しました。約1.5秒間、センサーを動かさず完全に静止させてください！")
        except Exception as e:
            self.get_logger().error(f"Failed to send ZERO_GYROS command: {e}")

        # 高頻度（200Hz）でシリアルバッファをチェックするタイマー
        self.timer = self.create_timer(0.005, self.read_serial)
        self.buffer = bytearray()

    def read_serial(self):
        if self.ser.in_waiting > 0:
            self.buffer.extend(self.ser.read(self.ser.in_waiting))
            self.parse_buffer()
        else:
            # タイムアウトチェック (3秒間データが来ない場合警告)
            if not self.timeout_warned and not self.is_initialized and (time.time() - self.last_data_time > 3.0):
                self.get_logger().error(f"⚠️ 3秒間 UM7 からのデータを受信していません！ポート {self.port} が間違っていませんか？ (例: /dev/ttyUSB1 を試してください)")
                self.timeout_warned = True

    def parse_buffer(self):
        while len(self.buffer) >= 3:
            idx = self.buffer.find(b'snp')
            if idx == -1:
                if b'sn' in self.buffer[-2:]: self.buffer = self.buffer[-2:]
                elif b's' in self.buffer[-1:]: self.buffer = self.buffer[-1:]
                else: self.buffer.clear()
                return
            
            if idx > 0:
                del self.buffer[:idx]
                
            if len(self.buffer) < 5:
                return
                
            pt = self.buffer[3]
            address = self.buffer[4]
            
            has_data = (pt & 0x80) != 0
            is_batch = (pt & 0x40) != 0
            batch_len = (pt >> 2) & 0x0F
            
            data_len = (batch_len * 4) if is_batch else (4 if has_data else 0)
            packet_len = 5 + data_len + 2
            
            if len(self.buffer) < packet_len:
                return
                
            packet_data = self.buffer[:packet_len]
            
            # チェックサムの検証
            calc_checksum = sum(packet_data[:-2]) & 0xFFFF
            recv_checksum = struct.unpack('>H', packet_data[-2:])[0]
            
            if calc_checksum == recv_checksum:
                self.last_data_time = time.time()
                self.process_packet(address, packet_data[5:-2])
                
            del self.buffer[:packet_len]

    def process_packet(self, start_address, data):
        # クォータニオン (DREG_QUAT_AB = 0x7D)
        if start_address == 0x7D and len(data) >= 8:
            a_raw, b_raw, c_raw, d_raw = struct.unpack('>hhhh', data[:8])
            scale = 29789.09
            self.publish_imu(a_raw/scale, b_raw/scale, c_raw/scale, d_raw/scale)
            
        # オイラー角 (DREG_EULER_PHI_THETA = 0x70)
        elif start_address == 0x70 and len(data) >= 8:
            phi, theta, psi, _ = struct.unpack('>hhhh', data[:8])
            scale = 0.0109863
            roll = math.radians(phi * scale)
            pitch = math.radians(theta * scale)
            yaw = math.radians(psi * scale)
            
            # 【新規】起動後1.5秒間はハードウェア校正を待つため無視
            time_elapsed = (self.get_clock().now() - self.init_start_time).nanoseconds / 1e9
            if time_elapsed < 1.5:
                return

            # 【新規】1.5秒経過直後の最初のデータを「基準（0度）」として記憶
            if not self.is_initialized:
                self.initial_roll = roll
                self.initial_pitch = pitch
                self.initial_yaw = yaw
                self.is_initialized = True
                self.get_logger().info("✨ 起動時初期化が完了しました！現在の姿勢を基準点（0度）に設定しました。")
            
            # 【新規】基準点からの相対角度（オフセット引き算）に変換
            roll -= self.initial_roll
            pitch -= self.initial_pitch
            yaw -= self.initial_yaw
            
            # 相対角度からクォータニオンを生成
            cy, sy = math.cos(yaw * 0.5), math.sin(yaw * 0.5)
            cp, sp = math.cos(pitch * 0.5), math.sin(pitch * 0.5)
            cr, sr = math.cos(roll * 0.5), math.sin(roll * 0.5)
            
            qw = cr * cp * cy + sr * sp * sy
            qx = sr * cp * cy - cr * sp * sy
            qy = cr * sp * cy + sr * cp * sy
            qz = cr * cp * sy - sr * sp * cr
            self.publish_imu(qw, qx, qy, qz)

    def publish_imu(self, qw, qx, qy, qz):
        now = self.get_clock().now().to_msg()
        
        # 1. IMUメッセージのパブリッシュ
        imu_msg = Imu()
        imu_msg.header.stamp = now
        imu_msg.header.frame_id = self.frame_id
        imu_msg.orientation.w = qw
        imu_msg.orientation.x = qx
        imu_msg.orientation.y = qy
        imu_msg.orientation.z = qz
        imu_msg.orientation_covariance = [1e-6, 0.0, 0.0, 0.0, 1e-6, 0.0, 0.0, 0.0, 1e-6]
        self.imu_pub.publish(imu_msg)
        
        # 2. TFブロードキャスト (odom -> imu_link)
        t = TransformStamped()
        t.header.stamp = now
        t.header.frame_id = 'odom'
        t.child_frame_id = self.frame_id
        t.transform.translation.x = 0.0
        t.transform.translation.y = 0.0
        t.transform.translation.z = 0.0
        t.transform.rotation = imu_msg.orientation
        self.tf_broadcaster.sendTransform(t)

def main(args=None):
    import sys
    rclpy.init(args=args)
    try:
        node = UM7DriverNode()
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f"Unhandled Exception: {e}", file=sys.stderr)
    finally:
        rclpy.try_shutdown()

if __name__ == '__main__':
    main()
