#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Обробка текстового payload’у (backward compatible).
 * Сюди прилітають строки типу "garland", "garl1", "garl0", "garland_echo".
 */
void legacy_handle_text(const char *msg);

/**
 * Опціонально: чи це “сенсорне” повідомлення типу TDS/температура.
 */
bool legacy_is_sensor_value(const char *msg);

#ifdef __cplusplus
}
#endif
