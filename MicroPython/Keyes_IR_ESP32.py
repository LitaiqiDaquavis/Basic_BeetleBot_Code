"""Beetlebot NEC receiver; codes keep the original Keyes bit order.

Uses MicroPython's C pulse measurement rather than the Arduino IR library.
Only NEC and NEC repeat frames are supported (not Sony/Samsung/RC5).
"""

from machine import Pin, time_pulse_us
from time import ticks_us, ticks_diff

REPEAT = 0xFFFFFFFF


class NECReceiver:
    def __init__(self, pin=19):
        self.pin = Pin(pin, Pin.IN, Pin.PULL_UP)

    def read(self):
        """Return a 32-bit code, REPEAT, or None. Call frequently.

        Idle waiting is bounded; receiving a complete frame occupies about
        70 ms. No print, sleep or callbacks run between data pulses.
        """
        leader = time_pulse_us(self.pin, 0, 12000)
        if not 7500 <= leader <= 10500:
            return None
        started = ticks_us()
        space = time_pulse_us(self.pin, 1, 6000)
        if 1700 <= space <= 2800:
            footer = time_pulse_us(self.pin, 0, 1500)
            return REPEAT if 250 <= footer <= 900 else None
        if not 3500 <= space <= 5500:
            return None

        code = 0
        for _ in range(32):
            if ticks_diff(ticks_us(), started) > 90000:
                return None
            mark = time_pulse_us(self.pin, 0, 1500)
            if not 250 <= mark <= 900:
                return None
            space = time_pulse_us(self.pin, 1, 2500)
            if 250 <= space <= 900:
                bit = 0
            elif 1200 <= space <= 2100:
                bit = 1
            else:
                return None
            # Accumulate in arrival order, as Keyes_IR_ESP32 does.
            # This intentionally differs from some NEC libraries' integers.
            code = (code << 1) | bit

        footer = time_pulse_us(self.pin, 0, 1500)
        if not 250 <= footer <= 900:
            return None
        # The supplied 17-key remote uses ordinary NEC inverse-byte checks.
        if ((code >> 24) ^ ((code >> 16) & 255)) != 255:
            return None
        if (((code >> 8) & 255) ^ (code & 255)) != 255:
            return None
        return code
