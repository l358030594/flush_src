#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".charge_store.data.bss")
#pragma data_seg(".charge_store.data")
#pragma const_seg(".charge_store.text.const")
#pragma code_seg(".charge_store.text")
#endif
#include "init.h"
#include "app_config.h"
#include "system/includes.h"
#include "asm/cpu.h"
#include "asm/charge.h"
#include "chargestore/chargestore.h"
#include "user_cfg.h"
#include "app_chargestore.h"
#include "device/vm.h"
#include "btstack/avctp_user.h"
#include "app_power_manage.h"
#include "app_action.h"
#include "app_main.h"
#include "app_charge.h"
#include "classic/tws_api.h"
#include "update.h"
#include "bt_ble.h"
#include "bt_tws.h"
#include "app_action.h"
#include "bt_common.h"
#include "app_testbox.h"
#include "app_charge.h"
#include "battery_manager.h"
#include "classic/tws_api.h"
#include "dual_bank_updata_api.h"

#if (THIRD_PARTY_PROTOCOLS_SEL&RCSP_MODE_EN)
#include "ble_rcsp_server.h"
#include "ble_rcsp_adv.h"
#endif

#define LOG_TAG_CONST       APP_CHARGESTORE
#define LOG_TAG             "[APP_CHARGESTORE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


//-----------------------------------------------------------------------------------
//-----------以下配对相关的接口,提供充电舱和蓝牙测试盒-------------------------------
//-----------------------------------------------------------------------------------
#if (TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE)
void chargestore_set_tws_channel_info(u8 channel)
{
    if ((channel == 'L') || (channel == 'R')) {
        syscfg_write(CFG_CHARGESTORE_TWS_CHANNEL, &channel, 1);
    }
}

u8 chargestore_get_tws_channel_info(void)
{
    u8 channel = 'U';
    syscfg_read(CFG_CHARGESTORE_TWS_CHANNEL, &channel, 1);
    return channel;
}

bool chargestore_set_tws_remote_info(u8 *data, u8 len)
{
    u8 i;
    bool ret = true;
    u8 remote_addr[6];
    u8 common_addr[6];
    u8 local_addr[6];
    CHARGE_STORE_INFO *charge_store_info = (CHARGE_STORE_INFO *)data;
    if (len > sizeof(CHARGE_STORE_INFO)) {
        log_error("len err\n");
        return false;
    }
    //set remote addr
    syscfg_read(CFG_TWS_REMOTE_ADDR, remote_addr, sizeof(remote_addr));
    if (memcmp(remote_addr, charge_store_info->tws_local_addr, sizeof(remote_addr))) {
        ret = false;
    }

#if TCFG_TEST_BOX_ENABLE
    int w_ret = 0;
    if (sizeof(remote_addr) != syscfg_write(CFG_TWS_REMOTE_ADDR, charge_store_info->tws_local_addr, sizeof(remote_addr))) {
        w_ret = -1;
    }
#endif
    bt_get_tws_local_addr(local_addr);

#if (CONFIG_TWS_COMMON_ADDR_SELECT == CONFIG_TWS_COMMON_ADDR_AUTO)
    //set common addr
    syscfg_read(CFG_TWS_COMMON_ADDR, common_addr, sizeof(common_addr));

    for (i = 0; i < sizeof(common_addr); i++) {
        if (common_addr[i] != (u8)(local_addr[i] + charge_store_info->tws_local_addr[i])) {
            ret = false;
        }
        common_addr[i] = local_addr[i] + charge_store_info->tws_local_addr[i];
    }

#if TCFG_TEST_BOX_ENABLE
    if (sizeof(common_addr) != syscfg_write(CFG_TWS_COMMON_ADDR, common_addr, sizeof(common_addr))) {
        w_ret = -3;
    }
#endif
#endif


#if TCFG_TEST_BOX_ENABLE
    if (testbox_get_status() && (0 == w_ret)) {
        u8 cmd = CMD_BOX_TWS_REMOTE_ADDR;
        chargestore_api_write(&cmd, 1);
        testbox_set_testbox_tws_paired(1);
    }
#endif
    return ret;
}

