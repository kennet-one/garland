#include <stdio.h>
#include <string.h>

#include "garland_node.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "keemash_mesh_node.h"

static const char *TAG = "garland_node";

/* ТВОЄ реле на GPIO35 */
#define GARLAND_RELAY_GPIO GPIO_NUM_35

/* 1 = off, 0 = on (як у тебе) */
static uint8_t garland_state = 1;

/* ---------------------- Низькорівневе керування реле ---------------------- */

static void apply_relay(void)
{
	/* як в Arduino-версії: ON = OUTPUT, OFF = INPUT */
	gpio_config_t io_conf = {
		.pin_bit_mask = 1ULL << GARLAND_RELAY_GPIO,
		.pull_up_en   = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type    = GPIO_INTR_DISABLE,
	};

	if (garland_state == 0) {
		/* ON: вихід, рівень задаємо явно (активний LOW) */
		io_conf.mode = GPIO_MODE_OUTPUT;
		gpio_config(&io_conf);
		gpio_set_level(GARLAND_RELAY_GPIO, 0);
		ESP_LOGI(TAG, "Relay ON (state=0, GPIO%d as OUTPUT)", GARLAND_RELAY_GPIO);
	} else {
		/* OFF: робимо пін INPUT (по суті high-Z) */
		io_conf.mode = GPIO_MODE_INPUT;
		gpio_config(&io_conf);
		ESP_LOGI(TAG, "Relay OFF (state=1, GPIO%d as INPUT)", GARLAND_RELAY_GPIO);
	}
}

/* ---------------------- Відправка стану в root (V2 only) ---------------------- */

void garland_node_publish_state(void)
{
	/* KeeMASH UART compatibility: обидва токени, як і раніше. */
	if (garland_state == 1) {
		(void)mesh_v2_node_send_event(0, "garland_off");
		(void)mesh_v2_node_send_event(0, "garl0");
	} else {
		(void)mesh_v2_node_send_event(0, "garland_on");
		(void)mesh_v2_node_send_event(0, "garl1");
	}
}

void garland_node_format_status(char *out, size_t out_size)
{
	if (!out || out_size == 0) return;
	snprintf(out, out_size, "%s",
		 garland_state == 1 ? "garland_off garl0" : "garland_on garl1");
}

/* ---------------------- Публічний API ---------------------- */

bool garland_node_is_on(void)
{
	return garland_state == 0;
}

void garland_node_set_on(bool on)
{
	garland_state = on ? 0 : 1;   /* нормалізуємо: 0=on, 1=off */
	apply_relay();
	garland_node_publish_state();
}

void garland_node_toggle(void)
{
	garland_node_set_on(garland_state != 0);
}

void garland_node_init(void)
{
	/* Початковий стан — OFF (як у старому коді) */
	garland_state = 1;
	apply_relay();
	garland_node_publish_state();
}
