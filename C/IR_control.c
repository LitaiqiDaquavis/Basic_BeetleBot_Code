/* ESP-IDF C version of IR_control.cpp; selected by default.
 * GPIO19: IR. Left motor: 33/26. Right motor: 32/25.
 * Build from the project root: idf.py -B build-ir build
 */
#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "Keyes_IR_ESP32.h"

#define LEFT_DIRECTION  GPIO_NUM_33
#define LEFT_PWM_PIN    GPIO_NUM_26
#define RIGHT_DIRECTION GPIO_NUM_32
#define RIGHT_PWM_PIN   GPIO_NUM_25
#define PWM_MODE        LEDC_LOW_SPEED_MODE
#define LEFT_CHANNEL    LEDC_CHANNEL_0
#define RIGHT_CHANNEL   LEDC_CHANNEL_1

typedef struct {
    unsigned direction;
    unsigned speed;
    unsigned duration_ms;
} movement_t;

static void set_duty(ledc_channel_t channel, unsigned duty)
{
    ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, channel));
}

static void stop_motors(void)
{
    ESP_ERROR_CHECK(gpio_set_level(LEFT_DIRECTION, 0));
    ESP_ERROR_CHECK(gpio_set_level(RIGHT_DIRECTION, 0));
    set_duty(LEFT_CHANNEL, 0);
    set_duty(RIGHT_CHANNEL, 0);
}

static void init_motors(void)
{
    const gpio_config_t pins = {
        .pin_bit_mask = (1ULL << LEFT_DIRECTION) | (1ULL << RIGHT_DIRECTION),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&pins));
    ESP_ERROR_CHECK(gpio_set_level(LEFT_DIRECTION, 0));
    ESP_ERROR_CHECK(gpio_set_level(RIGHT_DIRECTION, 0));
    const ledc_timer_config_t timer = {
        .speed_mode = PWM_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 490,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));
    const ledc_channel_config_t channels[] = {
        {
            .gpio_num = LEFT_PWM_PIN,
            .speed_mode = PWM_MODE,
            .channel = LEFT_CHANNEL,
            .timer_sel = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE,
            .duty = 0,
            .hpoint = 0,
        },
        {
            .gpio_num = RIGHT_PWM_PIN,
            .speed_mode = PWM_MODE,
            .channel = RIGHT_CHANNEL,
            .timer_sel = LEDC_TIMER_0,
            .intr_type = LEDC_INTR_DISABLE,
            .duty = 0,
            .hpoint = 0,
        },
    };
    for (unsigned i = 0; i < 2; ++i) {
        ESP_ERROR_CHECK(ledc_channel_config(&channels[i]));
    }
    stop_motors();
}

static void move_motors(movement_t movement)
{
    stop_motors();
    const bool left_reverse = movement.direction == 2 || movement.direction == 4;
    const bool right_reverse = movement.direction == 2 || movement.direction == 3;
    const unsigned speed = movement.speed > 255 ? 255 : movement.speed;
    if (speed == 0) {
        return;
    }
    ESP_ERROR_CHECK(gpio_set_level(LEFT_DIRECTION, left_reverse));
    ESP_ERROR_CHECK(gpio_set_level(RIGHT_DIRECTION, right_reverse));
    /* Fix the original speed-1 underflow at full reverse. */
    set_duty(LEFT_CHANNEL, left_reverse ? 255 - speed : speed);
    set_duty(RIGHT_CHANNEL, right_reverse ? 255 - speed : speed);
}

static movement_t movement_for_code(uint32_t code)
{
    switch (code) {
    case 16736925U: return (movement_t){1, 255, 300}; /* Up */
    case 16754775U: return (movement_t){2, 255, 300}; /* Down */
    case 16761405U: return (movement_t){3, 150, 50};  /* Right */
    case 16720605U: return (movement_t){4, 150, 50};  /* Left */
    default:       return (movement_t){0, 0, 0};
    }
}

void app_main(void)
{
    init_motors();
    ESP_ERROR_CHECK(keyes_ir_init(GPIO_NUM_19));
    movement_t last = {0};
    int64_t last_received_ms = -1;
    int64_t stop_at_ms = -1;

    for (;;) {
        uint32_t code = 0;
        const esp_err_t error = keyes_ir_read(&code, pdMS_TO_TICKS(10));
        const int64_t now_ms = esp_timer_get_time() / 1000;
        if (stop_at_ms >= 0 && now_ms >= stop_at_ms) {
            stop_motors();
            stop_at_ms = -1;
        }
        if (error != ESP_OK) {
            if (error != ESP_ERR_TIMEOUT && error != ESP_ERR_INVALID_RESPONSE) {
                stop_motors();
                ESP_ERROR_CHECK(error);
            }
            continue;
        }
        if (code == KEYES_IR_REPEAT) {
            if (last.direction == 0 || last_received_ms < 0 ||
                now_ms - last_received_ms >= 250) {
                continue;
            }
        } else {
            last = movement_for_code(code);
        }
        if (last.direction == 0) {
            stop_motors();
            stop_at_ms = -1;
            last_received_ms = -1;
            continue;
        }
        move_motors(last);
        last_received_ms = now_ms;
        stop_at_ms = now_ms + last.duration_ms;
    }
}