void chargestore_clean_tws_conn_info(u8 type)
{
    CHARGE_STORE_INFO charge_store_info;
    log_info("chargestore_clean_tws_conn_info=%d\n", type);
    if (type == TWS_DEL_TWS_ADDR) {
        log_info("TWS_DEL_TWS_ADDR\n");
    } else if (type == TWS_DEL_PHONE_ADDR) {
        log_info("TWS_DEL_PHONE_ADDR\n");
    } else if (type == TWS_DEL_ALL_ADDR) {
        log_info("TWS_DEL_ALL_ADDR\n");
    }
    memset(&charge_store_info, 0xff, sizeof(CHARGE_STORE_INFO));
    syscfg_write(CFG_TWS_REMOTE_ADDR, charge_store_info.tws_remote_addr, sizeof(charge_store_info.tws_remote_addr));
}


u16 chargestore_get_tws_remote_info(u8 *data)
{
    u16 ret_len = 0;
#if TCFG_USER_TWS_ENABLE
    CHARGE_STORE_INFO *charge_store_info = (CHARGE_STORE_INFO *)data;

    bt_get_tws_local_addr(charge_store_info->tws_local_addr);
    bt_get_vm_mac_addr(charge_store_info->tws_mac_addr);
#if TCFG_TEST_BOX_ENABLE
    if (testbox_get_keep_tws_conn_flag()) {
        memcpy(charge_store_info->tws_mac_addr, bt_get_mac_addr(), 6);
    }
#endif
#ifndef CONFIG_NEW_BREDR_ENABLE
    charge_store_info->search_aa = tws_le_get_search_aa();
    charge_store_info->pair_aa = tws_le_get_pair_aa();
#endif

#if (CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_AS_LEFT) \
    ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_AS_RIGHT) \
    ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_DOWN_AS_LEFT) \
    ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_UP_AS_LEFT) \
    ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_DOWN_AS_RIGHT) \
    ||(CONFIG_TWS_CHANNEL_SELECT == CONFIG_TWS_EXTERN_UP_AS_RIGHT)
    charge_store_info->local_channel = tws_api_get_local_channel();
#else
    charge_store_info->local_channel = 'U';
#endif
    charge_store_info->device_ind = bt_get_tws_device_indicate(NULL);
    charge_store_info->reserved_data = chargestore_api_crc8(data, sizeof(CHARGE_STORE_INFO) - 2);
#if TCFG_TEST_BOX_ENABLE
    if (testbox_get_status()) {
        charge_store_info->reserved_data = 0;
    }
#endif
    ret_len = sizeof(CHARGE_STORE_INFO);
#endif
    return ret_len;
}

#endif //TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE







//-----------------------------------------------------------------------------------
//-----------分割线---------以下是充电舱流程实现-------------------------------------
//-----------------------------------------------------------------------------------
#if TCFG_CHARGESTORE_ENABLE

struct chargestore_info {
    int timer;
    int shutdown_timer;
    u8 version;
    u8 chip_type;
    u8 max_packet_size;//充电舱端一包的最大值
    u8 bt_init_ok;//蓝牙协议栈初始化成功
    u8 power_level;//本机记录的充电舱电量百分比
    u8 pre_power_lvl;
    u8 sibling_chg_lev;//对耳同步的充电舱电量
    u8 power_status;//充电舱供电状态 0:断电 5V不在线 1:升压 5V在线
    u8 cover_status;//充电舱盖子状态 0:合盖 1:开盖
    u8 connect_status;//通信成功
    u8 ear_number;//盒盖时耳机在线数
    u8 channel;//左右
    u8 tws_power;//对耳的电量
    u8 power_sync;//第一次获取到充电舱电量时,同步到对耳
    u8 pair_flag;//配对标记
    u8 close_ing;//等待合窗超时
    u8 active_disconnect;//主动断开连接
    u8 switch2bt;
};

static struct chargestore_info info = {
    .power_status = 1,
    .ear_number = 1,
    .tws_power = 0xff,
    .power_level = 0xff,
    .sibling_chg_lev = 0xff,
    .max_packet_size = 32,
    .channel = 'U',
};

#define __this  (&info)
static u8 local_packet[36];
static CHARGE_STORE_INFO read_info, write_info;
static u8 read_index, write_index;

u8 chargestore_get_power_level(void)
{
    if ((__this->power_level == 0xff) ||
        ((!get_charge_online_flag()) &&
         (__this->sibling_chg_lev != 0xff))) {
        return __this->sibling_chg_lev;
    }
    return __this->power_level;
}

