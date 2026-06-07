# elp_gs800_stereo

ELP GS800 ステレオUSBカメラ用の ROS 2 (Jazzy) パッケージです。このパッケージは、V4L2とOpenCVを使用してUSBカメラに接続し（高フレームレートを実現するためにMJPGコーデックを強制しています）、標準的なROS 2の画像トピックを出力します。

## 出力トピック
- `/image_raw` (`sensor_msgs/msg/Image`): カメラからの生画像フレーム。
- `/camera_info` (`sensor_msgs/msg/CameraInfo`): カメラのメタデータ（デフォルトでは未キャリブレーション）。

## パラメータ
| パラメータ | 型 | デフォルト値 | 説明 |
|---|---|---|---|
| `video_device` | string | `/dev/video4` | ビデオデバイスのパス。 |
| `image_width` | int | `1280` | 画像の幅。 |
| `image_height` | int | `480` | 画像の高さ。 |
| `fps` | double | `60.0` | 目標フレームレート。MJPGコーデックが強制されます。 |
| `frame_id` | string | `camera_link` | ヘッダーに設定されるフレームID。 |
| `flip_vertical` | bool | `False` | `true` に設定すると画像を上下反転します。 |
| `flip_horizontal`| bool | `False` | `true` に設定すると画像を左右反転（鏡）します。 |

## ビルド手順
```bash
cd ~/workspace/distrobox/26a_ws
colcon build --packages-select elp_gs800_stereo
source install/setup.bash
```

## 実行手順

### 基本的な使い方
```bash
ros2 run elp_gs800_stereo camera_node
```

### デバイスやFPSの変更
カメラが別のビデオインデックスで認識されている場合:
```bash
ros2 run elp_gs800_stereo camera_node --ros-args -p video_device:=/dev/video4 -p fps:=60.0
```

### 画像の反転と鏡像
画像を上下反転（vertical）または左右反転（horizontal）させる場合:
```bash
# 上下反転
ros2 run elp_gs800_stereo camera_node --ros-args -p flip_vertical:=true

# 左右反転（鏡像）
ros2 run elp_gs800_stereo camera_node --ros-args -p flip_horizontal:=true

# 両方（180度回転）
ros2 run elp_gs800_stereo camera_node --ros-args -p flip_vertical:=true -p flip_horizontal:=true
```

## プレビュースクリプト
`/image_raw` トピックをOpenCVを使って簡単に確認できるよう、`test` ディレクトリにプレビュースクリプトを用意しています。

新しいターミナルを開いて実行します:
```bash
# ROS 2環境のロード
source /opt/ros/jazzy/setup.bash

# プレビュースクリプトの実行
/home/umypc/workspace/distrobox/26a_ws/src/sensor/elp_gs800_stereo/test/preview.py
```
