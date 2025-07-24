#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tone_table.data.bss")
#pragma data_seg(".tone_table.data")
#pragma const_seg(".tone_table.text.const")
#pragma code_seg(".tone_table.text")
#endif
#include "app_config.h"
#include "app_tone.h"
#include "fs/resfile.h"

static const struct tone_files chinese_tone_files = {
    .num = {
        "tone_zh/0.*",
        "tone_zh/1.*",
        "tone_zh/2.*",
        "tone_zh/3.*",
        "tone_zh/4.*",
        "tone_zh/5.*",
        "tone_zh/6.*",
        "tone_zh/7.*",
        "tone_zh/8.*",
        "tone_zh/9.*",
    },
    .power_on           = "tone_zh/power_on.*",
    .power_off          = "tone_zh/power_off.*",
    .bt_mode            = "tone_zh/bt.*",
    .bt_connect         = "tone_zh/bt_conn.*",
    .bt_disconnect      = "tone_zh/bt_dconn.*",
    .phone_in           = "tone_zh/ring.*",
    .phone_out          = "tone_zh/ring.*",
    .low_power          = "tone_zh/low_power.*",
    .max_vol            = "tone_zh/vol_max.*",
    .tws_connect        = "tone_zh/tws_conn.*",
    .tws_disconnect     = "tone_zh/tws_dconn.*",
    .normal             = "tone_zh/normal.*",
    .low_latency_in     = "tone_zh/game_in.*",
    .low_latency_out    = "tone_zh/game_out.*",
    .anc_on    			= "tone_zh/anc_on.*",
    .anc_trans    		= "tone_zh/anc_trans.*",
    .anc_off    		= "tone_zh/anc_off.*",
    .key_tone  		    = "tone_zh/key_tone.*",
    .anc_adaptive       = "tone_zh/adaptive.*",
    .anc_adaptive_coeff = "tone_zh/anc_on.*",
    .anc_normal_coeff   = "tone_zh/anc_on.*",
    .spkchat_on         = "tone_zh/spkchat_on.*",
    .spkchat_off        = "tone_zh/spkchat_off.*",
    .winddet_on         = "tone_zh/winddet_on.*",
    .winddet_off        = "tone_zh/winddet_off.*",
    .wclick_on          = "tone_zh/wclick_on.*",
    .wclick_off         = "tone_zh/wclick_off.*",
    .linein_mode        = "tone_zh/linein.*",
    .pc_mode         	= "tone_zh/pc.*",
    .music_mode         = "tone_zh/music.*",
    .device_sd          = "tone_zh/sd.*",
    .device_udisk       = "tone_zh/usb.*",
    .fit_det_on         = "tone_zh/fit_det_on.*",
    .share_search_pairing = "tone_zh/sharesearc.*",
    .share_wait_pairing = "tone_zh/sharewait.*",
    .share_conn_master = "tone_zh/sharemaster.*",
    .share_conn_slave = "tone_zh/shareslaver.*",
    .share_disconnect = "tone_zh/share_dconn.*",
};

static const struct tone_files english_tone_files = {
    .num = {
        "tone_en/0.*",
        "tone_en/1.*",
        "tone_en/2.*",
        "tone_en/3.*",
        "tone_en/4.*",
        "tone_en/5.*",
        "tone_en/6.*",
        "tone_en/7.*",
        "tone_en/8.*",
        "tone_en/9.*",
    },
    .power_on           = "tone_en/power_on.*",
    .power_off          = "tone_en/power_off.*",
    .bt_mode            = "tone_en/bt.*",
    .bt_connect         = "tone_en/bt_conn.*",
    .bt_disconnect      = "tone_en/bt_dconn.*",
    .phone_in           = "tone_en/ring.*",
    .phone_out          = "tone_en/ring.*",
    .low_power          = "tone_en/low_power.*",
    .max_vol            = "tone_en/vol_max.*",
    .tws_connect        = "tone_en/tws_conn.*",
    .tws_disconnect     = "tone_en/tws_dconn.*",
    .normal             = "tone_en/normal.*",
    .low_latency_in     = "tone_en/game_in.*",
    .low_latency_out    = "tone_en/game_out.*",
    .anc_on    			= "tone_en/anc_on.*",
    .anc_trans    		= "tone_en/anc_trans.*",
    .anc_off    		= "tone_en/anc_off.*",
    .key_tone  		    = "tone_en/key_tone.*",
    .anc_adaptive       = "tone_en/adaptive.*",
    .anc_adaptive_coeff = "tone_en/anc_on.*",
    .anc_normal_coeff   = "tone_en/anc_on.*",
    .spkchat_on         = "tone_en/spkchat_on.*",
    .spkchat_off        = "tone_en/spkchat_off.*",
    .winddet_on         = "tone_en/winddet_on.*",
    .winddet_off        = "tone_en/winddet_off.*",
    .wclick_on          = "tone_en/wclick_on.*",
    .wclick_off         = "tone_en/wclick_off.*",
    .linein_mode        = "tone_en/linein.*",
    .pc_mode         	= "tone_en/pc.*",
    .music_mode         = "tone_en/music.*",
    .device_sd          = "tone_en/sd.*",
    .device_udisk       = "tone_en/usb.*",
    .fit_det_on         = "tone_en/fit_det_on.*",
    .share_search_pairing = "tone_en/sharesearc.*",
    .share_wait_pairing = "tone_en/sharewait.*",
    .share_conn_master = "tone_en/sharemaster.*",
    .share_conn_slave = "tone_en/shareslaver.*",
    .share_disconnect = "tone_en/share_dconn.*",
};

#if TCFG_TONE_EN_ENABLE
static enum tone_language g_lang_used = TONE_ENGLISH;
#else
static enum tone_language g_lang_used = TONE_CHINESE;
#endif

enum tone_language tone_language_get()
{
    return g_lang_used;
}

void tone_language_set(enum tone_language lang)
{
    g_lang_used = lang;
}

const struct tone_files *get_tone_files()
{
#if TCFG_TONE_ZH_ENABLE
    if (g_lang_used == TONE_CHINESE) {
        return &chinese_tone_files;
    }
#endif

    return &english_tone_files;
}

