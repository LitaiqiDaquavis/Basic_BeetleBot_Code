#ifndef KEYES_IR_ESP32_C_H
#define KEYES_IR_ESP32_C_H

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

/* NEC only. Codes use the same bit order as the original Keyes library. */
#define KEYES_IR_REPEAT UINT32_MAX

/* One receiver, accessed from one task. Call init once before reading. */
esp_err_t keyes_ir_init(gpio_num_t pin);

/* ESP_OK: *code contains a full code or KEYES_IR_REPEAT.
 * ESP_ERR_TIMEOUT: no frame within wait_ticks; hardware keeps listening.
 * ESP_ERR_INVALID_RESPONSE: an invalid/non-NEC frame was discarded.
 * Other errors must be handled by the caller.
 */
esp_err_t keyes_ir_read(uint32_t *code, TickType_t wait_ticks);

#endif
