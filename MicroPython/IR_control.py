"""IR_control.cpp equivalent. Run this OR password.py on the ESP32."""

from machine import Pin, PWM
from time import ticks_ms, ticks_add, ticks_diff, sleep_ms, sleep_us
from keyes_ir_esp32 import NECReceiver, REPEAT

class Motors:
    def __init__(self):
        self.left_direction = Pin(33, Pin.OUT, value=0)
        self.right_direction = Pin(32, Pin.OUT, value=0)
        self.left_pwm = PWM(Pin(26, Pin.OUT, value=0), freq=490, duty_u16=0)
        self.right_pwm = PWM(Pin(25, Pin.OUT, value=0), freq=490, duty_u16=0)

    def stop(self):
        self.left_direction.value(0)
        self.right_direction.value(0)
        self.left_pwm.duty_u16(0)
        self.right_pwm.duty_u16(0)

    def move(self, direction, speed):
        # 1: forward, 2: reverse, 3: turn right, 4: turn left.
        if direction not in (1, 2, 3, 4):
            self.stop()
            return
        speed = max(0, min(255, int(speed)))
        self.stop()
        if speed == 0:
            return
        sleep_us(100)
        left_reverse = direction in (2, 4)
        right_reverse = direction in (2, 3)
        # INA high inverts effective PWM polarity on this motor driver.
        self.left_direction.value(int(left_reverse))
        self.right_direction.value(int(right_reverse))
        left_duty = 255 - speed if left_reverse else speed
        right_duty = 255 - speed if right_reverse else speed
        self.left_pwm.duty_u16(left_duty * 257)
        self.right_pwm.duty_u16(right_duty * 257)

    def close(self):
        self.stop()
        self.left_pwm.deinit()
        self.right_pwm.deinit()
        Pin(26, Pin.OUT, value=0)
        Pin(25, Pin.OUT, value=0)


# Original codes, speeds and nominal movement durations (milliseconds).
COMMANDS = {
    16736925: (1, 255, 300),  # Up: forward
    16754775: (2, 255, 300),  # Down: reverse
    16761405: (3, 150, 50),   # Right
    16720605: (4, 150, 50),   # Left
}
REPEAT_MAX_GAP_MS = 250


class DriveControl:
    def __init__(self, motors):
        self.motors = motors
        self.last_command = None
        self.last_received = None
        self.deadline = None

    def receive(self, code, now):
        if code == REPEAT:
            if (self.last_command is None or self.last_received is None or
                    ticks_diff(now, self.last_received) >= REPEAT_MAX_GAP_MS):
                return
            command = self.last_command
        else:
            command = COMMANDS.get(code)
        self.last_command = command
        if command is None:
            self.deadline = None
            self.last_received = None
            self.motors.stop()
            return
        direction, speed, duration = command
        self.motors.move(direction, speed)
        self.deadline = ticks_add(now, duration)
        self.last_received = now

    def update(self, now):
        if self.deadline is not None and ticks_diff(now, self.deadline) >= 0:
            self.motors.stop()
            self.deadline = None


def main():
    motors = Motors()
    try:
        receiver = NECReceiver(19)
        control = DriveControl(motors)
        print("IR drive ready: arrows move, any other key stops. Ctrl+C exits.")
        while True:
            control.update(ticks_ms())
            code = receiver.read()
            now = ticks_ms()
            control.update(now)
            if code is not None:
                control.receive(code, now)
            sleep_ms(1)
    finally:
        motors.close()


if __name__ == "__main__":
    main()
