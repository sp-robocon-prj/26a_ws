# wt901c485_imu — ROS2 Humble driver for WitMotion WT901C-485

WT901C-485のROS2 Humbleパッケージです。  
USB-RS485変換アダプタ経由でWT901C-485をModbus RTUで読み取り、
`sensor_msgs/Imu` と `sensor_msgs/MagneticField` をパブリッシュします。

ROS2 humble (ubuntu22.04)をサポートします。

---

## パッケージ構成

```
wt901c485_imu/
├── CMakeLists.txt
├── package.xml
├── setup.py
├── config/
│   └── wt901c485_params.yaml      # パラメータ設定
├── include/wt901c485_imu/
│   └── wt901c485_driver.hpp       # C++ドライバ ヘッダ
├── src/
│   ├── wt901c485_driver.cpp       # C++ Modbus RTUドライバ実装
│   └── wt901c485_node.cpp         # C++ ROS2ノード
├── wt901c485_imu/
│   ├── __init__.py
│   └── driver.py                  # Python Modbus RTUドライバ
├── scripts/
│   └── wt901c485_py_node.py       # Python ROS2ノード
├── launch/
│   └── wt901c485.launch.py        # 起動ファイル
└── rviz/
    └── wt901c485.rviz             # RViz2設定
```

---

## 1. 依存インストール

```bash
# pyserial（Python ノード用）
sudo apt install python3-serial

# RViz2 IMU プラグイン（IMUの3Dボックス表示用）
sudo apt install ros-humble-rviz-imu-plugin

# TF2 geometry_msgs
sudo apt install ros-humble-tf2-geometry-msgs
```

---

## 2. ビルド

```bash
# ワークスペースにコピー
cp -r wt901c485_imu ~/ros2_ws/src/

cd ~/ros2_ws
colcon build --packages-select wt901c485_imu
source install/setup.bash
```

---

## 3. シリアルポート権限設定

```bash
# ユーザーをdialoutグループに追加（再ログイン必要）
sudo usermod -aG dialout $USER

# または一時的に権限付与
sudo chmod 666 /dev/ttyUSB0
```

アダプタのポートを確認する方法：
```bash
ls /dev/ttyUSB*
# または
dmesg | grep tty
```

---

## 4. 起動

### C++ノード（デフォルト）+ RViz2
```bash
ros2 launch wt901c485_imu wt901c485.launch.py
```

### Pythonノード + RViz2
```bash
ros2 launch wt901c485_imu wt901c485.launch.py use_python:=true
```

### ポートやレートを変更する場合
```bash
ros2 launch wt901c485_imu wt901c485.launch.py \
    port:=/dev/ttyUSB1 \
    rate_hz:=100.0 \
    modbus_address:=80
```

### RViz2なしで起動
```bash
ros2 launch wt901c485_imu wt901c485.launch.py launch_rviz:=false
```

---

## 5. トピック一覧

| トピック    | 型                             | 内容                      |
|-------------|-------------------------------|--------------------------|
| `imu/data`  | `sensor_msgs/Imu`             | クォータニオン+角速度+加速度 |
| `imu/mag`   | `sensor_msgs/MagneticField`   | 3軸磁場 [T]              |

TF ブロードキャスト：`world → base_link → imu_link`

---

## 6. パラメータ

| パラメータ        | デフォルト      | 説明                          |
|------------------|---------------|------------------------------|
| `port`           | `/dev/ttyUSB0`| シリアルポート                 |
| `baud`           | `9600`        | ボーレート（工場デフォルト）    |
| `modbus_address` | `80` (=0x50)  | Modbusスレーブアドレス（10進）  |
| `frame_id`       | `imu_link`    | IMUのTFフレーム名              |
| `rate_hz`        | `50.0`        | ポーリングレート [Hz]          |
| `publish_tf`     | `true`        | TFをブロードキャストするか     |

---

## 7. Modbusレジスタマップ

| アドレス | データ    | スケール              |
|---------|----------|----------------------|
| 0x0034  | AX AY AZ | ÷32768 × 16g [m/s²] |
| 0x0037  | GX GY GZ | ÷32768 × 2000°/s    |
| 0x003A  | HX HY HZ | 磁場 (μT)            |
| 0x003D  | Roll Pitch Yaw | ÷32768 × 180° |
| 0x0051  | Q0 Q1 Q2 Q3  | ÷32768 (四元数)   |

---

## 8. 解決

| 症状              | 原因                    | 対処                          |
|------------------|------------------------|------------------------------|
| `Cannot open port` | 権限不足 / ポート違い   | `dialout`グループ追加、ポート確認 |
| CRCエラー        | A/B配線逆               | AとBを入れ替える               |
| 値がゼロ         | ボーレート不一致         | センサー側と`baud`パラメータを合わせる |
| IMUボックスが見えない | rviz_imu_plugin未インストール | `apt install ros-humble-rviz-imu-plugin` |
