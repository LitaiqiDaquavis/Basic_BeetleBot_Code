#include "Keyes_IR_ESP32.h"

#include <stdbool.h>
#include <stddef.h>
#include "driver/rmt_rx.h"
#include "freertos/queue.h"

static rmt_channel_handle_t s_channel;
static QueueHandle_t s_events;
static rmt_symbol_word_t s_symbols[64];
static const rmt_receive_config_t s_receive_config = {
    .signal_range_min_ns = 1250,
    .signal_range_max_ns = 12000000,
};

static bool duration_matches(uint32_t measured, uint32_t expected)
{
    return measured >= expected - 300 && measured <= expected + 300;
}

static bool pair_matches(const rmt_symbol_word_t *symbol,
                         uint32_t low_us, uint32_t high_us)
{
    return symbol->level0 == 0 && symbol->level1 == 1 &&
           duration_matches(symbol->duration0, low_us) &&
           duration_matches(symbol->duration1, high_us);
}

static bool footer_matches(const rmt_symbol_word_t *symbol)
{
    /* RMT's final high duration may be zero or the idle timeout. */
    return symbol->level0 == 0 && duration_matches(symbol->duration0, 560);
}

static bool decode_nec(const rmt_symbol_word_t *symbols, size_t count,
                       uint32_t *code)
{
    if (count == 2 && pair_matches(&symbols[0], 9000, 2250) &&
        footer_matches(&symbols[1])) {
        *code = KEYES_IR_REPEAT;
        return true;
    }
    if (count != 34 || !pair_matches(&symbols[0], 9000, 4500) ||
        !footer_matches(&symbols[33])) {
        return false;
    }

    uint32_t value = 0;
    for (size_t i = 1; i <= 32; ++i) {
        value <<= 1;
        if (pair_matches(&symbols[i], 560, 1690)) {
            value |= 1U;
        } else if (!pair_matches(&symbols[i], 560, 560)) {
            return false;
        }
    }
    /* Ordinary NEC address/command inverse-byte checks for this remote. */
    if ((((value >> 24) ^ (value >> 16)) & 0xFFU) != 0xFFU ||
        (((value >> 8) ^ value) & 0xFFU) != 0xFFU) {
        return false;
    }
    *code = value;
    return true;
}

static bool on_receive_done(rmt_channel_handle_t channel,
                            const rmt_rx_done_event_data_t *event,
                            void *context)
{
    (void)channel;
    BaseType_t wake = pdFALSE;
    /* One capture is in flight. Do not reuse its buffer until decoded. */
    xQueueSendFromISR((QueueHandle_t)context, event, &wake);
    return wake == pdTRUE;
}

esp_err_t keyes_ir_init(gpio_num_t pin)
{
    if (s_channel != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    s_events = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    if (s_events == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const rmt_rx_channel_config_t config = {
        .gpio_num = pin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 1000000,
        .mem_block_symbols = 64,
    };
    bool enabled = false;
    esp_err_t error = rmt_new_rx_channel(&config, &s_channel);
    if (error != ESP_OK) {
        goto fail;
    }
    error = gpio_set_pull_mode(pin, GPIO_PULLUP_ONLY);
    if (error != ESP_OK) {
        goto fail;
    }
    const rmt_rx_event_callbacks_t callbacks = {
        .on_recv_done = on_receive_done,
    };
    error = rmt_rx_register_event_callbacks(s_channel, &callbacks, s_events);
    if (error != ESP_OK) {
        goto fail;
    }
    error = rmt_enable(s_channel);
    if (error != ESP_OK) {
        goto fail;
    }
    enabled = true;
    error = rmt_receive(s_channel, s_symbols, sizeof(s_symbols),
                        &s_receive_config);
    if (error == ESP_OK) {
        return ESP_OK;
    }

fail:
    if (enabled) {
        ESP_ERROR_CHECK(rmt_disable(s_channel));
    }
    if (s_channel != NULL) {
        ESP_ERROR_CHECK(rmt_del_channel(s_channel));
        s_channel = NULL;
    }
    vQueueDelete(s_events);
    s_events = NULL;
    return error;
}

esp_err_t keyes_ir_read(uint32_t *code, TickType_t wait_ticks)
{
    if (code == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_channel == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    rmt_rx_done_event_data_t event;
    if (xQueueReceive(s_events, &event, wait_ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    uint32_t value = 0;
    const bool valid = decode_nec(event.received_symbols, event.num_symbols,
                                   &value);
    /* Rearm after decoding, including malformed frames. */
    esp_err_t error = rmt_receive(s_channel, s_symbols, sizeof(s_symbols),
                                 &s_receive_config);
    if (error != ESP_OK) {
        return error;
    }
    if (!valid) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    *code = value;
    return ESP_OK;
}
