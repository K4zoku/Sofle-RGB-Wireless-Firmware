#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zmk/behavior.h>

bool via_qmk_basic_to_usage(uint8_t keycode, uint32_t *usage);

int zmk_via_macro_get_count(void);
size_t zmk_via_macro_get_buffer_size(void);
int zmk_via_macro_get_buffer(uint16_t offset, uint8_t size, uint8_t *data);
int zmk_via_macro_set_buffer(uint16_t offset, uint8_t size, const uint8_t *data);
int zmk_via_macro_reset(void);
int zmk_via_macro_save_changes(void);
int zmk_via_macro_enqueue(uint8_t id, const struct zmk_behavior_binding_event *event);
