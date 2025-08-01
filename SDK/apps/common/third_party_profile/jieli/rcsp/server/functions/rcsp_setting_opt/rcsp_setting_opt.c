#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_setting_opt.data.bss")
#pragma data_seg(".rcsp_setting_opt.data")
#pragma const_seg(".rcsp_setting_opt.text.const")
#pragma code_seg(".rcsp_setting_opt.text")
#endif
#include "rcsp_config.h"
#include "rcsp_setting_opt.h"
#include "rcsp_setting_sync.h"
#include "rcsp_manage.h"
#include "btstack/avctp_user.h"
#include "bt_tws.h"
#include "rcsp_bt_manage.h"


#if (RCSP_MODE)

static RCSP_SETTING_OPT *g_opt_link_head = NULL;
static u32 g_opt_total_size = 0;

int register_rcsp_setting_opt_setting(void *opt_param)
{
    RCSP_SETTING_OPT *opt = g_opt_link_head;
    RCSP_SETTING_OPT *item = (RCSP_SETTING_OPT *)opt_param;
    while (opt && item) {
        if (opt->setting_type == item->setting_type) {
            return 0;
        }
        opt = opt->next;
    }

    if (item) {
        item->next = g_opt_link_head;
        g_opt_link_head = item;
        return 0;
    }

    return -1;
}

static u8 deal_setting_string_item(u8 *des, u8 *src, u8 src_len, u8 type)
{
    u8 offset = 0;
    if (src_len) {
        des[offset++] = type;
        memcpy(des + offset, src, src_len);
        offset += src_len;
    }
    return offset;
}

u8 rcsp_read_data_from_vm(u8 syscfg_id, u8 *buf, u8 buf_len)
{
    /* printf("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__); */
    int len = 0;

    len = syscfg_read(syscfg_id, buf, buf_len);
    /* if (buf) { */
    /*     put_buf(buf, buf_len); */
    /* } */

    if (len > 0) {
        for (int i = 0; i < buf_len; i++) {
            if (buf[i] != 0xff) {
                return (buf_len == len);
            }
        }
    }

    return 0;
}

void rcsp_setting_init(void)
{
    RCSP_SETTING_OPT *opt = g_opt_link_head;
    u32 opt_total_size = 0;
    while (opt) {
        u8 *data = NULL;
        if (opt->data_len) {
            data = zalloc(opt->data_len);
        }
        if (opt->custom_setting_init) {
            opt->custom_setting_init();
        } else if (rcsp_read_data_from_vm(opt->syscfg_id, data, opt->data_len)) {
            if (opt->set_setting && opt->deal_opt_setting) {
                opt->set_setting(data);
                opt->deal_opt_setting(NULL, 0, 0);
            }
        }

        if (data) {
            free(data);
        }
        if (opt->data_len) {
            opt_total_size += (opt->data_len + 1);
        }
        opt = opt->next;
    }

    if (opt_total_size) {
        g_opt_total_size = opt_total_size + 2;
    }
}

void update_rcsp_setting(u8 type)
{
    u16 offset = sizeof(u16);
    u8 *setting_to_sync = NULL;
    if (g_opt_total_size) {
        setting_to_sync = zalloc(g_opt_total_size);
    } else {
        return;
    }
    RCSP_SETTING_OPT *opt = g_opt_link_head;

    while (opt) {
        if (((u8) - 1 == type) || (type == opt->setting_type)) {
            u8 *data = NULL;
            if (opt->data_len) {
                data = zalloc(opt->data_len);
            }
            if (opt->custom_setting_update)	{
                opt->custom_setting_update(data);
            } else if (opt->get_setting && (opt->data_len > 0)) {
                opt->get_setting(data);
            }
            offset += deal_setting_string_item(setting_to_sync + offset, data, opt->data_len, opt->setting_type);
            if (data) {
                free(data);
            }
        }
        opt = opt->next;
    }

    if (offset > sizeof(u16)) {
        memcpy(setting_to_sync, &offset, sizeof(offset));
        if (((u8) - 1) == type) {
            tws_api_send_data_to_sibling(setting_to_sync, offset, TWS_FUNC_ID_ADV_SETTING_SYNC);
            tws_api_sync_call_by_uuid(TWS_FUNC_APP_OPT_UUID, APP_OPT_SYNC_CMD_APP_RESET_LED_UI, 300);
        } else {
            if (tws_api_get_role() == TWS_ROLE_MASTER) {
                tws_api_send_data_to_sibling(setting_to_sync, offset, TWS_FUNC_ID_ADV_SETTING_SYNC);
                tws_api_sync_call_by_uuid(TWS_FUNC_APP_OPT_UUID, APP_OPT_SYNC_CMD_APP_RESET_LED_UI, 300);
            }
        }
    }

    if (setting_to_sync) {
        free(setting_to_sync);
    }
}

