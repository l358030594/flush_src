#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".bt_profile_config.data.bss")
#pragma data_seg(".bt_profile_config.data")
#pragma const_seg(".bt_profile_config.text.const")
#pragma code_seg(".bt_profile_config.text")
#endif
#include "system/includes.h"
#include "app_config.h"
#include "btcontroller_config.h"
#include "btstack/btstack_typedef.h"
#include "bt_common.h"
#include "sdk_config.h"
#include "btstack/avctp_user.h"

#define LOG_TAG     "[BT-CFG]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#include "debug.h"


#if TCFG_APP_BT_EN

typedef struct {
    // linked list - assert: first field
    void *offset_item;

    // data is contained in same memory
    u32        service_record_handle;
    u8         *service_record;
} service_record_item_t;

extern const u8 sdp_pnp_service_data[];
extern const u8 sdp_a2dp_service_data[];
extern const u8 sdp_avctp_ct_service_data[];
extern const u8 sdp_avctp_ta_service_data[];
extern const u8 sdp_hfp_service_data[];
extern const u8 sdp_spp_service_data[];
extern const u8 sdp_hid_service_data[];
extern service_record_item_t  sdp_record_item_begin[];
extern service_record_item_t  sdp_record_item_end[];

#define SDP_RECORD_HANDLER_REGISTER(handler) \
	const service_record_item_t  handler \
		sec(.sdp_record_item)

#if TCFG_USER_BLE_ENABLE

#if TCFG_BLE_AUDIO_TEST_EN
const int config_stack_modules = BT_BTSTACK_LE;
#else /* TCFG_BLE_AUDIO_TEST_EN */

#if ((THIRD_PARTY_PROTOCOLS_SEL==0)&&(TCFG_LE_AUDIO_APP_CONFIG==0)&&TCFG_USER_BLE_ENABLE)
const int config_stack_modules = (BT_BTSTACK_CLASSIC | BT_BTSTACK_LE_ADV);
#else
const int config_stack_modules = BT_BTSTACK_CLASSIC | BT_BTSTACK_LE;
#endif

#endif /* TCFG_BLE_AUDIO_TEST_EN */

#else
const int config_stack_modules = BT_BTSTACK_CLASSIC;
#endif

#if (TCFG_BT_SUPPORT_PNP==1)
SDP_RECORD_HANDLER_REGISTER(pnp_sdp_record_item) = {
    .service_record = (u8 *)sdp_pnp_service_data,
    .service_record_handle = 0x1000A,
};
#endif

#if (TCFG_BT_SUPPORT_A2DP==1)
u8 a2dp_profile_support = 1;
SDP_RECORD_HANDLER_REGISTER(a2dp_sdp_record_item) = {
    .service_record = (u8 *)sdp_a2dp_service_data,
    .service_record_handle =  0x00010001,
};
#endif
#if (TCFG_BT_SUPPORT_AVCTP==1)
u8 acp_profile_support = 1;
SDP_RECORD_HANDLER_REGISTER(arp_ct_sdp_record_item) = {
    .service_record = (u8 *)sdp_avctp_ct_service_data,
    .service_record_handle = 0x00010002,
};
#if TCFG_BT_VOL_SYNC_ENABLE
SDP_RECORD_HANDLER_REGISTER(arp_ta_sdp_record_item) = {
    .service_record = (u8 *)sdp_avctp_ta_service_data,
    .service_record_handle = 0x00010005,
};
#endif
#endif
#if (TCFG_BT_SUPPORT_HFP==1)
u8 hfp_profile_support = 1;
SDP_RECORD_HANDLER_REGISTER(hfp_sdp_record_item) = {
    .service_record = (u8 *)sdp_hfp_service_data,
    .service_record_handle = 0x00010003,
};
#endif
#if (TCFG_BT_SUPPORT_SPP==1)

u8 spp_up_profile_support = 1;
//FE010000-1234-5678-ABCD-00805F9B34FB
/* const u8 sdp_user_spp_service_data[96] = { */
/* 0x36, 0x00, 0x5B, 0x09, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00, 0x11, 0x09, 0x00, 0x01, 0x36, 0x00, */
/* 0x11, 0x1C, 0xfe, 0x01, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0x00, 0x80, 0x5F, 0x9b, */
/* 0x34, 0xfb, 0x09, 0x00, 0x04, 0x36, 0x00, 0x0E, 0x36, 0x00, 0x03, 0x19, 0x01, 0x00, 0x36, 0x00, */
/* 0x05, 0x19, 0x00, 0x03, 0x08, 0x0a, 0x09, 0x00, 0x09, 0x36, 0x00, 0x17, 0x36, 0x00, 0x14, 0x1C, */
/* 0xfe, 0x01, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78, 0xab, 0xcd, 0x00, 0x80, 0x5F, 0x9b, 0x34, 0xfb, */
/* 0x09, 0x01, 0x00, 0x09, 0x01, 0x00, 0x25, 0x06, 0x4A, 0x4C, 0x5F, 0x53, 0x50, 0x50, 0x00, 0x00, */
/* }; */
/* SDP_RECORD_HANDLER_REGISTER(spp_user_sdp_record_item) = { */
/* .service_record = (u8 *)sdp_user_spp_service_data, */
/* .service_record_handle = 0x00010011, */
/* }; */