u8 chargestore_get_power_status(void)
{
    return __this->power_status;
}

u8 chargestore_get_cover_status(void)
{
    return __this->cover_status;
}

u8 chargestore_get_earphone_online(void)
{
    return __this->ear_number;
}

void chargestore_set_earphone_online(u8 ear_number)
{
    __this->ear_number = ear_number;
}

void chargestore_set_pair_flag(u8 pair_flag)
{
    __this->pair_flag = pair_flag;
}

void chargestore_set_active_disconnect(u8 active_disconnect)
{
    __this->active_disconnect = active_disconnect;
}

u8 chargestore_get_earphone_pos(void)
{
    u8 channel = chargestore_get_tws_channel_info();
    log_info("get_ear_channel = %c\n", channel);
    return channel;
}

u8 chargestore_get_sibling_power_level(void)
{
    return __this->tws_power;
}

static void set_tws_sibling_charge_level(void *_data, u16 len, bool rx)
{
    if (rx) {
        u8 *data = (u8 *)_data;
        chargestore_set_sibling_chg_lev(data[0]);
    }
}

REGISTER_TWS_FUNC_STUB(charge_level_stub) = {
    .func_id = TWS_FUNC_ID_CHARGE_SYNC,
    .func    = set_tws_sibling_charge_level,
};


int chargestore_sync_chg_level(void)
{
    int err = -1;
    if (app_in_mode(APP_MODE_BT) && !app_var.goto_poweroff_flag) {
        err = tws_api_send_data_to_sibling((u8 *)&__this->power_level, 1,
                                           TWS_FUNC_ID_CHARGE_SYNC);
    }
    return err;
}

void chargestore_set_sibling_chg_lev(u8 chg_lev)
{
    __this->sibling_chg_lev = chg_lev;
}

void chargestore_set_power_level(u8 power)
{
    __this->power_level = power;
}

u8 chargestore_check_going_to_poweroff(void)
{
    return __this->close_ing;
}

void chargestore_shutdown_reset(void)
{
    if (__this->shutdown_timer) {
        sys_timer_del(__this->shutdown_timer);
        __this->shutdown_timer = 0;
    }
}

void chargestore_set_bt_init_ok(u8 flag)
{
    __this->bt_init_ok = flag;
}

void chargestore_shutdown_do(void *priv)
{
    log_info("chargestore shutdown!\n");
    power_set_soft_poweroff();
}

void chargestore_event_to_user(u8 *packet, u8 event, u8 size)
{
    struct chargestore_event evt;

    if (packet != NULL) {
        if (size > sizeof(local_packet)) {
            return;
        }
        memcpy(local_packet, packet, size);
    }
    evt.event   = event;
    evt.packet  = local_packet;
    evt.size    = size;

    os_taskq_post_type("app_core", MSG_FROM_CHARGE_STORE,
                       (sizeof(evt) + 3) / 4, (int *)&evt);
}

bool chargestore_check_data_succ(u8 *data, u8 len)
{
    u16 crc;
    u16 device_ind;
    CHARGE_STORE_INFO *charge_store_info = (CHARGE_STORE_INFO *)data;
    if (len > sizeof(CHARGE_STORE_INFO)) {
        log_error("len err\n");
        return false;
    }
    crc = chargestore_api_crc8(data, len - 2);
    if (crc != charge_store_info->reserved_data) {
        log_error("crc err\n");
        return false;
    }
    device_ind = bt_get_tws_device_indicate(NULL);
    if (device_ind != charge_store_info->device_ind) {
        log_error("device_ind err\n");
        return false;
    }
    return true;
}

u16 chargestore_f95_read_tws_remote_info(u8 *data, u8 flag)
{
    u8 read_len;
    u8 *pbuf = (u8 *)&read_info;
    if (flag) {//first packet
        read_index = 0;
        chargestore_get_tws_remote_info((u8 *)&read_info);
    }
    read_len = sizeof(read_info) - read_index;
    read_len = (read_len > (__this->max_packet_size - 1)) ? (__this->max_packet_size - 1) : read_len;
    memcpy(data, pbuf + read_index, read_len);
    read_index += read_len;
    return read_len;
}

