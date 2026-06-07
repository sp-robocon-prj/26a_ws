"""
wt901c485_imu/driver.py
Pure-Python Modbus RTU driver for WitMotion WT901C-485.
Requires: pyserial  (pip install pyserial)
"""

from __future__ import annotations
import struct
import serial
from dataclasses import dataclass, field
from typing import List

# ── Scale factors from datasheet ──────────────────────────────────────────────
_G = 9.80665          # m/s²
_ACC_SCALE  = 16.0 * _G / 32768.0      # m/s²
_GYRO_SCALE = 2000.0 / 32768.0         # °/s  (caller converts to rad/s)
_ANG_SCALE  = 180.0  / 32768.0         # °    (caller converts to rad)
_QUAT_SCALE = 1.0    / 32768.0
_MAG_SCALE  = 1.0e-6                   # raw → Tesla (approx)

# ── Modbus register start addresses ──────────────────────────────────────────
_REG_ACC   = 0x0034   # AX AY AZ       (3 registers)
_REG_GYRO  = 0x0037   # GX GY GZ       (3 registers)
_REG_ANGLE = 0x003D   # Roll Pitch Yaw (3 registers)
_REG_QUAT  = 0x0051   # Q0 Q1 Q2 Q3   (4 registers)
_REG_MAG   = 0x003A   # HX HY HZ      (3 registers)


@dataclass
class ImuData:
    # Acceleration [m/s²]
    acc: List[float] = field(default_factory=lambda: [0.0, 0.0, 0.0])
    # Angular velocity [rad/s]
    gyro: List[float] = field(default_factory=lambda: [0.0, 0.0, 0.0])
    # Euler angles [rad]
    euler: List[float] = field(default_factory=lambda: [0.0, 0.0, 0.0])
    # Quaternion [w, x, y, z]
    quat: List[float] = field(default_factory=lambda: [1.0, 0.0, 0.0, 0.0])
    # Magnetic field [T]
    mag: List[float] = field(default_factory=lambda: [0.0, 0.0, 0.0])


def _crc16(data: bytes) -> int:
    """Modbus CRC-16 (polynomial 0xA001)."""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc


def _build_request(address: int, reg_start: int, reg_count: int) -> bytes:
    """Build a Modbus RTU Read Holding Registers (0x03) request."""
    pdu = struct.pack('>BBHH', address, 0x03, reg_start, reg_count)
    crc = _crc16(pdu)
    return pdu + struct.pack('<H', crc)   # CRC little-endian


def _parse_response(raw: bytes, reg_count: int) -> List[int]:
    """
    Parse a Modbus RTU response into a list of signed 16-bit integers.
    Raises ValueError on CRC mismatch or unexpected length.
    """
    expected = 3 + reg_count * 2 + 2
    if len(raw) < expected:
        raise ValueError(f"Short response: got {len(raw)}, expected {expected}")
    crc_calc = _crc16(raw[:-2])
    crc_recv = struct.unpack_from('<H', raw, len(raw) - 2)[0]
    if crc_calc != crc_recv:
        raise ValueError(f"CRC mismatch: calc=0x{crc_calc:04X} recv=0x{crc_recv:04X}")
    values = []
    for i in range(reg_count):
        raw_val = struct.unpack_from('>H', raw, 3 + i * 2)[0]
        signed  = struct.unpack('>h', struct.pack('>H', raw_val))[0]
        values.append(signed)
    return values


class WT901C485Driver:
    """
    Blocking Modbus RTU driver for the WT901C-485.

    Usage::
        drv = WT901C485Driver('/dev/ttyUSB0')
        drv.open()
        data = drv.read()
        drv.close()
    """

    def __init__(self,
                 port: str = '/dev/ttyUSB0',
                 baud: int = 9600,
                 address: int = 0x50,
                 timeout: float = 0.5):
        self._port    = port
        self._baud    = baud
        self._address = address
        self._timeout = timeout
        self._ser: serial.Serial | None = None

    # ── Lifecycle ────────────────────────────────────────────────────────────
    def open(self) -> None:
        self._ser = serial.Serial(
            port     = self._port,
            baudrate = self._baud,
            bytesize = serial.EIGHTBITS,
            parity   = serial.PARITY_NONE,
            stopbits = serial.STOPBITS_ONE,
            timeout  = self._timeout,
        )

    def close(self) -> None:
        if self._ser and self._ser.is_open:
            self._ser.close()

    def is_open(self) -> bool:
        return self._ser is not None and self._ser.is_open

    # ── Read ─────────────────────────────────────────────────────────────────
    def read(self) -> ImuData:
        """Read all sensor data and return an ImuData instance."""
        import math
        data = ImuData()

        # Acceleration
        regs = self._transact(_REG_ACC, 3)
        data.acc = [v * _ACC_SCALE for v in regs]

        # Angular velocity (convert °/s → rad/s)
        regs = self._transact(_REG_GYRO, 3)
        data.gyro = [v * _GYRO_SCALE * math.pi / 180.0 for v in regs]

        # Euler angles (convert ° → rad)
        regs = self._transact(_REG_ANGLE, 3)
        data.euler = [v * _ANG_SCALE * math.pi / 180.0 for v in regs]

        # Quaternion
        regs = self._transact(_REG_QUAT, 4)
        data.quat = [v * _QUAT_SCALE for v in regs]  # [w, x, y, z]

        # Magnetic field
        regs = self._transact(_REG_MAG, 3)
        data.mag = [v * _MAG_SCALE for v in regs]

        return data

    # ── Private helpers ──────────────────────────────────────────────────────
    def _transact(self, reg_start: int, reg_count: int) -> List[int]:
        if not self.is_open():
            raise RuntimeError("Serial port is not open")
        req = _build_request(self._address, reg_start, reg_count)
        self._ser.reset_input_buffer()
        self._ser.write(req)
        expected = 3 + reg_count * 2 + 2
        resp = self._ser.read(expected)
        return _parse_response(resp, reg_count)
