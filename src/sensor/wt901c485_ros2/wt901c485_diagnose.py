#!/usr/bin/env python3
"""
wt901c485_diagnose.py
WT901C-485 の通信診断ツール。
ボーレートとModbusアドレスを総当たりしてセンサーを検出します。

使い方:
  python3 wt901c485_diagnose.py                   # デフォルト /dev/ttyUSB0
  python3 wt901c485_diagnose.py /dev/ttyUSB1
"""

import sys
import struct
import time
import serial

PORT = sys.argv[1] if len(sys.argv) > 1 else '/dev/ttyUSB0'

# WT901C-485 の工場デフォルトは 9600 baud / addr 0x50
# ただし上位ソフトで変更されている場合があるため全候補をスキャン
BAUDS    = [9600, 4800, 19200, 38400, 57600, 115200]
ADDRS    = [0x50, 0x01, 0x02, 0x03, 0xFF]   # 0x50=80 が工場デフォルト

# Modbus CRC-16
def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc

def build_req(addr: int, reg: int, count: int) -> bytes:
    pdu = struct.pack('>BBHH', addr, 0x03, reg, count)
    return pdu + struct.pack('<H', crc16(pdu))

def try_read(ser: serial.Serial, addr: int) -> bool:
    """加速度レジスタ(0x0034, 3個)を読んでみる。成功したらTrue。"""
    req = build_req(addr, 0x0034, 3)
    ser.reset_input_buffer()
    ser.write(req)
    time.sleep(0.15)
    resp = ser.read(11)   # 3 + 3*2 + 2 = 11 bytes
    if len(resp) < 11:
        return False
    crc_calc = crc16(resp[:-2])
    crc_recv = struct.unpack_from('<H', resp, 9)[0]
    if crc_calc != crc_recv:
        return False
    if resp[0] != addr or resp[1] != 0x03:
        return False
    # パース
    ax = struct.unpack('>h', resp[3:5])[0] / 32768 * 16 * 9.80665
    ay = struct.unpack('>h', resp[5:7])[0] / 32768 * 16 * 9.80665
    az = struct.unpack('>h', resp[7:9])[0] / 32768 * 16 * 9.80665
    print(f"    → AX={ax:+.3f}  AY={ay:+.3f}  AZ={az:+.3f}  [m/s²]")
    return True

def raw_bytes_test(ser: serial.Serial) -> None:
    """生のバイトを表示してセンサーが何か送ってきているか確認。"""
    ser.reset_input_buffer()
    # 何も送らずに100ms待ち（自発送信モードの場合）
    time.sleep(0.1)
    raw = ser.read(32)
    if raw:
        print(f"  [raw] センサーから自発送信あり ({len(raw)} bytes): {raw.hex(' ')}")
    else:
        print("  [raw] 自発送信なし（Modbusモードと推定）")

# ── メイン ──────────────────────────────────────────────────────────────────
print(f"{'='*60}")
print(f" WT901C-485 診断ツール  ポート: {PORT}")
print(f"{'='*60}\n")

found = []

for baud in BAUDS:
    print(f"[baud={baud}] 試行中...")
    try:
        ser = serial.Serial(PORT, baud,
                            bytesize=8, parity='N', stopbits=1,
                            timeout=0.3)
    except Exception as e:
        print(f"  ポートオープン失敗: {e}")
        break

    raw_bytes_test(ser)

    for addr in ADDRS:
        print(f"  addr=0x{addr:02X}({addr}) ... ", end='', flush=True)
        ok = try_read(ser, addr)
        if ok:
            print(f"  ✓ 成功！ baud={baud}, addr=0x{addr:02X}({addr})")
            found.append((baud, addr))
        else:
            print("応答なし")

    ser.close()
    if found:
        break   # 見つかったら終了

print()
if found:
    baud, addr = found[0]
    print("="*60)
    print(f"  検出成功！")
    print(f"  baud={baud}  modbus_address={addr}  (0x{addr:02X})")
    print()
    print("  config/wt901c485_params.yaml を以下に変更してください:")
    print(f"    baud: {baud}")
    print(f"    modbus_address: {addr}")
    print()
    print("  起動コマンド:")
    print(f"    ros2 launch wt901c485_imu wt901c485.launch.py \\")
    print(f"      port:={PORT} baud:={baud} modbus_address:={addr}")
    print("="*60)
else:
    print("="*60)
    print("  センサーが見つかりませんでした。")
    print()
    print("  確認事項:")
    print("  1. A↔A, B↔B の配線になっているか（逆接するとCRCエラーになります）")
    print("  2. VCC/GND が正しく接続されているか")
    print("  3. ls /dev/ttyUSB* でポートが見えているか")
    print(f"  4. sudo chmod 666 {PORT} を実行したか")
    print("  5. WitMotion専用ソフト(PC)でセンサーの現在設定を確認する")
    print("="*60)
