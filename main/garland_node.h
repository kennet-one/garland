#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Ініціалізація гірлянди (викликати один раз з app_main)
void garland_node_init(void);

// Обробка текстових команд у старому форматі ("garland", "garl1"...)
void garland_node_legacy_cmd(const char *txt);

#ifdef __cplusplus
}
#endif
