# 26a機体 CANフォーマット準拠 UDPパケット https://github.com/sp-robocon-prj/can_format_26a
uint32 id           # id
uint32 size         # データ長 (最大64)
uint8[64] data      # データ本体 (リトルエンディアン)
