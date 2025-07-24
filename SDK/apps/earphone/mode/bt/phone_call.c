#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".phone_call.data.bss")
#pragma data_seg(".phone_call.data")
#pragma const_seg(".phone_call.text.const")
#pragma code_seg(".phone_call.text")
#endif
#include "system/includes.h"
#include "media/includes.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/a2dp_media_codec.h"
#include "btstack/bluetooth.h"
#include "btctrler/btctrler_task.h"
#include "btctrler/btcontroller_modules.h"
#include "classic/hci_lmp.h"
#include "bt_common.h"
#include "bt_ble.h"
#include "pbg_user.h"
#include "user_cfg.h"
#include "audio_cvp.h"
#include "app_tone.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "esco_recoder.h"
#include "earphone.h"
#include "app_config.h"
#include "app_testbox.h"
#include "app_online_cfg.h"
#include "app_main.h"
#include "vol_sync.h"
#include "audio_config.h"
#include "bt_slience_detect.h"
#if TCFG_SMART_VOICE_ENABLE
#include "asr/jl_kws.h"
#include "smart_voice/smart_voice.h"
#endif
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
#include "jl_kws/jl_kws_api.h"
#endif
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
#include "app_le_connected.h"
#endif
#if TCFG_AUDIO_SOMATOSENSORY_ENABLE
#include "somatosensory/audio_somatosensory.h"
#endif
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif
#if (TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)
#include "le_audio_player.h"
#include "app_le_auracast.h"
#endif


#if (TCFG_USER_TWS_ENABLE == 0)

#define LOG_TAG             "[PHONECALL]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if TCFG_APP_BT_EN

/*配置通话时前面丢掉的数据包包数*/
#define ESCO_DUMP_PACKET_ADJUST		1	/*配置使能*/
#define ESCO_DUMP_PACKET_DEFAULT	0
#define ESCO_DUMP_PACKET_CALL		60 /*0~0xFF*/
#define SYNC_TONE_PHONE_RING_TIME   300

static u8 esco_dump_packet = ESCO_DUMP_PACKET_CALL;

static void phone_income_num_check(void *priv);

enum {
    CMD_OPEN_ESCO_PLAYER = 1,
    CMD_CLOSE_ESCO_PLAYER,
    CMD_PHONE_INCOME,
    CMD_PHONE_OUT,
    CMD_PHONE_HANGUP,
    CMD_PHONE_ACTIVE,
    CMD_PHONE_LASET_CALL_TYPE,
    CMD_PHONE_SIRI,
};

void tone_ring_player_stop()
{
    tone_player_stop();
    ring_player_stop();
}


static u8 phone_ring_play_start(void)
{
    log_info("%s,%d\n", __FUNCTION__, g_bt_hdl.inband_ringtone);
    /* check if support inband ringtone */
    if (BT_CALL_INCOMING != bt_get_call_status_for_addr(g_bt_hdl.phone_ring_addr)) {
        return 0;
    }
    if (!g_bt_hdl.inband_ringtone) {
        return 1;
    }
    return 0;
}

void phone_second_call_ring_play_start(void *priv)
{
//音频设计只有在ESCO工作的时候，播嘟嘟声才能叠加
//两种情况，一是先有嘟嘟声，然后来了ESCO铃声，要先关嘟嘟再开。
//二是两个都是ESCO铃声，第二个打断了第一个之后，第一个的铃声变成嘟嘟叠加。指示有两个来电
    u8 *other_addr = btstack_get_other_dev_addr(priv);
    if (!other_addr) {
        return;
    }
    printf("bt_get_call_status_for_addr=%d,%d\n",
           bt_get_call_status_for_addr(priv),
           bt_get_call_status_for_addr(other_addr));
    if (bt_stack_get_incoming_call_num() > 1 ||
        (bt_get_call_status_for_addr(priv) == BT_CALL_INCOMING &&
         bt_get_call_status_for_addr(other_addr) != BT_CALL_OUTGOING)) {
        play_ring_file_alone(get_tone_files()->phone_in);
    }
}

