#include <Keyes_IR_ESP32.h>

float irButtonKey;
float thisButtonKey;
Keyes_ESP32_IR_Recv ir_recv(19);

void task_1(void * pvParameters) {
    for (;;) {
        ir_recv.task();
  if (ir_recv.nec_available()) {
    thisButtonKey = ir_recv.data();
    if (!(thisButtonKey == 4294967295)) {
      irButtonKey = thisButtonKey;
    }
    if (irButtonKey == 16736925) {
      beetleMove_towards_N_speed_N_coldown_N(1, 255, 0.3);
    }
    if (irButtonKey == 16754775) {
      beetleMove_towards_N_speed_N_coldown_N(2, 255, 0.3);
    }
    if (irButtonKey == 16761405) {
      beetleMove_towards_N_speed_N_coldown_N(3, 150, 0.05);
    }
    if (irButtonKey == 16720605) {
      beetleMove_towards_N_speed_N_coldown_N(4, 150, 0.05);
    }
    beetleStop();
  }
        vTaskDelay(1);
    }
}


void beetleStop() {
  digitalWrite(33, LOW);
  ledcWrite(26, 0);
  digitalWrite(32, LOW);
  ledcWrite(25, 0);
}


void beetleMove_towards_N_speed_N_coldown_N(float towards, float speed, float coldown) {
  if (towards == 1) {
    digitalWrite(33, LOW);
    ledcWrite(26, (speed - 1));
    digitalWrite(32, LOW);
    ledcWrite(25, speed);
  }
  if (towards == 2) {
    digitalWrite(33, HIGH);
    ledcWrite(26, ((255 - speed) - 1));
    digitalWrite(32, HIGH);
    ledcWrite(25, (255 - speed));
  }
  if (towards == 3) {
    digitalWrite(33, LOW);
    ledcWrite(26, (speed - 1));
    digitalWrite(32, HIGH);
    ledcWrite(25, (255 - speed));
  }
  if (towards == 4) {
    digitalWrite(33, HIGH);
    ledcWrite(26, ((255 - speed) - 1));
    digitalWrite(32, LOW);
    ledcWrite(25, speed);
  }
  delay(coldown * 1000);
}


void setup() {
  ledcAttachChannel(26,490,8,2);
  ledcAttachChannel(25,490,8,3);
  pinMode(33, OUTPUT);
    pinMode(32, OUTPUT);
    beetleStop();
      xTaskCreatePinnedToCore(task_1, "task_1", 4096, NULL, 2, NULL, 0);
  vTaskDelay(1);
}

void loop() {
}
