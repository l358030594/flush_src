#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".rcsp_eq_setting.data.bss")
#pragma data_seg(".rcsp_eq_setting.data")
#pragma const_seg(".rcsp_eq_setting.text.const")
#pragma code_seg(".rcsp_eq_setting.text")
#endif
#include "app_config.h"
#include "syscfg_id.h"
#include "ble_rcsp_server.h"

#include "rcsp_setting_sync.h"
#include "rcsp_setting_opt.h"

#if (RCSP_MODE && TCFG_EQ_ENABLE && RCSP_ADV_EQ_SET_ENABLE)

#if RCSP_MODE == RCSP_MODE_EARPHONE
#include "media/effects/audio_eq.h"
#include "media/effects/eq_config.h"
#else
#ifndef CONFIG_MEDIA_NEW_ENABLE
#include "media/eq_config.h"
#else
#include "effects/audio_eq.h"
#include "effects/eq_config.h"
#endif
#endif


#include "clock_manager/clock_manager.h"

extern int get_bt_tws_connect_status();

__attribute__((weak))
u8 eq_get_table_nsection(EQ_MODE mode)
{
    return EQ_SECTION_MAX;
}

static u8 g_eq_setting_info[11] = {0};

static void rcsp_eq_clock_refurbishi_in_app_core()
{
    clock_refurbish();
}

static void rcsp_eq_clock_refurbish()
{
    if (strcmp(os_current_task(), "app_core")) {
        int argv[2];
        argv[0] = (int)rcsp_eq_clock_refurbishi_in_app_core;
        argv[1] = 0;
        int ret = os_taskq_post_type("app_core", Q_CALLBACK, sizeof(argv) / sizeof(int), argv);
        if (ret) {
            log_e("taskq post err \n");
        }
    } else {
        clock_refurbish();
    }
}

static void eq_setting_info_deal(u8 *eq_info_data)
{
    u8 data;
    u8 status;
    u8 mode;
    if (eq_info_data[0] >> 7) {
        status = eq_info_data[2];
    } else {
        status = *(((u8 *)eq_info_data) + 1);
    }
    mode = eq_info_data[0] & 0x7F;
    if (mode < EQ_MODE_CUSTOM) {
        rcsp_eq_clock_refurbish();
        eq_mode_set(mode);
    } else {
        // 自定义修改EQ参数
        if (EQ_MODE_CUSTOM == mode) {
            rcsp_eq_clock_refurbish();
            if (status != 0x7F)	{
                u8 i;
                for (i = 0; i < eq_get_table_nsection(0); i++) {
                    if (eq_info_data[0] >> 7) {
                        data = eq_info_data[i + 2];
                    } else {
                        data = eq_info_data[i + 1];
                    }
                    eq_mode_set_custom_param(i, (s8)data);
                }
            }
            eq_mode_set(mode);
        }
    }
}

static void set_eq_setting(u8 *eq_setting)
{
    memcpy(g_eq_setting_info, eq_setting, 11);
}

static int get_eq_setting(u8 *eq_setting)
{
    memcpy(eq_setting, g_eq_setting_info, 11);
    return 0;
}

// 1、写入VM
static void update_eq_vm_value(u8 *eq_setting)
{
    u8 status = *(((u8 *)eq_setting) + 1);

    syscfg_write(CFG_RCSP_ADV_EQ_MODE_SETTING, eq_setting, 1);

    /*自定义修改EQ参数*/
    if (EQ_MODE_CUSTOM == (eq_setting[0] & 0x7F)) {
        if (eq_setting[0] >> 7) {
            // 多段eq
            status = eq_setting[2];
        }
        if (status != 0x7F) {
            syscfg_write(CFG_RCSP_ADV_EQ_DATA_SETTING, &eq_setting[1], 10);
        }
    }

}
// 2、同步对端
static void eq_sync(u8 *eq_setting)
{
#if TCFG_USER_TWS_ENABLE
    if (get_bt_tws_connect_status()) {
        update_rcsp_setting(ATTR_TYPE_EQ_SETTING);
    }
#endif
}

static void deal_eq_setting(u8 *eq_setting, u8 write_vm, u8 tws_sync)
{
    u8 eq_info[11] = {0};
    if (!eq_setting) {
        get_eq_setting(eq_info);
    } else {
        u8 status = *(((u8 *)eq_setting) + 1);
        /*自定义修改EQ参数*/
        if (EQ_MODE_CUSTOM == (eq_setting[0] & 0x7F)) {
            if (eq_setting[0] >> 7) {
                status = eq_setting[2];
            }

            if (status != 0x7F) {
                memcpy(eq_info, eq_setting, 11);
                set_eq_setting(eq_info);
            } else {
                g_eq_setting_info[0] = eq_setting[0];
                get_eq_setting(eq_info);
            }
        } else {
            memcpy(eq_info, eq_setting, 11);
            g_eq_setting_info[0] = eq_setting[0];
        }

    }
    if (write_vm) {
        update_eq_vm_value(eq_info);
    }
    if (tws_sync) {
        eq_sync(eq_info);
    }
    eq_setting_info_deal(eq_info);
}