u16 chargestore_f95_write_tws_remote_info(u8 *data, u8 len, u8 flag)
{
    u8 write_len;
    u8 *pbuf = (u8 *)&write_info;
    if (flag) {
        write_index = 0;
        memset(&write_info, 0, sizeof(write_info));
    }
    write_len = sizeof(write_info) - write_index;
    write_len = (write_len >= len) ? len : write_len;
    memcpy(pbuf + write_index, data, write_len);
    write_index += write_len;
    return write_len;
}


void chargestore_timeout_deal(void *priv)
{
    __this->timer = 0;
    __this->close_ing = 0;
    if (!__this->cover_status || __this->active_disconnect) {
        //当前为合盖或者主动断开连接
        if (!app_in_mode(APP_MODE_IDLE)) {
            sys_enter_soft_poweroff(POWEROFF_RESET);
        }
    } else {

        /* #if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV) */
#if 0

        /* if ((!bt_get_total_connect_dev()) && (tws_api_get_role() == TWS_ROLE_MASTER) && (get_bt_tws_connect_status())) { */
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            printf("charge_icon_ctl...\n");
            bt_ble_icon_reset();
#if CONFIG_NO_DISPLAY_BUTTON_ICON
            if (bt_get_total_connect_dev()) {
                //蓝牙未真正断开;重新广播
                bt_ble_icon_open(ICON_TYPE_RECONNECT);
            } else {
                //蓝牙未连接，重新开可见性
                bt_ble_icon_open(ICON_TYPE_INQUIRY);
            }
#else
            //不管蓝牙是否连接，默认打开
            bt_ble_icon_open(ICON_TYPE_RECONNECT);
#endif

        }
#elif (THIRD_PARTY_PROTOCOLS_SEL&RCSP_MODE_EN)
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            printf("charge_icon_ctl...\n");
#if CONFIG_NO_DISPLAY_BUTTON_ICON
            if (bt_get_total_connect_dev()) {
                //蓝牙未真正断开;重新广播
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTED, 1);
            } else {
                //蓝牙未连接，重新开可见性
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
            }
#else
            //不管蓝牙是否连接，默认打开
            bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_CONNECTED, 1);
#endif

        }
#endif

    }
    __this->ear_number = 1;
}

void chargestore_set_phone_disconnect(void)
{
    /* #if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV) */
#if 0
#if (!CONFIG_NO_DISPLAY_BUTTON_ICON)
    if (__this->pair_flag && get_tws_sibling_connect_state()) {
        //check ble inquiry
        //printf("get box log_key...con_dev=%d\n",bt_get_total_connect_dev());
        if ((bt_ble_icon_get_adv_state() == ADV_ST_RECONN)
            || (bt_ble_icon_get_adv_state() == ADV_ST_DISMISS)
            || (bt_ble_icon_get_adv_state() == ADV_ST_END)) {
            bt_ble_icon_reset();
            bt_ble_icon_open(ICON_TYPE_INQUIRY);
        }
    }
#endif
    __this->pair_flag = 0;
#elif (THIRD_PARTY_PROTOCOLS_SEL&RCSP_MODE_EN)
#if (!CONFIG_NO_DISPLAY_BUTTON_ICON)
    if (__this->pair_flag && get_tws_sibling_connect_state()) {
        //check ble inquiry
        bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_UNCONNECTED, 1);
    }
#endif
    __this->pair_flag = 0;
#endif
}

void chargestore_set_phone_connect(void)
{
    __this->active_disconnect = 0;
}

