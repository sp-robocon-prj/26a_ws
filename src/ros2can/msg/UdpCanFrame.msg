# 26a機体 CANフォーマット準拠 UDPパケット https://github.com/sp-robocon-prj/can_format_26a
uint8 priority      # 0~4 (4bit)
uint8 data_type     # 0x1~0xC (4bit)
uint8 board_num     # 0x0~0xF (4bit)
uint16 register_id  # 0x0000~0xFFFF (16bit)
uint8 dlc           # データ長 (最大64)
uint8[64] data      # データ本体 (リトルエンディアン)