int bt_phone_income(u8 after_conn, u8 *bt_addr)
{
#if TCFG_BT_INBAND_RING
#else
    lmp_private_esco_suspend_resume(3);
#endif
    tone_ring_player_stop();

    memcpy(g_bt_hdl.phone_ring_addr, bt_addr, 6);
    put_buf(g_bt_hdl.phone_ring_addr, 6);

    void *device = btstack_get_conn_device(bt_addr);
    if (!device || BT_CALL_HANGUP == btstack_bt_get_call_status(device)) {
        puts("bt_phone_income err 1\n");
        return 1;
    }
#if TCFG_BT_INBAND_RING
    g_bt_hdl.inband_ringtone = btstack_get_inband_ringtone_flag_for_addr(bt_addr);
#else
    g_bt_hdl.inband_ringtone = 0 ;
#endif
#if ((TCFG_LE_AUDIO_APP_CONFIG & (LE_AUDIO_UNICAST_SINK_EN | LE_AUDIO_JL_UNICAST_SINK_EN)))
    if (is_cig_phone_conn()) {
        g_bt_hdl.inband_ringtone = 1;
    }
#endif

    printf("inband_ringtone = 0x%x,after_conn = %d\n", g_bt_hdl.inband_ringtone, after_conn);
    g_bt_hdl.phone_ring_flag = 1;
    g_bt_hdl.phone_income_flag = 1;

#if TCFG_BT_DUAL_CONN_ENABLE
    if (bt_stack_get_incoming_call_num() > 1) {
        //第一次来电被置上一次。第二次再来就有值了
        sys_timeout_add(g_bt_hdl.phone_ring_addr, phone_second_call_ring_play_start, 2000);
        return 0;
    }
    if (g_bt_hdl.inband_ringtone && after_conn == 2) { //强制播本地铃声
        sys_timeout_add(g_bt_hdl.phone_ring_addr, phone_second_call_ring_play_start, 2000);
        return 0;
    }
#endif

#if TCFG_BT_PHONE_NUMBER_ENABLE
    if (after_conn) {
        phone_ring_play_start();
    }
#else
    phone_ring_play_start();
#endif

    return 0;
}

int bt_phone_hangup(u8 *bt_addr)
{
    esco_dump_packet = ESCO_DUMP_PACKET_CALL;
    log_info("phone_handup\n");
    g_bt_hdl.phone_ring_flag = 0;
    tone_ring_player_stop();
    if (g_bt_hdl.phone_timer_id) {
        sys_timeout_del(g_bt_hdl.phone_timer_id);
        g_bt_hdl.phone_timer_id = 0;
    }
    g_bt_hdl.phone_income_flag = 0;
    g_bt_hdl.phone_num_flag = 0;
    lmp_private_esco_suspend_resume(4);

    /*
     * 挂断的时候会清除了一些标识并且会stop了提示音
     * 判断如果另一个手机还在来电并且不支持inband ring，那就恢复一个嘟嘟声提示音
     */
#if TCFG_BT_DUAL_CONN_ENABLE
    u8 *addr = btstack_get_other_dev_addr(bt_addr);
    if (addr && bt_get_call_status_for_addr(bt_addr) != BT_CALL_OUTGOING) {
        //有另一个连接存在
        u8 call_status = bt_get_call_status_for_addr(addr);
        u8 inband_ring_flag = btstack_get_inband_ringtone_flag_for_addr(addr);
        printf("other connect call sta:%d-%d-%d\n", call_status,
               inband_ring_flag, bt_check_esco_state_via_addr(addr));
        if (call_status == BT_CALL_INCOMING) {
            memcpy(g_bt_hdl.phone_ring_addr, addr, 6);
            if (inband_ring_flag == 0 ||
                (bt_check_esco_state_via_addr(g_bt_hdl.phone_ring_addr) == BT_ESCO_STATUS_CLOSE)) {
                bt_phone_income(2, g_bt_hdl.phone_ring_addr);

            }
        }
    }
#endif
    return 0;

}
static int bt_phone_out(u8 *bt_addr)
{
    esco_dump_packet = ESCO_DUMP_PACKET_CALL;
    g_bt_hdl.phone_income_flag = 0;
    return 0;
}

static int esco_audio_open(u8 *bt_addr)
{
#if TCFG_AUDIO_ANC_EAR_ADAPTIVE_EN && TCFG_AUDIO_ANC_ENABLE
    //自适应与通话互斥，等待自适应结束之后再打开音频流程
    if (anc_ear_adaptive_busy_get()) {
        anc_ear_adaptive_forced_exit(1, 1);
    }
#endif
    esco_player_open(bt_addr);
#if TCFG_TWS_POWER_BALANCE_ENABLE && TCFG_USER_TWS_ENABLE
    /* #if 0	//功能未完善，暂时关闭 */
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        log_info("tws_master open esco recoder\n");
        esco_recoder_open(COMMON_SCO, bt_addr);
    } else {
        log_info("tws_slave don't open esco recoder\n");
    }