static int app_chargestore_event_handler(int *msg)
{
    int ret = false;
    struct chargestore_event *chargestore_dev = (struct chargestore_event *)msg;

#if defined(TCFG_CHARGE_KEEP_UPDATA) && TCFG_CHARGE_KEEP_UPDATA
    //在升级过程中,不响应智能充电舱app层消息
    if (dual_bank_update_exist_flag_get() || classic_update_task_exist_flag_get()) {
        return ret;
    }
#endif

    switch (chargestore_dev->event) {
    case CMD_RESTORE_SYS:
#if TCFG_USER_TWS_ENABLE
        bt_tws_remove_pairs();
#endif
        bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
        cpu_reset();
        break;
    case CMD_TWS_CHANNEL_SET:
        chargestore_set_tws_channel_info(__this->channel);
        break;
    case CMD_TWS_REMOTE_ADDR:
        log_info("event_CMD_TWS_REMOTE_ADDR\n");
        if (chargestore_set_tws_remote_info(chargestore_dev->packet, chargestore_dev->size) == false) {
            //交换地址后,断开与手机连接,并删除所有连过的手机地址
            bt_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
            __this->ear_number = 2;
            sys_enter_soft_poweroff(POWEROFF_RESET);
        } else {
            __this->pair_flag = 1;
            if (bt_get_total_connect_dev()) {
                __this->active_disconnect = 1;
                bt_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
            } else {
                chargestore_set_phone_disconnect();
            }
        }
        break;
    case CMD_TWS_ADDR_DELETE:
        log_info("event_CMD_TWS_ADDR_DELETE\n");
        chargestore_clean_tws_conn_info(chargestore_dev->packet[0]);
        break;
    case CMD_POWER_LEVEL_OPEN:
        log_info("event_CMD_POWER_LEVEL_OPEN\n");

        //电压过低,不进响应开盖命令
#if TCFG_SYS_LVD_EN
        if (get_vbat_need_shutdown() == TRUE) {
            log_info(" lowpower deal!\n");
            break;
        }
#endif

        if (__this->cover_status) {//当前为开盖
            if (__this->power_sync) {
                if (chargestore_sync_chg_level() == 0) {
                    __this->power_sync = 0;
                }
            }
            if (app_have_mode() && !app_in_mode(APP_MODE_BT) && app_var.goto_poweroff_flag == 0) {
                app_var.play_poweron_tone = 0;
                power_set_mode(TCFG_LOWPOWER_POWER_SEL);
                charge_close();//开盖切蓝牙关闭充电
                __this->switch2bt = 1;
                app_send_message(APP_MSG_GOTO_MODE, APP_MODE_BT);
                __this->switch2bt = 0;
            }
        }
        break;
    case CMD_POWER_LEVEL_CLOSE:
        log_info("event_CMD_POWER_LEVEL_CLOSE\n");
        if (!__this->cover_status) {//当前为合盖
            if (!app_in_mode(APP_MODE_IDLE)) {
                sys_enter_soft_poweroff(POWEROFF_RESET);
            }
        }
        break;
    case CMD_CLOSE_CID:
        log_info("event_CMD_CLOSE_CID\n");
        if (!__this->cover_status) {//当前为合盖
            /* #if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV) */
#if 0
            if ((__this->bt_init_ok) && (tws_api_get_role() == TWS_ROLE_MASTER))  {
                bt_ble_icon_close(1);
            }
#elif (THIRD_PARTY_PROTOCOLS_SEL&RCSP_MODE_EN)
            if ((__this->bt_init_ok) && (tws_api_get_role() == TWS_ROLE_MASTER))  {
                bt_ble_adv_ioctl(BT_ADV_SET_EDR_CON_FLAG, SECNE_DISMISS, 1);
            }
#endif
            if (!__this->timer) {
                __this->timer = sys_timeout_add(NULL, chargestore_timeout_deal, 2000);
                if (!__this->timer) {
                    log_error("timer alloc err!\n");
                } else {
                    __this->close_ing = 1;
                }
            } else {
                sys_timer_modify(__this->timer, 2000);
                __this->close_ing = 1;
            }
        } else {
            __this->ear_number = 1;
            __this->close_ing = 0;
        }
        break;
    case CMD_SHUT_DOWN:
        log_info("event_CMD_SHUT_DOWN\n");
        if (!__this->shutdown_timer) {
            __this->shutdown_timer = sys_timer_add(NULL, chargestore_shutdown_do, 1000);
        } else {
            sys_timer_modify(__this->shutdown_timer, 1000);
        }
        break;
    default:
        break;
    }

    return ret;
}
APP_MSG_HANDLER(chargestore_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_CHARGE_STORE,
    .handler    = app_chargestore_event_handler,
};

static int chargestore_tws_msg_handler(int *msg)
{
    struct tws_event *evt = (struct tws_event *)msg;
    int role = evt->args[0];
    int phone_link_connection = evt->args[1];
    int reason = evt->args[2];

    switch (evt->event) {
    case TWS_EVENT_CONNECTED:
        chargestore_sync_chg_level();//同步充电舱电量
        break;
    case TWS_EVENT_CONNECTION_DETACH:
    case TWS_EVENT_REMOVE_PAIRS:
        if (app_var.goto_poweroff_flag) {
            break;
        }
        chargestore_set_sibling_chg_lev(0xff);
        break;
    }
    return 0;
}
APP_MSG_HANDLER(chargestore_tws_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_TWS,
    .handler    = chargestore_tws_msg_handler,
};


