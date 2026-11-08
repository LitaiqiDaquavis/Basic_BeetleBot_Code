#include <Keyes_IR_ESP32.h>

float irButtonKey;
float thisButtonKey;
float lock;
float userInput;
float currentNum;
float digits;
Keyes_ESP32_IR_Recv ir_recv(19);

void task_1(void * pvParameters) {
    for (;;) {
        ir_recv.task();
  if (ir_recv.nec_available()) {
    thisButtonKey = ir_recv.data();
    if (!(thisButtonKey == 4294967295)) {
      irButtonKey = thisButtonKey;
      beetlePassword_password_N(6767);
    }
  }
        vTaskDelay(1);
    }
}

int beetlebot_servo_angle_23(int angle) {
  return map(constrain(angle, 0, 180), 0, 180, 25, 128);
}


void determineCurrentNumber() {
  if (thisButtonKey == 16738455) {
    currentNum = 1;
  }
  if (thisButtonKey == 16750695) {
    currentNum = 2;
  }
  if (thisButtonKey == 16756815) {
    currentNum = 3;
  }
  if (thisButtonKey == 16724175) {
    currentNum = 4;
  }
  if (thisButtonKey == 16718055) {
    currentNum = 5;
  }
  if (thisButtonKey == 16743045) {
    currentNum = 6;
  }
  if (thisButtonKey == 16716015) {
    currentNum = 7;
  }
  if (thisButtonKey == 16726215) {
    currentNum = 8;
  }
  if (thisButtonKey == 16734885) {
    currentNum = 9;
  }
  if (thisButtonKey == 16730805) {
    currentNum = 0;
  }
}


void beetlePassword_password_N(float password) {
  if (thisButtonKey == 16732845) {
    hashtagReset();
  }
  else{
    currentNum = -1;
    determineCurrentNumber();
    if (currentNum > -1) {
      userInput = userInput * 10 + currentNum;
      digits += 1;
      if (digits == 4) {
        if (userInput == password) {
          irButtonKey = 0;
          lock = 1;
          unlockAndLockSound();
        }
        userInput = 0;
        digits = 0;
      }
    }
  }
}


void unlockAndLockSound() {
  noTone(2);
  if (lock == 1) {
    tone(2, 131, 120);
    tone(2, 165, 120);
    tone(2, 196, 120);
  }
  else{
    tone(2, 196, 120);
    tone(2, 165, 120);
    tone(2, 131, 120);
  }
}


void hashtagReset() {
  irButtonKey = 0;
  lock = 0;
  userInput = 0;
  digits = 0;
  unlockAndLockSound();
}


void _E7_B2_89_E7_A2_8E_E9_BD_BF_E8_BD_AE__E5_BC_80_E5_85_B3_N__E5_86_B7_E5_8D_B4_E6_97_B6_E9_97_B4_N(float _E5_BC_80_E5_85_B3, float _E5_86_B7_E5_8D_B4_E6_97_B6_E9_97_B4) {
  if (_E5_BC_80_E5_85_B3 == 1) {
    ledcWrite(23, beetlebot_servo_angle_23(0));
    delay(_E5_86_B7_E5_8D_B4_E6_97_B6_E9_97_B4 * 1000);
    ledcWrite(23, beetlebot_servo_angle_23(180));
    delay(_E5_86_B7_E5_8D_B4_E6_97_B6_E9_97_B4 * 1000);
  }
  if (_E5_BC_80_E5_85_B3 == 0) {
    ledcWrite(23, beetlebot_servo_angle_23(0));
  }
}


void setup() {
  pinMode(2, OUTPUT);

  ledcAttachChannel(23,50,10,4);

  lock = 0;
    userInput = 0;
    digits = 0;
      xTaskCreatePinnedToCore(task_1, "task_1", 4096, NULL, 2, NULL, 0);
  vTaskDelay(1);
}

void loop() {
}
