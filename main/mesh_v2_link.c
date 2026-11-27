#include "mesh_v2_link.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "garland_node.h"
#include "keemash_mesh_hooks.h"
#include "keemash_mesh_tx_broker.h"
#include "mesh_log_stream.h"
#include "mesh_time_sync.h"

static const char *TAG = "mesh_link";
static keemash_mesh_tx_broker_t *s_tx_broker;

static bool mac_is_zero(const uint8_t mac[6]);
esp_err_t mesh_manual_reboot_schedule(uint16_t delay_ms, const char *reason);

void mesh_v2_link_log_diagnostics(void)
{
	keemash_mesh_tx_broker_stats_t tx = {0};
	keemash_mesh_tx_broker_stats(s_tx_broker, &tx);
	ESP_LOGI("mesh_tx_diag", "pending=%lu high=%lu accepted=%lu completed=%lu errors=%lu full=%lu reserved=%lu last=%ld",
	         (unsigned long)tx.pending, (unsigned long)tx.high_watermark,
	         (unsigned long)tx.accepted, (unsigned long)tx.completed,
	         (unsigned long)tx.transport_errors, (unsigned long)tx.rejected_full,
	         (unsigned long)tx.rejected_reserved, (long)tx.last_transport_err);
}

static esp_err_t raw_mesh_send(void *user, const uint8_t dst[6],
			       const void *packet, size_t packet_len)
{
	(void)user;
	if (!mesh_log_stream_transport_ready()) return ESP_ERR_INVALID_STATE;
	mesh_addr_t dest = {0};
	if (!mac_is_zero(dst)) memcpy(dest.addr, dst, 6);
	mesh_data_t data = {
		.data = (uint8_t *)packet,
		.size = packet_len,
		.proto = MESH_PROTO_BIN,
		.tos = MESH_TOS_P2P,
	};
	return esp_mesh_send(&dest, &data, MESH_DATA_P2P, NULL, 0);
}

esp_err_t mesh_v2_link_require(void)
{
	if (s_tx_broker) return ESP_OK;
	keemash_mesh_tx_broker_config_t cfg = {
		.slots = 24,
		.control_reserved_slots = 4,
		.max_packet_size = 512,
		.task_stack_words = 4096,
		.task_priority = 7,
		.task_name = "mesh_tx",
		.raw_send = raw_mesh_send,
	};
	return keemash_mesh_tx_broker_init(&s_tx_broker, &cfg);
}

static bool mac_is_zero(const uint8_t mac[6])
{
	static const uint8_t zero[6] = {0};
	return !mac || memcmp(mac, zero, sizeof(zero)) == 0;
}

esp_err_t keemash_mesh_transport_send(const uint8_t dst[6],
				      const void *packet, size_t packet_len)
{
	if (!packet || packet_len == 0) return ESP_ERR_INVALID_ARG;
	if (!s_tx_broker) return ESP_ERR_INVALID_STATE;
	return keemash_mesh_tx_broker_submit_auto(s_tx_broker, dst, packet, packet_len);
}

void keemash_mesh_get_local_mac(uint8_t mac[6])
{
	(void)esp_wifi_get_mac(WIFI_IF_STA, mac);
}

bool keemash_mesh_node_on_control_command(const char *text)
{
	/*
	 * Garland relay commands execute exclusively through the reliable
	 * control result hook. Returning false here is intentional: the
	 * reliable control layer must not cache a fake success for one-way
	 * text.
	 */
	(void)text;
	return false;
}

static void reboot_task(void *arg)
{
	typedef struct {
		uint16_t delay_ms;
		char reason[32];
	} reboot_request_t;
	reboot_request_t request = *(reboot_request_t *)arg;
	vPortFree(arg);
	ESP_LOGW(TAG, "manual reboot scheduled in %u ms: %s",
	         (unsigned)request.delay_ms, request.reason);
	vTaskDelay(pdMS_TO_TICKS(request.delay_ms));
	esp_restart();
}

esp_err_t mesh_manual_reboot_schedule(uint16_t delay_ms, const char *reason)
{
	typedef struct {
		uint16_t delay_ms;
		char reason[32];
	} reboot_request_t;
	static TaskHandle_t reboot_task_handle;
	if (reboot_task_handle) return ESP_ERR_INVALID_STATE;
	reboot_request_t *request = pvPortMalloc(sizeof(*request));
	if (!request) return ESP_ERR_NO_MEM;
	request->delay_ms = delay_ms;
	snprintf(request->reason, sizeof(request->reason), "%s",
		 reason ? reason : "manual");
	if (xTaskCreate(reboot_task, "manual_reboot", 2048, request, 5,
			&reboot_task_handle) != pdPASS) {
		vPortFree(request);
		reboot_task_handle = NULL;
		return ESP_ERR_NO_MEM;
	}
	return ESP_OK;
}

bool keemash_mesh_node_on_control_command_result(const char *text, uint8_t *status,
						 char *result, size_t result_size)
{
	if (!text) return false;
	if (strcmp(text, "system.reboot") == 0) {
		esp_err_t err = mesh_manual_reboot_schedule(1200, "reliable control");
		if (status) *status = err == ESP_OK ? MESH_V2_CONTROL_STATUS_OK
		                                    : MESH_V2_CONTROL_STATUS_FAILED;
		if (result && result_size > 0) {
			snprintf(result, result_size, "%s",
				 err == ESP_OK ? "manual reboot accepted"
				               : "manual reboot busy");
			result[result_size - 1] = '\0';
		}
		return true;
	}

	if (strcmp(text, "garland") == 0) {
		garland_node_toggle();
	} else if (strcmp(text, "garl1") == 0) {
		garland_node_set_on(true);
	} else if (strcmp(text, "garl0") == 0) {
		garland_node_set_on(false);
	} else if (strcmp(text, "garland_echo") == 0) {
		garland_node_publish_state();
	} else {
		return false;
	}

	if (status) *status = MESH_V2_CONTROL_STATUS_OK;
	if (result && result_size > 0) {
		garland_node_format_status(result, result_size);
		result[result_size - 1] = '\0';
	}
	return true;
}

void keemash_mesh_node_on_log_ctrl(bool enable)
{
	mesh_log_ctrl_packet_t control = {0};
	control.h.magic = MESH_PKT_MAGIC;
	control.h.version = MESH_PKT_VERSION;
	control.h.type = MESH_LOG_TYPE_CTRL;
	control.enable = enable ? 1 : 0;
	(void)mesh_log_stream_handle_rx(&control, sizeof(control));
}

uint32_t keemash_mesh_node_v1_ok_age_ms(void)
{
	return mesh_log_stream_tx_accepted_age_ms();
}

bool keemash_mesh_node_log_stream_enabled(void)
{
	return mesh_log_stream_enabled();
}

void keemash_mesh_node_on_time_sync(const mesh_v2_time_payload_t *time_sync)
{
	(void)mesh_time_sync_apply_v2(time_sync);
}
