#include <string.h>
#include "garland_node.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "legacy_root_sender.h"   // для legacy_send_to_root()

static const char *TAG = "garland_node";

// ТВОЄ реле на GPIO35
#define GARLAND_RELAY_GPIO   GPIO_NUM_35

// 1 = off, 0 = on (як у тебе)
static uint8_t garland_state = 1;

// ---------------------- Низькорівневе керування реле ----------------------

// static void apply_relay(void)
// {
// 	static bool inited = false;

// 	if (!inited) {
// 		inited = true;

// 		gpio_config_t io_conf = {
// 			.pin_bit_mask = 1ULL << GARLAND_RELAY_GPIO,
// 			.mode = GPIO_MODE_OUTPUT,
// 			.pull_up_en = GPIO_PULLUP_DISABLE,
// 			.pull_down_en = GPIO_PULLDOWN_DISABLE,
// 			.intr_type = GPIO_INTR_DISABLE,
// 		};
// 		gpio_config(&io_conf);
// 	}

// 	// garland_state: 0=on, 1=off
// 	if (garland_state == 0) {
// 		gpio_set_level(GARLAND_RELAY_GPIO, 0); // ON
// 		ESP_LOGI(TAG, "Relay ON (GPIO%d=0)", GARLAND_RELAY_GPIO);
// 	} else {
// 		gpio_set_level(GARLAND_RELAY_GPIO, 1); // OFF
// 		ESP_LOGI(TAG, "Relay OFF (GPIO%d=1)", GARLAND_RELAY_GPIO);
// 	}
// }
// static uint8_t s_state = 0;

// static void apply_relay(void)
// {
// 	gpio_set_level(GARLAND_RELAY_GPIO, s_state ? 1 : 0);
// 	ESP_LOGI(TAG, "GPIO%d=%d (state=%u)", (int)GARLAND_RELAY_GPIO, s_state ? 1 : 0, (unsigned)s_state);
// }

static void apply_relay(void)
{
	// як в Arduino-версії: ON = OUTPUT, OFF = INPUT
	gpio_config_t io_conf = {
		.pin_bit_mask = 1ULL << GARLAND_RELAY_GPIO,
		.pull_up_en   = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type    = GPIO_INTR_DISABLE,
	};

	if (garland_state == 0) {
		// ON: вихід, рівень задаємо явно (0 або 1 – підстрой під своє реле)
		io_conf.mode = GPIO_MODE_OUTPUT;
		gpio_config(&io_conf);
		gpio_set_level(GARLAND_RELAY_GPIO, 0);  // якщо треба активний LOW
		ESP_LOGI(TAG, "Relay ON (state=0, GPIO%d as OUTPUT)", GARLAND_RELAY_GPIO);
	} else {
		// OFF: як у тебе – робимо пін INPUT (по суті high-Z)
		io_conf.mode = GPIO_MODE_INPUT;
		gpio_config(&io_conf);
		ESP_LOGI(TAG, "Relay OFF (state=1, GPIO%d as INPUT)", GARLAND_RELAY_GPIO);
	}
}

// ---------------------- Відправка стану в root ----------------------

static void publish_state(void)
{
	if (garland_state == 1) {
		legacy_send_to_root("garland_off");
		legacy_send_to_root("garl0");
	} else {
		legacy_send_to_root("garland_on");
		legacy_send_to_root("garl1");
	}
}

// ---------------------- Публічний API ----------------------

void garland_node_init(void)
{
	// Можна зробити початковий стан (як у старому коді)
	garland_state = 1;  // OFF
	apply_relay();
	publish_state();
}

static void garland_set_state(uint8_t new_state)
{
	garland_state = new_state ? 1 : 0;   // нормалізуємо 0/1
	apply_relay();
	publish_state();
}

void garland_node_legacy_cmd(const char *txt)
{
	if (!txt) return;

	ESP_LOGI(TAG, "legacy cmd: \"%s\"", txt);

	if (strcmp(txt, "garland") == 0) {
		// тумблер
		garland_set_state(garland_state ^ 1);
		return;
	}
	if (strcmp(txt, "garland_echo") == 0) {
		// просто віддати поточний стан
		publish_state();
		return;
	}
	if (strcmp(txt, "garl1") == 0) {
		// за твоєю логікою: garl1 = ON => state=0
		garland_set_state(0);
		return;
	}
	if (strcmp(txt, "garl0") == 0) {
		// garl0 = OFF => state=1
		garland_set_state(1);
		return;
	}

	// якщо прилетіло щось інше – просто залогуємо
	ESP_LOGW(TAG, "unknown garland cmd: \"%s\"", txt);
}
