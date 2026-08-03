# controller パッケージ

このパッケージは、ロボットの運動学（キネマティクス）演算と上位の速度制御を担います。`cmd_vel` (Twist) を入力とし、設定された足回りの種類に応じて各車輪の目標速度 (`wheel_vel`) を算出します。

## 含まれるノード
- `twist2velocity_node` (レイヤー3): `cmd_vel` (Twist) を購読し、ロボット自身の並進・旋回速度 (`RobotVelocityCommand`) に変換します。
- `velocity2omni_node` (レイヤー2): ロボット速度 (`RobotVelocityCommand`) を購読し、パラメータで指定された足回りの種類に基づいて各車輪の目標速度 (`WheelVelocityCommand`) に変換（逆運動学）します。

## カスタムメッセージ
- `RobotVelocityCommand.msg`: ロボットの目標速度 ($v_x, v_y, \omega$)
- `WheelVelocityCommand.msg`: 各車輪の目標速度（可変長配列 `float64[] velocities`）

## 起動方法

### 1. 推奨の起動方法 (Launchファイルを使用)

`controller` パッケージ内のノード群と、UDP通信を行う `ros2can` パッケージの `udp_bridge_node` を **一度にすべて起動** します。設定（足回りの種類など）も自動で読み込まれます。

```bash
ros2 launch controller controller.launch.py
```

### 2. 単独でノードを起動する場合

デバッグなどの目的で個別にノードを立ち上げる場合は、以下のように実行します。

```bash
ros2 run controller twist2velocity_node
ros2 run controller velocity2omni_node
```

## パラメータの設定

足回りの種類は、`config/controller_params.yaml` で設定されています。以下の値を変更することで、再ビルドなしで計算式と出力配列の要素数を切り替えることができます。

- `"omni4_x"` (デフォルト): 4輪対角オムニ
- `"mecanum"`: 4輪メカナム
- `"omni3"`: 3輪オムニ (正面、左後方、右後方)

YAMLファイルを書き換えた後は、一度ビルド (`colcon build`) して Launch を再起動してください。