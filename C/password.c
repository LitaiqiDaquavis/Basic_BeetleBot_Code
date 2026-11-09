/* ESP-IDF C version of passowrd.cpp. GPIO19: IR; GPIO2: buzzer.
 * Build: idf.py -B build-password -DBEETLE_APP=password build
 * Four digits unlock; # clears input and locks. No servo movement is
 * triggered, matching the original program's active code.
 */
#include <stdbool.h>
#include <stdint.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "Keyes_IR_ESP32.h"

#define PASSWORD_VALUE 6767U
#define RESET_KEY      16732845U
#define BUZZER_MODE    LEDC_LOW_SPEED_MODE
#define BUZZER_TIMER   LEDC_TIMER_0
#define BUZZER_CHANNEL LEDC_CHANNEL_0

static bool s_unlocked;
static unsigned s_input;
static unsigned s_digits;
static const unsigned s_unlock_notes[] = {131, 165, 196};
static const unsigned s_lock_notes[] = {196, 165, 131};
static const unsigned *s_notes;
static unsigned s_note_index;
static int64_t s_note_deadline_ms;

static void buzzer_duty(unsigned duty)
{
    ESP_ERROR_CHECK(ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL));
}

static void init_buzzer(void)
{
    const ledc_timer_config_t timer = {
        .speed_mode = BUZZER_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = BUZZER_TIMER,
        .freq_hz = 131,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));
    const ledc_channel_config_t channel = {
        .gpio_num = GPIO_NUM_2,
        .speed_mode = BUZZER_MODE,
        .channel = BUZZER_CHANNEL,
        .timer_sel = BUZZER_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel));
}

static void start_note(int64_t now_ms)
{
    buzzer_duty(0);
    ESP_ERROR_CHECK(ledc_set_freq(BUZZER_MODE, BUZZER_TIMER,
                                 s_notes[s_note_index]));
    buzzer_duty(512);
    s_note_deadline_ms = now_ms + 120;
}

static void play_lock_sound(int64_t now_ms)
{
    s_notes = s_unlocked ? s_unlock_notes : s_lock_notes;
    s_note_index = 0;
    start_note(now_ms);
}

static void update_sound(int64_t now_ms)
{
    if (s_notes == NULL || now_ms < s_note_deadline_ms) {
        return;
    }
    if (++s_note_index == 3) {
        buzzer_duty(0);
        s_notes = NULL;
    } else {
        start_note(now_ms);
    }
}

static int digit_for_code(uint32_t code)
{
    switch (code) {
    case 16738455U: return 1;
    case 16750695U: return 2;
    case 16756815U: return 3;
    case 16724175U: return 4;
    case 16718055U: return 5;
    case 16743045U: return 6;
    case 16716015U: return 7;
    case 16726215U: return 8;
    case 16734885U: return 9;
    case 16730805U: return 0;
    default:       return -1;
    }
}

static void handle_code(uint32_t code, int64_t now_ms)
{
    if (code == KEYES_IR_REPEAT) {
        return;
    }
    if (code == RESET_KEY) {
        s_unlocked = false;
        s_input = 0;
        s_digits = 0;
        play_lock_sound(now_ms);
        return;
    }
    const int digit = digit_for_code(code);
    if (digit < 0) {
        return;
    }
    s_input = s_input * 10U + (unsigned)digit;
    if (++s_digits == 4) {
        if (s_input == PASSWORD_VALUE) {
            s_unlocked = true;
            play_lock_sound(now_ms);
        }
        /* A wrong attempt leaves the previous lock state unchanged. */
        s_input = 0;
        s_digits = 0;
    }
}

void app_main(void)
{
    init_buzzer();
    ESP_ERROR_CHECK(keyes_ir_init(GPIO_NUM_19));
    for (;;) {
        uint32_t code = 0;
        const esp_err_t error = keyes_ir_read(&code, pdMS_TO_TICKS(10));
        const int64_t now_ms = esp_timer_get_time() / 1000;
        update_sound(now_ms);
        if (error == ESP_OK) {
            handle_code(code, now_ms);
        } else if (error != ESP_ERR_TIMEOUT && error != ESP_ERR_INVALID_RESPONSE) {
            buzzer_duty(0);
            ESP_ERROR_CHECK(error);
        }
    }
}