u8 spp_profile_support = 1;
/* SDP_RECORD_HANDLER_REGISTER(spp_sdp_record_item) = { */
/* .service_record = (u8 *)sdp_spp_service_data, */
/* .service_record_handle = 0x00010004, */
/* }; */

#endif
#if (TCFG_BT_SUPPORT_HID==1)
u8 hid_profile_support = 1;
SDP_RECORD_HANDLER_REGISTER(hid_sdp_record_item) = {
    .service_record = (u8 *)sdp_hid_service_data,
    .service_record_handle = 0x00010006,
};
#endif
#if (TCFG_BT_SUPPORT_PBAP==1)
extern const u8 sdp_pbap_service_data[];
u8 pbap_profile_support = 1;
SDP_RECORD_HANDLER_REGISTER(pbap_sdp_record_item) = {
    .service_record = (u8 *)sdp_pbap_service_data,
    .service_record_handle = 0x00010007,
};
#endif
#if (TCFG_BT_SUPPORT_MAP==1)
extern const u8 sdp_map_mce_service_data[];
u8 map_profile_support = 1;
SDP_RECORD_HANDLER_REGISTER(map_sdp_record_item) = {
    .service_record = (u8 *)sdp_map_mce_service_data,
    .service_record_handle = 0x00010009,
};
#endif

#if (defined TCFG_BT_SUPPORT_PAN && (TCFG_BT_SUPPORT_PAN==1))
extern const u8 sdp_pan_service_data[200];
u8 pan_profile_support = 1;
const int IPV4_ADDR_CONFLICT_DETECT = 0;
const int  ntp_get_time_init = 0;
SDP_RECORD_HANDLER_REGISTER(pan_sdp_record_item) = {
    .service_record = (u8 *)sdp_pan_service_data,
    .service_record_handle =  0x0001000E,
};
#endif

/*注意hid_conn_depend_on_dev_company置1之后，安卓手机会默认断开HID连接 */
/*注意hid_conn_depend_on_dev_company置2之后，默认不断开HID连接 */
const u8 hid_conn_depend_on_dev_company = 1;
const u8 sdp_get_remote_pnp_info = 0;

//edr 功能配置代码优化
const int CONFIG_BTSTACK_SUPPORT_FUN = HFP_UPDATE_BATTERY;
#ifdef CONFIG_256K_FLASH
const u8 pbg_support_enable = 0;
const u8 adt_profile_support = 0;
const u8 more_hfp_cmd_support = 0;
#else

#if ((THIRD_PARTY_PROTOCOLS_SEL==0)&&(TCFG_LE_AUDIO_APP_CONFIG==0)&&TCFG_USER_BLE_ENABLE)
const u8 pbg_support_enable = 0;
const u8 adt_profile_support = 0;
#else
const u8 pbg_support_enable = 0;
#if (ATT_OVER_EDR_DEMO_EN || (THIRD_PARTY_PROTOCOLS_SEL & AURACAST_APP_EN))
const u8 adt_profile_support = 1;//gatt over edr
#else
const u8 adt_profile_support = 0;
#endif
#endif

const u8 more_hfp_cmd_support = 1;

#endif

#if TCFG_BT_DUAL_CONN_ENABLE
#if USER_SUPPORT_DUAL_A2DP_SOURCE
const u8 more_avctp_cmd_support = 0;
#else
const u8 more_avctp_cmd_support = 1;
#endif
#else
#if (RCSP_MODE == RCSP_MODE_EARPHONE) || TCFG_BT_MUSIC_INFO_ENABLE
const u8 more_avctp_cmd_support = 1;
#else
const u8 more_avctp_cmd_support = 0;
#endif
#endif
#if TCFG_USER_EMITTER_ENABLE
const u8 hci_inquiry_support = 1;
const u8 btstack_emitter_support  = 1;  /*定义用于优化代码编译*/

#if TCFG_BT_SUPPORT_A2DP
const u8 emitter_hfp_hf_support = 0;    /*置1时做发射器连接手机时只支持hfp*/
#else
const u8 emitter_hfp_hf_support = 1;    /*置1时做发射器连接手机时只支持hfp*/
#endif

extern const u8 sdp_a2dp_source_service_data[];

#if (TCFG_BT_SUPPORT_A2DP==0)
u8 a2dp_profile_support = 2;
#endif

SDP_RECORD_HANDLER_REGISTER(a2dp_src_sdp_record_item) = {
    .service_record = (u8 *)sdp_a2dp_source_service_data,
    .service_record_handle =  0x0001000B,
};
#if (TCFG_BT_SUPPORT_PROFILE_HFP_AG==1)
extern const u8 sdp_hfp_ag_service_data[];
u8 hfp_ag_profile_support = 1;
SDP_RECORD_HANDLER_REGISTER(hfp_ag_sdp_record_item) = {
    .service_record = (u8 *)sdp_hfp_ag_service_data,
    .service_record_handle = 0x00010008,
};
#endif
#else
const u8 hci_inquiry_support = 0;
const u8 btstack_emitter_support  = 0;  /*定义用于优化代码编译*/
#endif
/*u8 l2cap_debug_enable = 0xf0;
u8 rfcomm_debug_enable = 0xf;
u8 profile_debug_enable = 0xff;
u8 ble_debug_enable    = 0xff;
u8 btstack_tws_debug_enable = 0xf;*/

#else
const u8 btstack_emitter_support  = 1;  /*定义用于优化代码编译*/
const u8 adt_profile_support = 0;
const u8 pbg_support_enable = 0;
#endif
