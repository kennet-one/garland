#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Ініціалізація гірлянди (викликати один раз з app_main). */
void garland_node_init(void);

/* Керування реле: ON = вихід з рівнем 0, OFF = пін у high-Z (input). */
void garland_node_set_on(bool on);
void garland_node_toggle(void);
bool garland_node_is_on(void);

/* Дублюємо стан у mesh (V2 CONTROL EVENT на node0) — без legacy V1. */
void garland_node_publish_state(void);

/* Рядок стану для reliable control result: "garland_on garl1" тощо. */
void garland_node_format_status(char *out, size_t out_size);

#ifdef __cplusplus
}
#endif
