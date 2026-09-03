# 26a機体 CANフォーマット準拠 
uint8 priority      # 0~4 (4bit)
uint8 board_num     # 0x0~0xF (4bit)

uint8 mode #test
int16 rps_target
float32 angle_target
bool monitor_peirod
uint16 monitor_freq
uint8 motor_type
float32 gear_ratio
uint32 encoder_pulse
bool en_limit_sw