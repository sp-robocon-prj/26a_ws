import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image, CameraInfo
from cv_bridge import CvBridge
import cv2

class CameraNode(Node):
    def __init__(self):
        super().__init__('camera_node')
        
        self.declare_parameter('video_device', '/dev/video4')
        self.declare_parameter('image_width', 1280)
        self.declare_parameter('image_height', 480)
        self.declare_parameter('fps', 60.0)
        self.declare_parameter('frame_id', 'camera_link')
        self.declare_parameter('flip_vertical', True)
        self.declare_parameter('flip_horizontal', True)
        
        self.video_device = self.get_parameter('video_device').value
        self.image_width = self.get_parameter('image_width').value
        self.image_height = self.get_parameter('image_height').value
        self.fps = self.get_parameter('fps').value
        self.frame_id = self.get_parameter('frame_id').value
        self.flip_vertical = self.get_parameter('flip_vertical').value
        self.flip_horizontal = self.get_parameter('flip_horizontal').value
        
        # Open with V4L2 backend
        self.cap = cv2.VideoCapture(self.video_device, cv2.CAP_V4L2)
        
        # Force MJPEG compression for high FPS over USB
        self.cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*'MJPG'))
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.image_width)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.image_height)
        self.cap.set(cv2.CAP_PROP_FPS, self.fps)
        
        if not self.cap.isOpened():
            self.get_logger().error(f"Failed to open video device: {self.video_device}")
            return
            
        actual_fps = self.cap.get(cv2.CAP_PROP_FPS)
        self.get_logger().info(f"Successfully opened video device: {self.video_device} ({self.image_width}x{self.image_height} @ requested {self.fps}fps, actual {actual_fps}fps)")
        
        self.image_pub = self.create_publisher(Image, 'image_raw', 10)
        self.camera_info_pub = self.create_publisher(CameraInfo, 'camera_info', 10)
        self.bridge = CvBridge()
        
        timer_period = 1.0 / self.fps
        self.timer = self.create_timer(timer_period, self.timer_callback)

    def timer_callback(self):
        if not hasattr(self, 'cap') or not self.cap.isOpened():
            return
            
        ret, frame = self.cap.read()
        if not ret:
            self.get_logger().warning("Failed to capture frame from camera", throttle_duration_sec=2.0)
            return
            
        # Apply flipping based on parameters
        if self.flip_vertical and self.flip_horizontal:
            frame = cv2.flip(frame, -1)
        elif self.flip_vertical:
            frame = cv2.flip(frame, 0)
        elif self.flip_horizontal:
            frame = cv2.flip(frame, 1)
            
        now = self.get_clock().now().to_msg()
        
        try:
            img_msg = self.bridge.cv2_to_imgmsg(frame, encoding="bgr8")
            img_msg.header.stamp = now
            img_msg.header.frame_id = self.frame_id
            self.image_pub.publish(img_msg)
        except Exception as e:
            self.get_logger().error(f"Failed to convert frame to ROS Image: {e}", throttle_duration_sec=2.0)
            return
            
        info_msg = CameraInfo()
        info_msg.header.stamp = now
        info_msg.header.frame_id = self.frame_id
        info_msg.width = frame.shape[1]
        info_msg.height = frame.shape[0]
        self.camera_info_pub.publish(info_msg)

    def destroy_node(self):
        if hasattr(self, 'cap') and self.cap.isOpened():
            self.cap.release()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = CameraNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()

if __name__ == '__main__':
    main()
