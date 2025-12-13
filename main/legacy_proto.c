#include "legacy_proto.h"
#include "garland_node.h"
#include "esp_log.h"

static const char *TAG = "legacy";

void legacy_handle_text(const char *txt)
{
	ESP_LOGI(TAG, "legacy RX: \"%s\"", txt);
	garland_node_legacy_cmd(txt);
}