static u8 app_get_eq_info(u8 *get_eq_info)
{
    u16 i;
    get_eq_info[0] = eq_mode_get_cur();
    u8 data_len = 1;

    if (10 == eq_get_table_nsection(0)) {
        // 10段eq : mode + gain[10byte]
        for (i = 0; i < 10; i++) {
            get_eq_info[data_len] = eq_mode_get_gain(get_eq_info[0], i);
            data_len++;
        }
    } else {
        // 多段eq : mode + num + value(freq[2byte] + gain[1byte])
        get_eq_info[1] = eq_get_table_nsection(0);
        data_len++;
        for (i = 0; i < eq_get_table_nsection(0); i++) {
            get_eq_info[data_len] = eq_mode_get_gain(get_eq_info[0], i);
            data_len++;
        }
        get_eq_info[0] |= (1 << 7);
    }
    return data_len;
}

static u8 app_get_eq_all_info(u8 *get_eq_info)
{
    u16 i;
    u8 mode = EQ_MODE_NORMAL;
    get_eq_info[0] = eq_get_table_nsection(0);
    u8 data_len = 1;
    // get freq
    for (i = 0; i < eq_get_table_nsection(0); i++) {
        u16 eq_freq = (u16)(eq_mode_get_freq(mode, i) & ((u16) - 1));
        eq_freq = ((((u8 *)&eq_freq)[0] << 8)  + ((u8 *)&eq_freq)[1]);
        memcpy(get_eq_info + data_len, &eq_freq, sizeof(eq_freq));
        data_len += 2;
    }
    // get gain
    for (; mode < EQ_MODE_CUSTOM; mode++) {
        get_eq_info[data_len] = mode;
        if (10 != eq_get_table_nsection(0)) {
            get_eq_info[data_len] |= (1 << 7);
        }
        data_len++;
        for (i = 0; i < eq_get_table_nsection(0); i++) {
            u8 eq_gain = eq_mode_get_gain(mode, i);
            get_eq_info[data_len] = eq_gain;
            data_len++;
        }
    }
    // get custom
    get_eq_info[data_len] = EQ_MODE_CUSTOM;
    if (10 != eq_get_table_nsection(0)) {
        get_eq_info[data_len] |= (1 << 7);
    }
    data_len++;
    for (i = 0; i < eq_get_table_nsection(0); i++) {
        if (10 == eq_get_table_nsection(0)) {
            get_eq_info[data_len] = g_eq_setting_info[i + 1];
        } else {
            get_eq_info[data_len] = g_eq_setting_info[i + 2];
        }
        data_len++;
    }

    return data_len;
}

static int eq_opt_init(void)
{
    u8 eq_setting_vm_info[11] = {0};
    u8 eq_setting_info[10] = {0};
    u8 eq_setting_mode = 0;
    u8 i;
    rcsp_read_data_from_vm(CFG_RCSP_ADV_EQ_DATA_SETTING, eq_setting_info, sizeof(eq_setting_info));
    if (rcsp_read_data_from_vm(CFG_RCSP_ADV_EQ_MODE_SETTING, &eq_setting_mode, sizeof(eq_setting_mode))) {
        eq_setting_vm_info[0] = eq_setting_mode;
        memcpy(&eq_setting_vm_info[1], eq_setting_info, 10);
        set_eq_setting(eq_setting_vm_info);
        deal_eq_setting(NULL, 0, 0);
    }
    return 0;
}

static int eq_get_setting_extra_handle(void *setting_data, void *setting_data_len)
{
    u16 data_len = *((u16 *) setting_data_len);
    if (data_len > sizeof(g_eq_setting_info)) {
        data_len = app_get_eq_all_info(setting_data);
    } else {
        data_len = app_get_eq_info(setting_data);
    }
    return data_len;
}

static RCSP_SETTING_OPT eq_opt = {
    .data_len = 11,
    .setting_type = ATTR_TYPE_EQ_SETTING,
    .syscfg_id = CFG_RCSP_ADV_EQ_DATA_SETTING,
    .deal_opt_setting = deal_eq_setting,
    .set_setting = set_eq_setting,
    .get_setting = get_eq_setting,
    .custom_setting_init = eq_opt_init,
    .custom_vm_info_update = NULL,
    .custom_setting_update = NULL,
    .custom_sibling_setting_deal = NULL,
    .custom_setting_release = NULL,
    .set_setting_extra_handle = NULL,
    .get_setting_extra_handle = eq_get_setting_extra_handle,
};
REGISTER_APP_SETTING_OPT(eq_opt);

#endif