u8 chargestore_get_vbat_percent(void)
{
    u8 power;
#if CONFIG_DISPLAY_DETAIL_BAT
    power = get_vbat_percent();//显示个位数的电量
#else
    power = get_self_battery_level() * 10 + 10; //显示10%~100%
#endif

#if TCFG_CHARGE_ENABLE
    if (get_charge_full_flag()) {
        power = 100;
    } else if (power == 100) {
        power = 99;
    }
    if (get_charge_online_flag()) {
        power |= BIT(7);
    }
#endif
    return power;
}

void chargestore_set_power_status(u8 *buf, u8 len)
{
    __this->version = buf[0] & 0x0f;
    __this->chip_type = (buf[0] >> 4) & 0x0f;
    //f95可能传一个大于100的电量
    if ((buf[1] & 0x7f) > 100) {
        __this->power_level = (buf[1] & 0x80) | 100;
    } else {
        __this->power_level = buf[1];
    }
    if (len > 2) {
        __this->max_packet_size = buf[2];
        if (len > 3) {
            __this->tws_power = buf[3];
        }
    }
}

static int app_chargestore_data_handler(u8 *buf, u8 len)
{
    u8 send_buf[36];
    /* log_info_hexdump(buf, len); */
    chargestore_shutdown_reset();
    send_buf[0] = buf[0];
#ifdef CONFIG_CHARGESTORE_REMAP_ENABLE
    if (remap_app_chargestore_data_deal(buf, len)) {
        return 1;
    }
#endif

    switch (buf[0]) {
    case CMD_TWS_CHANNEL_SET:
        __this->channel = (buf[1] == TWS_CHANNEL_LEFT) ? 'L' : 'R';
        chargestore_event_to_user(NULL, buf[0], 0);
        if (__this->bt_init_ok) {
            len = chargestore_get_tws_remote_info(&send_buf[1]);
            chargestore_api_write(send_buf, len + 1);
        } else {
            send_buf[0] = CMD_UNDEFINE;
            chargestore_api_write(send_buf, 1);
        }
        break;
    case CMD_TWS_SET_CHANNEL:
        __this->channel = (buf[1] == TWS_CHANNEL_LEFT) ? 'L' : 'R';
        log_info("f95 set channel = %c\n", __this->channel);
        chargestore_event_to_user(NULL, CMD_TWS_CHANNEL_SET, 0);
        if (__this->bt_init_ok) {
            chargestore_api_write(send_buf, 1);
        } else {
            send_buf[0] = CMD_UNDEFINE;
            chargestore_api_write(send_buf, 1);
        }
        break;

    case CMD_TWS_REMOTE_ADDR:
        __this->close_ing = 0;
        if (chargestore_check_data_succ((u8 *)&buf[1], len - 1) == true) {
            chargestore_event_to_user((u8 *)&buf[1], buf[0], len - 1);
        } else {
            send_buf[0] = CMD_FAIL;
        }
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_EX_FIRST_READ_INFO:
        log_info("read first!\n");
        __this->close_ing = 0;
        len = chargestore_f95_read_tws_remote_info(&send_buf[1], 1);
        chargestore_api_write(send_buf, len + 1);
        break;
    case CMD_EX_CONTINUE_READ_INFO:
        log_info("read continue!\n");
        __this->close_ing = 0;
        len = chargestore_f95_read_tws_remote_info(&send_buf[1], 0);
        chargestore_api_write(send_buf, len + 1);
        break;
    case CMD_EX_FIRST_WRITE_INFO:
        log_info("write first!\n");
        __this->close_ing = 0;
        chargestore_f95_write_tws_remote_info(&buf[1], len - 1, 1);
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_EX_CONTINUE_WRITE_INFO:
        log_info("write continue!\n");
        __this->close_ing = 0;
        chargestore_f95_write_tws_remote_info(&buf[1], len - 1, 0);
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_EX_INFO_COMPLETE:
        log_info("ex complete!\n");
        if (chargestore_check_data_succ((u8 *)&write_info, sizeof(write_info)) == true) {
            chargestore_event_to_user((u8 *)&write_info, CMD_TWS_REMOTE_ADDR, sizeof(write_info));
        } else {
            send_buf[0] = CMD_FAIL;
        }
        chargestore_api_write(send_buf, 1);
        break;

    case CMD_TWS_ADDR_DELETE:
        __this->close_ing = 0;
        chargestore_event_to_user(&buf[1], CMD_TWS_ADDR_DELETE, len - 1);
        chargestore_api_write(send_buf, 1);
        break;
    case CMD_POWER_LEVEL_OPEN:
        __this->power_status = 1;
        __this->cover_status = 1;
        __this->close_ing = 0;
        if (__this->power_level == 0xff) {
            __this->power_sync = 1;
        }
        chargestore_set_power_status(&buf[1], len - 1);
        if (__this->power_level != __this->pre_power_lvl) {
            __this->power_sync = 1;
        }
        __this->pre_power_lvl = __this->power_level;
        send_buf[1] = chargestore_get_vbat_percent();
        send_buf[2] = chargestore_get_det_level(__this->chip_type);
        chargestore_api_write(send_buf, 3);
        //切模式过程中不发送消息,防止堆满消息
        if (__this->switch2bt == 0) {
            chargestore_event_to_user(NULL, CMD_POWER_LEVEL_OPEN, 0);
        }
        break;
    case CMD_POWER_LEVEL_CLOSE:
        __this->power_status = 1;
        __this->cover_status = 0;
        __this->close_ing = 0;
        chargestore_set_power_status(&buf[1], len - 1);
        send_buf[1] = chargestore_get_vbat_percent();
        send_buf[2] = chargestore_get_det_level(__this->chip_type);
        chargestore_api_write(send_buf, 3);
        chargestore_event_to_user(NULL, CMD_POWER_LEVEL_CLOSE, 0);
        break;
    case CMD_SHUT_DOWN:
        log_info("shut down\n");
        __this->power_status = 0;
        __this->cover_status = 0;
        __this->close_ing = 0;
        chargestore_api_write(send_buf, 1);
        chargestore_event_to_user(NULL, CMD_SHUT_DOWN, 0);
        break;
    case CMD_CLOSE_CID:
        log_info("close cid\n");
        __this->power_status = 1;
        __this->cover_status = 0;
        __this->ear_number = buf[1];
        chargestore_api_write(send_buf, 1);
        chargestore_event_to_user(NULL, CMD_CLOSE_CID, 0);
        break;

    case CMD_RESTORE_SYS:
        r_printf("restore sys\n");
        __this->power_status = 1;
        __this->cover_status = 1;
        __this->close_ing = 0;
        chargestore_api_write(send_buf, 1);
        chargestore_event_to_user(NULL, CMD_RESTORE_SYS, 0);
        break;
    //不是充电舱的指令,返回0
    default:
        return 0;
    }
    return 1;
}