void deal_sibling_setting(u8 *buf)
{
    u8 type;
    u16 len = 0;
    memcpy(&len, buf, sizeof(len));
    u16 offset = sizeof(len);
    u8 *data;
    while (offset < len) {
        type = buf[offset++];
        data = buf + offset;
        RCSP_SETTING_OPT *opt = g_opt_link_head;
        while (opt) {
            if (type == opt->setting_type) {
                if (opt->custom_sibling_setting_deal) {
                    opt->custom_sibling_setting_deal(data);
                } else if (opt->set_setting) {
                    opt->set_setting(data);
                }
                offset += opt->data_len;
                opt->need_opt = true;
                break;
            }
            opt = opt->next;
        }
    }
    // 发送事件
    JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_ADV_SETTING_UPDATE, NULL, 0);
}

void update_info_from_vm_info(void)
{
    RCSP_SETTING_OPT *opt = g_opt_link_head;
    while (opt) {
        if (opt->need_opt) {
            if (opt->custom_vm_info_update) {
                opt->custom_vm_info_update();
            } else if (opt->deal_opt_setting) {
                opt->deal_opt_setting(NULL, 1, 0);
            }
            opt->need_opt = false;
        }
        opt = opt->next;
    }
}

RCSP_SETTING_OPT *get_rcsp_setting_opt_hdl(u16 setting_type)
{
    RCSP_SETTING_OPT *opt = g_opt_link_head;
    while (opt) {
        if (opt->setting_type == setting_type) {
            return opt;
        }
        opt = opt->next;
    }
    return NULL;
}

void set_rcsp_opt_setting(RCSP_SETTING_OPT *setting_opt_hdl, u8 *data)
{
    if (setting_opt_hdl && setting_opt_hdl->deal_opt_setting) {
        setting_opt_hdl->deal_opt_setting(data, 1, 1);
    }
}

int get_rcsp_opt_setting(RCSP_SETTING_OPT *setting_opt_hdl, u8 *data)
{
    if (setting_opt_hdl && setting_opt_hdl->get_setting) {
        return setting_opt_hdl->get_setting(data);
    }
    return 0;
}

int set_setting_extra_handle(RCSP_SETTING_OPT *setting_opt_hdl, void *data, void *data_len)
{
    if (setting_opt_hdl && setting_opt_hdl->set_setting_extra_handle) {
        return setting_opt_hdl->set_setting_extra_handle(data, data_len);
    }
    return -1;
}

int get_setting_extra_handle(RCSP_SETTING_OPT *setting_opt_hdl, void *setting_data, void *setting_data_len)
{
    if (setting_opt_hdl && setting_opt_hdl->get_setting_extra_handle) {
        return setting_opt_hdl->get_setting_extra_handle(setting_data, setting_data_len);
    }
    return 0;
}

void rcsp_opt_release(void)
{
    RCSP_SETTING_OPT *opt = g_opt_link_head;
    while (opt) {
        if (opt->custom_setting_release) {
            opt->custom_setting_release();
        }
        opt = opt->next;
    }
}

#if RCSP_ADV_EN
void modify_bt_name_and_reset(u32 msec)
{
    tws_api_sync_call_by_uuid(TWS_FUNC_ID_ADV_RESET_SYNC, 0, msec);
}
#endif

#endif
