import os
import glob

replacements = {
    "omni_4_1": "wheel_front_left_link",
    "omni_1_1": "wheel_front_right_link",
    "omni_3_1": "wheel_rear_left_link",
    "omni_2_1": "wheel_rear_right_link",
    "回転 7": "wheel_front_left_joint",
    "回転 6": "wheel_front_right_joint",
    "回転 8": "wheel_rear_left_joint",
    "回転 5": "wheel_rear_right_joint",
    "r_lidar_1": "lidar_link",
    "r_cam_1": "camera_link",
    "r_cam_body_1": "camera_body_link",
    "r_lidar_head_1": "lidar_head_link",
    "回転 9": "lidar_joint",
    "回転 14": "camera_joint",
    "剛性 15": "camera_body_joint",
    "回転 17": "lidar_head_joint"
}

urdf_dir = "/home/umypc/workspace/distrobox/26a_ws/src/robot26a_urdf/urdf"
files = glob.glob(os.path.join(urdf_dir, "*.*"))

for fpath in files:
    with open(fpath, "r", encoding="utf-8") as f:
        content = f.read()
    for old, new in replacements.items():
        content = content.replace(old, new)
    with open(fpath, "w", encoding="utf-8") as f:
        f.write(content)
