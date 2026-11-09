"""passowrd.cpp equivalent: four IR digits unlock; # resets and locks."""

from machine import Pin, PWM
from time import sleep_ms, ticks_ms, ticks_add, ticks_diff
from keyes_ir_esp32 import NECReceiver, REPEAT

class Buzzer:
    """Play three notes without sleeping through incoming IR frames."""

    def __init__(self, pin=2):
        self.pin_number = pin
        self.pwm = PWM(Pin(pin, Pin.OUT, value=0), freq=131, duty_u16=0)
        self.notes = ()
        self.index = 0
        self.deadline = 0

    def play(self, unlocked):
        self.notes = (131, 165, 196) if unlocked else (196, 165, 131)
        self.index = 0
        self._start_note(ticks_ms())

    def _start_note(self, now):
        self.pwm.duty_u16(0)
        self.pwm.freq(self.notes[self.index])
        self.pwm.duty_u16(32768)
        self.deadline = ticks_add(now, 120)

    def update(self):
        now = ticks_ms()
        if self.notes and ticks_diff(now, self.deadline) >= 0:
            self.index += 1
            if self.index == len(self.notes):
                self.pwm.duty_u16(0)
                self.notes = ()
            else:
                self._start_note(now)

    def close(self):
        self.pwm.duty_u16(0)
        self.pwm.deinit()
        Pin(self.pin_number, Pin.OUT, value=0)


def servo_duty_u16(angle):
    """Original GPIO23 servo mapping: 25..128 / 1023 at 50 Hz.

    Helper only: password.py does not move a servo, just like the C++ entry.
    """
    angle = max(0, min(180, int(angle)))
    duty_10bit = 25 + angle * 103 // 180
    return duty_10bit * 65535 // 1023


PASSWORD = "6767"  # Keep exactly four digits; strings also allow leading zeros.
RESET_KEY = 16732845
DIGIT_KEYS = {
    16738455: "1", 16750695: "2", 16756815: "3",
    16724175: "4", 16718055: "5", 16743045: "6",
    16716015: "7", 16726215: "8", 16734885: "9",
    16730805: "0",
}


class PasswordControl:
    def __init__(self, password=PASSWORD):
        if (len(password) != 4 or
                any(character not in "0123456789" for character in password)):
            raise ValueError("Password must contain exactly four ASCII digits")
        self.password = password
        self.entered = ""
        self.unlocked = False

    def receive(self, code):
        """Return 'locked', 'unlocked', 'wrong', or None for no event."""
        if code == REPEAT:
            return None  # A held numeric key must not enter repeated digits.
        if code == RESET_KEY:
            self.entered = ""
            self.unlocked = False
            return "locked"
        digit = DIGIT_KEYS.get(code)
        if digit is None:
            return None
        self.entered += digit
        if len(self.entered) == 4:
            correct = self.entered == self.password
            self.entered = ""
            if correct:
                self.unlocked = True
                return "unlocked"
            # Preserve the original: a wrong attempt does not relock.
            return "wrong"
        return None


def main():
    buzzer = Buzzer(2)
    try:
        receiver = NECReceiver(19)
        control = PasswordControl()
        print("Password ready: enter four digits; # locks and clears input.")
        while True:
            buzzer.update()
            code = receiver.read()
            buzzer.update()
            if code is not None:
                event = control.receive(code)
                if event == "unlocked":
                    buzzer.play(True)
                    print("Unlocked")
                elif event == "locked":
                    buzzer.play(False)
                    print("Locked; input cleared")
                elif event == "wrong":
                    print("Wrong password; input cleared")
            sleep_ms(1)
    finally:
        buzzer.close()


if __name__ == "__main__":
    main()