CHARGESTORE_HANDLE_REG(chargestore, app_chargestore_data_handler);

#if TCFG_CHARGE_ENABLE
static int app_chargestore_charge_msg_handler(int msg, int type)
{
    switch (msg) {
    case CHARGE_EVENT_LDO5V_KEEP:
        if (!get_charge_poweron_en()) {
            batmgr_send_msg(BAT_MSG_CHARGE_LDO5V_OFF, 0);
            return 1;//拦截关机,智能充电舱不在维持电压消息关机
        }
        break;
    case CHARGE_EVENT_LDO5V_IN:
        chargestore_shutdown_reset();
        if (!get_charge_poweron_en()) {
//开启了弹窗才需要等待弹窗关闭才复位进充电
            /* #if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV) */
#if 0
            if (chargestore_check_going_to_poweroff()) {
                log_info("chargestore do poweroff!\n");
                return 1;//不要关闭蓝牙,让充电舱流程执行关蓝牙
            }
#endif
        }
        break;
    case CHARGE_EVENT_LDO5V_OFF:
#if TCFG_USER_TWS_ENABLE
        if (TWS_ROLE_SLAVE == tws_api_get_role()) {
            chargestore_set_power_level(0xFF);
        }
#endif
        if (chargestore_get_power_status() == 0) {
            return 1;
        }
        break;
    }
    return 0;
}

APP_CHARGE_HANDLER(chargestore_charge_msg_entry, 0) = {
    .handler = app_chargestore_charge_msg_handler,
};
#endif

#endif //TCFG_CHARGESTORE_ENABLE