#else
    esco_recoder_open(COMMON_SCO, bt_addr);
#endif/*TCFG_TWS_POWER_BALANCE_ENABLE && TCFG_USER_TWS_ENABLE*/
    return 0;
}

static int bt_phone_active(u8 *bt_addr)
{
    if (g_bt_hdl.phone_call_dec_begin) {
        log_info("call_active,dump_packet clear\n");
        esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
    }
    if (g_bt_hdl.phone_ring_flag) {
        g_bt_hdl.phone_ring_flag = 0;
        tone_ring_player_stop();
        if (g_bt_hdl.phone_timer_id) {
            sys_timeout_del(g_bt_hdl.phone_timer_id);
            g_bt_hdl.phone_timer_id = 0;
        }
    }

    lmp_private_esco_suspend_resume(4);
    g_bt_hdl.phone_income_flag = 0;
    g_bt_hdl.phone_num_flag = 0;
    /* g_bt_hdl.phone_con_sync_ring = 0; */
    g_bt_hdl.phone_vol = 15;
    r_printf("phone_active:%d\n", app_var.call_volume);
    app_audio_set_volume(APP_AUDIO_STATE_CALL, app_var.call_volume, 1);
    return 0;

}

struct esco_delay_ctl {
    u8 esco_addr[6];
    u16 timer;
    u16 delay_timeout_cnt;
};
static struct esco_delay_ctl ed_ctl = {0};
static void esco_delay_open(void *priv)
{
    ed_ctl.delay_timeout_cnt++;
    if (ed_ctl.delay_timeout_cnt >= 60) {
        tone_player_stop();
    }
    if ((!tone_player_runing() && !key_tone_player_running()) || (ed_ctl.delay_timeout_cnt >= 60)) {
        ed_ctl.delay_timeout_cnt = 0;
        lmp_private_esco_suspend_resume(2);
        esco_audio_open(ed_ctl.esco_addr);
        sys_timer_del(ed_ctl.timer);
        ed_ctl.timer = 0;
    }
}

#define ESCO_SIRI_WAKEUP()      (app_var.siri_stu == 1 || app_var.siri_stu == 2)
static void esco_smart_voice_detect_handler(void)
{
#if TCFG_SMART_VOICE_ENABLE
#if TCFG_CALL_KWS_SWITCH_ENABLE
    if (ESCO_SIRI_WAKEUP() || (bt_get_call_status() != BT_CALL_INCOMING)) {
        audio_smart_voice_detect_close();
    }
#else
    audio_smart_voice_detect_close();
#endif
#endif
}

int bt_phone_esco_play(u8 *bt_addr)
{
    puts("<<<<<<<<<<<esco_dec_stat\n");
    mem_stats();
    u8 temp_btaddr[6];
    if (esco_player_get_btaddr(temp_btaddr)) {
        //已经有设备在用了，不能重复初始化
        printf("CMD_OPEN_ESCO_PLAYER error\n");
        return 1;
    }
#if TCFG_SMART_VOICE_ENABLE
    esco_smart_voice_detect_handler();
#endif

#if TCFG_AUDIO_SOMATOSENSORY_ENABLE && SOMATOSENSORY_CALL_EVENT
    somatosensory_open();
#endif
    if (a2dp_player_get_btaddr(temp_btaddr)) {
        a2dp_player_close(temp_btaddr);
        a2dp_media_close(temp_btaddr);
    }
    bt_stop_a2dp_slience_detect(bt_addr);
    a2dp_media_close(bt_addr);
#if 0   //debug
    void mic_capless_feedback_toggle(u8 toggle);
    mic_capless_feedback_toggle(0);
#endif
#if TCFG_BT_INBAND_RING
    tone_ring_player_stop();
#else
    if (BT_CALL_INCOMING == bt_get_call_status_for_addr(bt_addr)) {
        lmp_private_esco_suspend_resume(3);
    } else {
        tone_ring_player_stop();
    }
#endif
    bt_api_esco_status(bt_addr, BT_ESCO_STATUS_OPEN);
    printf("bt_phone_esco_play:%d %d %d\n", tone_player_runing(), key_tone_player_running(), ed_ctl.timer);
    if (tone_player_runing() || key_tone_player_running()) {
        if (!ed_ctl.timer) {
            lmp_private_esco_suspend_resume(1);
            memcpy(ed_ctl.esco_addr, bt_addr, 6);
            ed_ctl.timer = sys_timer_add(NULL, esco_delay_open, 50);
        }
    } else {
        esco_audio_open(bt_addr);
    }

    g_bt_hdl.phone_call_dec_begin = 1;
    if (bt_get_call_status() == BT_CALL_ACTIVE) {
        log_info("dec_begin,dump_packet clear\n");
        esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
    }
#if TCFG_BT_PHONE_NUMBER_ENABLE
    y_printf("play the calling number\n");
    phone_income_num_check(NULL);
#endif
    pbg_user_mic_fixed_deal(1);
    return 0;

}

