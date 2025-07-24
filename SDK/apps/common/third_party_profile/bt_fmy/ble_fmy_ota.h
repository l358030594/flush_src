#ifndef __BLE_FMY_OTA_H__
#define __BLE_FMY_OTA_H__

#include "typedef.h"
#include "btstack/third_party/fmna/fmna_api.h"

#define FMY_OTA_SUPPORT_CONFIG   (0)//(CONFIG_APP_OTA_ENABLE && CONFIG_APP_OTA_ENABLE && CONFIG_DB_UPDATE_DATA_GENERATE_EN)

enum {
    FMY_OTA_STATE_IDLE = 0,
    FMY_OTA_STATE_REQ,
    FMY_OTA_STATE_CHECK_FILESIZE,
    FMY_OTA_STATE_WRITE_DATA,
    FMY_OTA_STATE_COMPLETE,
    FMY_OTA_STATE_WRITE_ERROR,
};

int fmy_ota_process(uarp_cmd_type_t cmd_type, u8 *recv_data, u32 recv_len);
u8 fmy_ota_get_state(void);

#endif
