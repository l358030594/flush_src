#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".key.data.bss")
#pragma data_seg(".key.data")
#pragma const_seg(".key.text.const")
#pragma code_seg(".key.text")
#endif
#include "app_msg.h"
#include "key_driver.h"
#include "system/timer.h"
#include "app_config.h"
#include "bt_tws.h"
#if (defined TCFG_AUDIO_WIDE_AREA_TAP_ENABLE) && TCFG_AUDIO_WIDE_AREA_TAP_ENABLE
#include "icsd_adt_app.h"
#endif /*TCFG_AUDIO_WIDE_AREA_TAP_ENABLE*/

struct key_hold {
    u8 value;
    u8 action;
    u32 start_time;
};

static struct key_hold key_hold_hdl[1] = {
    { .value = NO_KEY }
};

static struct key_hold *get_key_hold(u8 key_value, int _new)
{
    for (int i = 0; i < ARRAY_SIZE(key_hold_hdl); i++) {
        if (key_hold_hdl[i].value == key_value) {
            return &key_hold_hdl[i];
        }
    }
    if (!_new) {
        return NULL;
    }
    for (int i = 0; i < ARRAY_SIZE(key_hold_hdl); i++) {
        if (key_hold_hdl[i].value == NO_KEY) {
            key_hold_hdl[i].value = key_value;
            return &key_hold_hdl[i];
        }
    }
    return NULL;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief 多击按键判断
 *
 * @param key：基础按键动作（mono_click、long、hold、up）和键值
 *
 * @return 0：不拦截按键事件
 *         1：拦截按键事件
 */
/* ----------------------------------------------------------------------------*/
static int multi_clicks_translate(struct key_event *key)
{
    static u8 click_cnt;
    static u8 notify_value = 0xff;
    struct key_hold *hold = get_key_hold(key->value, 0);

    if (key->event == KEY_ACTION_LONG) {
        hold = get_key_hold(key->value, 1);
        if (hold) {
            hold->start_time = jiffies;
        }
    } else if (key->event == KEY_ACTION_HOLD) {
        if (hold) {
            int time_msec = jiffies_offset_to_msec(hold->start_time, jiffies);
#if TCFG_SEND_HOLD_SEC_MSG_DURING_HOLD
            if (time_msec >= 1000 && hold->action == 0) {
                key->event = KEY_ACTION_HOLD_1SEC;
            } else if (time_msec >= 3000 && hold->action == KEY_ACTION_HOLD_1SEC) {
                key->event = KEY_ACTION_HOLD_3SEC;
            } else if (time_msec >= 5000 && hold->action == KEY_ACTION_HOLD_3SEC) {
                key->event = KEY_ACTION_HOLD_5SEC;
            } else if (time_msec >= 8000 && hold->action == KEY_ACTION_HOLD_5SEC) {
                key->event = KEY_ACTION_HOLD_8SEC;
            } else if (time_msec >= 10000 && hold->action == KEY_ACTION_HOLD_8SEC) {
                key->event = KEY_ACTION_HOLD_10SEC;
            } else {
                return 0;
            }
            if (time_msec >= (TCFG_MAX_HOLD_SEC & 0xff) * 1000) {
                hold->action = KEY_ACTION_HOLD_10SEC;
            } else {
                hold->action = key->event;
            }
#else
            if (time_msec >= (TCFG_MAX_HOLD_SEC & 0xff) * 1000 && hold->action == 0) {
                key->event = TCFG_MAX_HOLD_SEC >> 8;
            } else {
                return 0;
            }
            hold->action = key->event;
#endif
        }
    } else {
        if (hold) {
#if TCFG_SEND_HOLD_SEC_MSG_DURING_HOLD == 0
            if (hold->action == 0) {
                int time_msec = jiffies_offset_to_msec(hold->start_time, jiffies);
                if (time_msec >= 8000) {
                    key->event = KEY_ACTION_HOLD_8SEC;
                } else if (time_msec >= 5000) {
                    key->event = KEY_ACTION_HOLD_5SEC;
                } else if (time_msec >= 3000) {
                    key->event = KEY_ACTION_HOLD_3SEC;
                } else if (time_msec >= 1000) {
                    key->event = KEY_ACTION_HOLD_1SEC;
                }
                if (time_msec >= (TCFG_MAX_HOLD_SEC & 0xff) * 1000) {
                    key->event = TCFG_MAX_HOLD_SEC >> 8;
                }
            }
#endif
            hold->value = NO_KEY;
            hold->action = 0;
            hold->start_time = 0;
        }
    }
    if (key->type == KEY_DRIVER_TYPE_CTMU_TOUCH) {
        return 0;
    }

    if (key->event == KEY_ACTION_CLICK) {
        if (key->value != notify_value) {
            click_cnt = 1;
            notify_value = key->value;
        } else {
            click_cnt++;
        }
        return 1;
    }
    if (key->event == KEY_ACTION_NO_KEY) {
        if (click_cnt == 1) {
            key->event = KEY_ACTION_CLICK;
        } else if (click_cnt <= 7) {
            key->event = KEY_ACTION_DOUBLE_CLICK + (click_cnt - 2);
        }
        key->value = notify_value;
        click_cnt = 0;
        notify_value = NO_KEY;
    } else if (key->event > KEY_ACTION_CLICK) {
        click_cnt = 0;
        notify_value = NO_KEY;
    }
    return 0;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief 联合按键判断
 *
 * @param key：基础按键动作（mono_click、long、hold、up）和键值
 *
 * @return 0：不拦截按键事件
 *         1：拦截按键事件
 */
/* ----------------------------------------------------------------------------*/
static int combination_key_translate(struct key_event *key)
{
    return 0;
}


#if TCFG_USER_TWS_ENABLE
#if TCFG_TWS_COMBINATIION_KEY_ENABLE
static int g_msg[2];
static int g_time[2];

static int tws_combination_key_translate(int msg, int rx)
{
    if (!(tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED)) {
        return msg;
    }

    if (g_msg[0] != g_msg[1]) {
        return msg;
    }
    if (g_msg[0] == 0) {
        return 0;
    }
    int msec = jiffies_offset_to_msec(g_time[0], g_time[1]);
    if (msec > 200 || msec < -200) {
        /*r_printf("---msec: %x, %x, %d\n", g_time[0], g_time[1], msec);*/
        return msg;
    }
    g_msg[0] = 0;
    g_msg[1] = 0;

    int action = APP_MSG_KEY_ACTION(msg);
    if (action == KEY_ACTION_CLICK) {
        action = KEY_ACTION_TWS_CLICK;
    } else if (action >= KEY_ACTION_DOUBLE_CLICK && action <= KEY_ACTION_HOLD_10SEC) {
        action += KEY_ACTION_TWS_DOUBLE_CLICK - KEY_ACTION_DOUBLE_CLICK;
    } else {
        return msg;
    }
    printf("tws_comb_key: %d, %d\n", msec, action);

    return (msg & ~0xff) | action;
}
#endif

static void tws_key_msg_sync_callback(void *_msg)
{
    int msg[2];
    msg[0] = ((u32)_msg) & 0x7fffffff;
    int rx = ((u32)_msg) >> 31;

#if TCFG_TWS_COMBINATIION_KEY_ENABLE
    int key_value = APP_MSG_KEY_VALUE(msg[0]);
    if (key_value == KEY_POWER) {
        msg[0] = tws_combination_key_translate(msg[0], rx);
        if (msg[0] == 0) {
            return;
        }
    }
#endif

    msg[1] = rx ? APP_KEY_MSG_FROM_TWS : 0;
    app_send_message_from(MSG_FROM_KEY, 8, msg);
}


static void tws_key_msg_callback_in_irq(void *data, u16 len, bool rx)
{
    int msg[2];
    msg[0] = *(int *)data;
#if TCFG_TWS_COMBINATIION_KEY_ENABLE
    g_msg[rx] = msg[0];
    g_time[rx] = jiffies;
    sys_hi_timeout_add((void *)(msg[0] | (rx << 31)), tws_key_msg_sync_callback, 250);
    return;
#endif

    msg[1] = rx ? APP_KEY_MSG_FROM_TWS : 0;
    app_send_message_from(MSG_FROM_KEY, 8, msg);
}
REGISTER_TWS_FUNC_STUB(tws_key_msg_sync_entry) = {
    .func_id = 0x8097ADF1,
    .func = tws_key_msg_callback_in_irq,
};

void bt_tws_key_msg_sync(int key_msg)
{
    int msg[2] = { key_msg, 0 };

#if TCFG_TWS_COMBINATIION_KEY_ENABLE
    int action = APP_MSG_KEY_ACTION(key_msg);
    if (action == KEY_ACTION_HOLD) {
        app_send_message_from(MSG_FROM_KEY, 8, msg);
        return;
    }
#endif
    tws_api_tx_unsniff_req();

    int err = tws_api_send_data_to_sibling(&key_msg, 4, 0x8097ADF1);
    if (err) {
        app_send_message_from(MSG_FROM_KEY, 8, msg);
    }
}
#endif

/* --------------------------------------------------------------------------*/
/**
 * @brief 按键事件过滤、检测和发送
 *
 * @param key：基础按键动作（mono_click、long、hold、up）和键值
 */
/* ----------------------------------------------------------------------------*/
void key_event_handler(struct key_event *key)
{
    const struct key_callback *p;

    /*printf("key_event: %d\n", key->event);*/

    if (multi_clicks_translate(key)) {
        return;
    }
    if (combination_key_translate(key)) {
        return;
    }
    if (key->event == KEY_ACTION_NO_KEY) {
        return;
    }

    //外部需要格外做的处理流程，请通过注册的形式在此处回调
    list_for_each_key_callback(p) {
        if (p->cb_deal == NULL) {
            continue;
        }
        if (p->cb_deal(p->arg)) {
            return;
        }
    }
    int msg[2];
    msg[0] = (key->value << 8) | key->event;
    msg[1] = 0;

    /*printf("key_msg: %d\n", msg[0]);*/

#if TCFG_USER_TWS_ENABLE
    bt_tws_key_msg_sync(msg[0]);
#else
    app_send_message_from(MSG_FROM_KEY, 8, msg);
#endif
}