int bt_phone_esco_stop(u8 *bt_addr)
{
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
    /* 处理来电时挂断电话，先跑释放资源再收到handup命令的情况
     * 避免先开smart voice，再关闭"yes/no"，导致出错*/
    jl_kws_speech_recognition_close();
#endif /*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

    puts("<<<<<<<<<<<esco_dec_stop\n");
#if TCFG_AUDIO_SOMATOSENSORY_ENABLE && SOMATOSENSORY_CALL_EVENT
    somatosensory_close();
#endif
    g_bt_hdl.phone_call_dec_begin = 0;
    esco_dump_packet = ESCO_DUMP_PACKET_CALL;
    bt_api_esco_status(bt_addr, BT_ESCO_STATUS_CLOSE);
    esco_recoder_close();
    esco_player_close();

#if TCFG_SMART_VOICE_ENABLE
    audio_smart_voice_detect_open(JL_KWS_COMMAND_KEYWORD);
#endif

    if (app_var.goto_poweroff_flag) {
        return 0;
    }
    pbg_user_mic_fixed_deal(0);
    return 0;

}
int bt_phone_last_call_type(u8 *bt_addr, u8 bt_value)
{
    g_bt_hdl.last_call_type = bt_value;
    return 0;
}
int bt_phone_siri(u8 *bt_addr, u8 bt_value)
{
    app_var.siri_stu = bt_value;
    esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
    return 0;
}


void phone_sync_vol(void)
{
    bt_cmd_prepare(USER_CTRL_HFP_CALL_SET_VOLUME, 1, &g_bt_hdl.phone_vol);
}

#if ESCO_DUMP_PACKET_ADJUST
u8 get_esco_packet_dump(void)
{
    //log_info("esco_dump_packet:%d\n", esco_dump_packet);
    return esco_dump_packet;
}
#endif


static u8 check_phone_income_idle(void)
{
    if (g_bt_hdl.phone_ring_flag) {
        return 0;
    }
    return 1;
}
REGISTER_LP_TARGET(phone_incom_lp_target) = {
    .name       = "phone_check",
    .is_idle    = check_phone_income_idle,
};


static int bt_phone_num_to_file_list(const char *file_list[], int max_file_num)
{
    int i;
    char *num = (char *)(g_bt_hdl.income_phone_num);

    printf("phone_num: %s\n", num);

    for (i = 0; (i < max_file_num) && (num[i] != '\0'); i++) {
        file_list[i] = get_tone_files()->num[num[i] - '0'];
    }
    return i;
}

int phone_income_num_play_end(void *priv, enum stream_event ev)
{
    printf("%s\n", __func__);
    switch (ev) {
    case STREAM_EVENT_STOP:
        ///来电报号结束，启动本地铃声播放
        phone_ring_play_start();
        break;
    default:
        break;
    }
    return 0;
}

static void phone_income_num_check(void *priv)
{
    const char *file_list[20];
    g_bt_hdl.phone_timer_id = 0;

    if (g_bt_hdl.phone_num_flag) {
        if (g_bt_hdl.phone_ring_flag) {
            tone_ring_player_stop();
            int file_num = bt_phone_num_to_file_list(file_list, ARRAY_SIZE(file_list));
            if (g_bt_hdl.inband_ringtone == 0) {
                play_tone_files_alone_callback(file_list, file_num, NULL, phone_income_num_play_end);
            } else {
                play_tone_files_alone(file_list, file_num);
            }
        }
    } else {
        /*电话号码还没有获取到，定时查询*/
        g_bt_hdl.phone_timer_id = sys_timeout_add(NULL, phone_income_num_check, 200);
    }
}

bool is_siri_open()
{
    if (app_var.siri_stu == 1 || app_var.siri_stu == 2) {
        return 1;
    }
    return 0;
}

