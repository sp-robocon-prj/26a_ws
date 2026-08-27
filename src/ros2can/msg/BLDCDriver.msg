# 26a機体 CANフォーマット準拠 UDPパケット https://github.com/sp-robocon-prj/can_format_26a
uint8 priority      # 0~4 (4bit)
uint8 board_num     # 0x0~0xF (4bit)

uint8 mode #test
uint16 rps_target
float32 angle_target
uint16 rps
float32 inc_pos
uint16 angle
uint16 monitor_peirod
uint64 monitor_reg
uint8 motor_type
float32 gear_ratio
uint32 encoder_pulse
uint8 en_limit_sw