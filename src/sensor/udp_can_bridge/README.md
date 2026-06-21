# udp_can_bridge

対象機器とUDPで通信し、接続が確認できた場合にROS 2 Jazzyのノードとして起動するパッケージです。

## 概要
このパッケージは以下の順序で動作します。
1. 起動時に対象デバイス（デフォルト `127.0.0.1:8888`）へ向けてUDPで `PING` パケットを送信します。
2. 返答がない場合は定期的にパケットを再送信します。
3. 対象デバイスからの返答を受信し接続が確認できると、ROS 2ノード (`UDPCANBridge`) として登録され実行されます。

将来的に、ROS 2のトピックをサブスクライブしてそのデータをUDPで送信するなどの拡張が可能な設計になっています。

## ディレクトリ構成
```text
udp_can_bridge/
├── CMakeLists.txt
├── package.xml
├── README.md
├── include/
│   └── udp_can_bridge/
│       ├── udp_can_bridge.hpp # ROS 2ノードのヘッダー
│       └── udp_manager.hpp    # UDP通信管理クラスのヘッダー
├── scripts/
│   └── test_udp_server.py     # 接続テスト用のダミーUDPサーバー
└── src/
    ├── main.cpp               # エントリーポイント (接続確認とノード起動)
    ├── udp_can_bridge.cpp     # ROS 2ノードの実装
    └── udp_manager.cpp        # UDP通信管理クラスの実装
```

## ビルド方法
Jazzy環境(distrobox)内でビルドします。

```bash
distrobox enter 26a-jazzy
cd /home/umypc/workspace/distrobox/26a_ws
colcon build --packages-select udp_can_bridge
source install/setup.bash
```

## 動作テストの実行方法

ノードが対象デバイスの応答を待機する処理をテストするために、ダミーのUDPサーバーを使います。端末（ターミナル）を2つ用意してください。

### 端末1: テスト用UDPサーバーの起動
Pythonスクリプトを実行して、ポート `8888` で待機させます。
```bash
# distrobox環境に入る
distrobox enter 26a-jazzy
cd /home/umypc/workspace/distrobox/26a_ws/src/sensor/udp_can_bridge

# テストサーバーを実行
python3 scripts/test_udp_server.py
```

### 端末2: ROS 2ノードの起動
別の端末からノードを起動します。
```bash
# distrobox環境に入る
distrobox enter 26a-jazzy
cd /home/umypc/workspace/distrobox/26a_ws
source install/setup.bash

# ノードを実行
ros2 run udp_can_bridge udp_can_bridge_node
```

### テストの成功確認
* **端末1 (サーバー側):** ノードから `PING` を受信し、`PONG` を返信したログが出力されます。
* **端末2 (ノード側):** `Connection confirmed.` と表示された後、ROS 2のノードが初期化され `[INFO] [udp_can_bridge]: UDPCANBridge node initialized and active!` と出力されればテスト成功です。

## 対象デバイスのIPとポートの変更
実機に合わせて設定を変更する場合は、`src/main.cpp` の以下の箇所を修正し、再度 `colcon build` してください。

```cpp
// src/main.cpp
const std::string target_ip = "192.168.1.xxx"; // 対象機器のIP
const uint16_t target_port = 8888;             // 対象機器のポート
```
