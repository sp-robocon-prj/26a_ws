import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from std_msgs.msg import Float64MultiArray
import math

class OmniKinematicsNode(Node):
    def __init__(self):
        super().__init__('omni_kinematics_node')
        
        # Declare parameters for tuning
        self.declare_parameter('wheel_radius', 0.05) # adjust as needed
        self.declare_parameter('robot_radius', 0.54) # roughly 0.38 * sqrt(2)
        
        self.wheel_radius = self.get_parameter('wheel_radius').value
        self.robot_radius = self.get_parameter('robot_radius').value
        
        self.subscription = self.create_subscription(
            Twist,
            'cmd_vel',
            self.cmd_vel_callback,
            10)
            
        self.publisher_ = self.create_publisher(
            Float64MultiArray,
            '/omni_wheel_controller/commands',
            10)

    def cmd_vel_callback(self, msg):
        vx = msg.linear.x
        vy = msg.linear.y
        wz = msg.angular.z
        
        R = self.wheel_radius
        D = self.robot_radius
        
        # X-drive kinematics (adjusted for URDF axis orientations)
        v_fl = ( vx - vy - wz * D) / R
        v_fr = (-vx - vy - wz * D) / R
        v_rl = ( vx + vy - wz * D) / R
        v_rr = (-vx + vy - wz * D) / R
        
        cmd_msg = Float64MultiArray()
        cmd_msg.data = [v_fl, v_fr, v_rl, v_rr]
        self.publisher_.publish(cmd_msg)

def main(args=None):
    rclpy.init(args=args)
    node = OmniKinematicsNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