/*
 * 对应原来的状态处理函数，连接，电话状态等
 */
static int bt_phone_status_event_handler(int *msg)
{
    u8 *phone_number = NULL;
    struct bt_event *bt = (struct bt_event *)msg;

    switch (bt->event) {
    case BT_STATUS_PHONE_INCOME:
        log_info("BT_STATUS_PHONE_INCOME\n");
        put_buf(bt->args, 6);
        esco_dump_packet = ESCO_DUMP_PACKET_CALL;
        /*
         *(1)1t2有一台通话的时候，另一台如果来电会叠加
         */
        bt_phone_income(0, bt->args);
        bt_cmd_prepare(USER_CTRL_HFP_CALL_CURRENT, 0, NULL); //发命令获取电话号码
        break;
    case BT_STATUS_PHONE_OUT:
        log_info("BT_STATUS_PHONE_OUT\n");
        put_buf(bt->args, 6);
        bt_phone_out(bt->args);
        bt_cmd_prepare(USER_CTRL_HFP_CALL_CURRENT, 0, NULL); //发命令获取电话号码
        break;
    case BT_STATUS_PHONE_ACTIVE:
        log_info("BT_STATUS_PHONE_ACTIVE\n");
        bt_phone_active(bt->args);
        break;
    case BT_STATUS_PHONE_HANGUP:
        log_info("BT_STATUS_PHONE_HANGUP\n");
        put_buf(bt->args, 6);
        bt_phone_hangup(bt->args);
        break;
    case BT_STATUS_PHONE_NUMBER:
        log_info("BT_STATUS_PHONE_NUMBER\n");
#if TCFG_BT_SUPPORT_PBAP_LIST
        bt_cmd_prepare(USER_CTRL_PBAP_READ_LIST, 0, NULL);
#endif
#if TCFG_BT_PHONE_NUMBER_ENABLE
        phone_number = (u8 *)bt->value;
        printf("phone_number = %s\n", phone_number);
        if (g_bt_hdl.phone_num_flag == 1) {
            break;
        }
        g_bt_hdl.income_phone_len = 0;
        memset(g_bt_hdl.income_phone_num, '\0',
               sizeof(g_bt_hdl.income_phone_num));
        for (int i = 0; i < strlen((const char *)phone_number); i++) {
            if (phone_number[i] >= '0' && phone_number[i] <= '9') {
                //过滤，只有数字才能报号
                g_bt_hdl.income_phone_num[g_bt_hdl.income_phone_len++] = phone_number[i];
                if (g_bt_hdl.income_phone_len >= sizeof(g_bt_hdl.income_phone_num)) {
                    break;    /*buffer 空间不够，后面不要了*/
                }
            }
        }
        if (g_bt_hdl.income_phone_len > 0) {
            g_bt_hdl.phone_num_flag = 1;  //等esco建立后才开始来电号码的播报
        } else {
            log_info("PHONE_NUMBER len err\n");
        }
#endif
        break;
    case BT_STATUS_PHONE_NAME:
        log_info("BT_STATUS_PHONE_NAME\n");
        u8 *phone_name = (u8 *)bt->value;
        log_info(">>name:%s\n", phone_name);
        break;
    case BT_STATUS_INBAND_RINGTONE:
        log_info("BT_STATUS_INBAND_RINGTONE\n");
#if TCFG_BT_INBAND_RING
        g_bt_hdl.inband_ringtone = bt->value;
#else
        g_bt_hdl.inband_ringtone = 0;
#endif
        printf("inband_ringtone=0x%x\n", g_bt_hdl.inband_ringtone);
        break;

    case BT_STATUS_SCO_STATUS_CHANGE:
        printf("BT_STATUS_SCO_STATUS_CHANGE len:%d, type:%d\n",
               (bt->value >> 16), (bt->value & 0x0000ffff));
        if (bt->value != 0xff) {
#if (TCFG_LE_AUDIO_APP_CONFIG & LE_AUDIO_AURACAST_SINK_EN)
            if (le_audio_player_is_playing()) {
                le_auracast_stop();
            }
#endif
            u8 call_vol = 15;
            //为了解决两个手机都在通话，在手机上轮流切声卡的音量问题
            call_vol = bt_get_call_vol_for_addr(bt->args);
            //r_printf("---bt_get_call_vol_for_addr--%d\n", call_vol);
            app_audio_state_switch(APP_AUDIO_STATE_CALL, app_var.aec_dac_gain, NULL);
            app_audio_set_volume(APP_AUDIO_STATE_CALL, call_vol, 1);
#if (TCFG_BT_ESCO_PLAYER_ENABLE == 0)
            lmp_private_esco_suspend_resume(1);
#else
            bt_phone_esco_play(bt->args);
#endif

        } else {
            u8 bt_esco_play[6];
            int ret = esco_player_get_btaddr(bt_esco_play);
            if (ret && memcmp(bt_esco_play, bt->args, 6) != 0) {
                //如果有地址在是用esco音频，但跟传出来的值地址不一致，就不更新了。
                puts("<<<<<<<<<<<bt_esco_stop err,check addr\n");
                break;
            }

            bt_phone_esco_stop(bt->args);
            bt_phone_esco_stop(bt->args);

        }
        break;
    case BT_STATUS_CALL_VOL_CHANGE:
        //判断是当前地址的音量值才更新
        u8 bt_esco_play[6];
        int ret = esco_player_get_btaddr(bt_esco_play);
        if (ret && memcmp(bt_esco_play, bt->args, 6) != 0) {
            //如果有地址在是用esco音频，但跟传出来的值地址不一致，就不更新了。
            break;
        }
        printf("call_vol:%d", bt->value);
        u8 volume = bt->value;
        u8 call_status = bt_get_call_status_for_addr(bt->args);

#if TCFG_BT_INBAND_RING
        if ((call_status != BT_CALL_ACTIVE) && (bt->value == 0)) {
            printf("not set vol\n");
            break;
        }
#endif

        g_bt_hdl.phone_vol = bt->value;
        if ((call_status == BT_CALL_ACTIVE) ||
            (call_status == BT_CALL_OUTGOING) || app_var.siri_stu) {
            app_audio_set_volume(APP_AUDIO_STATE_CALL, volume, 1);
        } else {
            /*
             *只保存，不设置到dac
             *微信语音通话的时候，会更新音量到app，但是此时的call status可能是hangup
             */
            app_var.call_volume = volume;
        }
        break;

    case BT_STATUS_LAST_CALL_TYPE_CHANGE:
        log_info("BT_STATUS_LAST_CALL_TYPE_CHANGE:%d\n", bt->value);
        g_bt_hdl.last_call_type = bt->value;
        bt_phone_last_call_type(bt->args, bt->value);
        break;

    case BT_STATUS_CONN_HFP_CH:
        r_printf("++++++++ BT_STATUS_CONN_HFP_CH +++++++++  \n");
        break;
    case BT_STATUS_DISCON_HFP_CH:
        r_printf("++++++++ BT_STATUS_DISCON_HFP_CH +++++++++  \n");
        break;
    case BT_STATUS_PHONE_MANUFACTURER:
        log_info("BT_STATUS_PHONE_MANUFACTURER:%d\n", bt->value);
        app_var.remote_dev_company = bt->value;
        break;
    case BT_STATUS_SIRI_OPEN:
    case BT_STATUS_SIRI_CLOSE:
    case BT_STATUS_SIRI_GET_STATE:
        /* case BT_STATUS_VOICE_RECOGNITION: */
        log_info(" BT_STATUS_SIRI_RECOGNITION:%d \n", bt->value);
        /* put_buf(bt, sizeof(struct bt_event)); */
        app_var.siri_stu = bt->value;
        esco_dump_packet = ESCO_DUMP_PACKET_DEFAULT;
        bt_phone_siri(bt->args, bt->value);
        break;
    case BT_STATUS_SCO_CONNECTION_REQ :
        g_printf(" BT_STATUS_SCO_CONNECTION_REQ");
        put_buf(bt->args, 6);
        break;
    case BT_STATUS_RECONN_OR_CONN :
#if TCFG_BT_SUPPORT_PBAP_LIST
        bt_cmd_prepare(USER_CTRL_PBAP_CONNECT, 0, NULL);
#endif
        break;
    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }

    return 0;
}
APP_MSG_HANDLER(bt_phone_msg_handler_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_STACK,
    .handler    = bt_phone_status_event_handler,
};


static int phone_hci_event_handler(int *event)
{
    struct bt_event *bt = (struct bt_event *)event;

    switch (bt->event) {
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (esco_player_is_playing(bt->args)) {
            bt_phone_esco_stop(bt->args);
        }
        break;
    }

    return 0;
}
APP_MSG_HANDLER(phone_hci_msg_stub) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BT_HCI,
    .handler    = phone_hci_event_handler,
};

#endif
#endif
