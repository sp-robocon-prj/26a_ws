# ros2can パッケージ

このパッケージは、PC (ROS2) と H723マイコン 等の間で、UDP通信を経由してCANフレームの送受信を行うためのブリッジ機能を提供します。

## 含まれるノード
- `udp_bridge_node`: ROS2トピック (`udp_can_tx`, `udp_can_rx`) とUDPソケットの相互変換を行います。

## カスタムメッセージ
- `UdpCanFrame.msg`: UDP経由で送受信するCANデータのフォーマット（Priority, Data Type, Board Num, Register ID, DLC, Data[64]）を定義しています。

## 起動方法

単独でUDP通信ブリッジを起動する場合は、以下のコマンドを実行します。

```bash
# UDP-CANブリッジノードの起動
ros2 run ros2can udp_bridge_node
```

> **注意**: 足回りの制御ノードも含めてシステム全体を起動する場合は、`controller` パッケージの Launch ファイルを使用してください。
