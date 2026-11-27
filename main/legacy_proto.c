#include "legacy_proto.h"

#include <string.h>

#include "esp_log.h"

static const char *TAG = "legacy";

static bool starts_with(const char *text, const char *prefix)
{
	return text && prefix && strncmp(text, prefix, strlen(prefix)) == 0;
}

bool legacy_is_sensor_value(const char *message)
{
	return starts_with(message, "TDSB") || starts_with(message, "TDS") ||
	       starts_with(message, "ttds");
}

void legacy_handle_text(const char *message)
{
	if (!message || !message[0]) return;
	/*
	 * Garland actuator commands execute exclusively through the reliable
	 * control result hook (keemash_mesh_node_on_control_command_result).
	 * Warn-and-ignore here is intentional: the one-way legacy TEXT path
	 * must not double-execute relay commands.
	 */
	ESP_LOGW(TAG, "unsupported legacy text: \"%s\"", message);
}
