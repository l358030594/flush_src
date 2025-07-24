
/*************************************************************
  此文件函数主要是linein 插入检测

  支持和sd卡 io复用检测
 **************************************************************/

#include "app_config.h"
#include "system/event.h"
#include "system/init.h"
#include "system/timer.h"
#include "asm/power_interface.h"
#include "gpadc.h"
#include "linein.h"
#include "linein_dev.h"
#include "gpio.h"
#include "asm/sdmmc.h"
#include "app_msg.h"
#include "linein_file.h"
#include "gpio_config.h"


#if TCFG_APP_LINEIN_EN

#define LINEIN_DETECT_CNT   					6//滤波计算

// linein检测频率
#define LINEIN_DETECT_FREQUENCY_500MS			500
#define LINEIN_DETECT_FREQUENCY_30MS			30

struct linein_dev_opr {
    struct linein_dev_data *dev;
    u8 cnt;//滤波计算
    u8 status;//当前状态
    u16 timer;//定时器句柄
    u8 online; //是否在线
    u8 active; //进入sniff的判断标志
    u8 init; //初始化完成标志
    u16 detect_frequency; //检测频率 参考LINEIN_DETECT_FREQUENCY_500MS
};
static struct linein_dev_opr linein_dev_hdl = {0};
#define __this 	(&linein_dev_hdl)

/**
 * @brief 获取linein是否在线
 * @param 1:在线 0：不在线
 */
u8 linein_is_online(void)
{
#if ((defined TCFG_LINEIN_DETECT_ENABLE) && (TCFG_LINEIN_DETECT_ENABLE == 0))
    return 1;
#else
    return __this->online;
#endif//TCFG_LINEIN_DETECT_ENABLE

}

/**
 * @brief 设置linein是否在线
 * @return  1:在线 0：不在线
 */
static void linein_set_online(u8 online)
{
    __this->online = !!online;
}

/**
 * @brief   发布设备上下线消息
 * @param    上下线消息
 */
static void linein_event_notify(u8 arg)
{
    int msg[2];

    msg[0] = (u32)DEVICE_EVENT_FROM_LINEIN;
    if (arg == DEVICE_EVENT_IN) {
        msg[1] = DEVICE_EVENT_IN;
    } else if (arg == DEVICE_EVENT_OUT) {
        msg[1] = DEVICE_EVENT_OUT;
    } else {
        return ;
    }

#if (TCFG_DEV_MANAGER_ENABLE)
    os_taskq_post_type("dev_mg", MSG_FROM_DEVICE, 2, msg);
#else
    os_taskq_post_type("app_core", MSG_FROM_DEVICE, 2, msg);
#endif
}

/**
 * @brief   检测前使能io
 * @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
 * (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
 */
static void linein_io_start(void)
{
    /* printf("<<<linein_io_start\n"); */
    struct linein_dev_data *linein_dev = (struct linein_dev_data *)__this->dev;
    if (__this->init) {
        return ;
    }
    if (linein_dev == NULL) {
        return;
    }
    __this->init = 1;

    if (linein_dev->down) {
        gpio_set_mode(IO_PORT_SPILT(linein_dev->port), PORT_INPUT_PULLDOWN_10K);
    } else if (linein_dev->up) {
        gpio_set_mode(IO_PORT_SPILT(linein_dev->port), PORT_INPUT_PULLUP_10K);
    } else {
        gpio_set_mode(IO_PORT_SPILT(linein_dev->port), PORT_INPUT_FLOATING);
    }
    if (linein_dev->ad_channel == (u32)NO_CONFIG_PORT) {
    } else {
        gpio_set_function(IO_PORT_SPILT(linein_dev->port), PORT_FUNC_GPADC);
    }
}

/**
 * @brief   检测完成关闭使能io
 * @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
 * (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
 */
static void linein_io_stop(void)
{
    /* printf("<<<linein_io_stop\n"); */
    struct linein_dev_data *linein_dev = (struct linein_dev_data *)__this->dev;
    if (!__this->init) {
        return ;
    }
    __this->init = 0;
    gpio_set_mode(IO_PORT_SPILT(linein_dev->port), PORT_HIGHZ);
    gpio_set_drive_strength(linein_dev->port / 16, BIT(linein_dev->port % 16), PORT_DRIVE_STRENGT_2p4mA);
}

/**
 * @brief   检测是否在线
 * @param    驱动句柄
 * @return    1:有设备插入 0：没有设备插入
 * @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
 * (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
 */
static int linein_sample_detect(void *arg)
{
    struct linein_dev_data *linein_dev = (struct linein_dev_data *)arg;
    u8 cur_status;

    if (linein_dev->ad_channel == (u32)NO_CONFIG_PORT) {
        linein_io_start();
        cur_status = gpio_read(linein_dev->port) ? false : true;
        linein_io_stop();
        if (!linein_dev->up) {
            cur_status	= !cur_status;
        }
    } else {
        cur_status = adc_get_voltage(linein_dev->ad_channel) > linein_dev->ad_vol * 10 ? false : true;
        printf("<%d> <%d> \n", adc_get_voltage(linein_dev->ad_channel), cur_status);
    }
    /* putchar('A' + cur_status); */
    return cur_status;
}

/**
 * @brief   sd cmd 复用检测是否在线
 * @param    驱动句柄
 * @return    1:有设备插入 0：没有设备插入
 * @note    检测驱动检测前使能io ，检测完成后设为高阻 可以节约功耗
 * (io检测、sd复用ad检测动态使用，单独ad检测不动态修改)
 */
static int linein_sample_mult_sd(void *arg)
{
    struct linein_dev_data *linein_dev = (struct linein_dev_data *)arg;
    linein_io_start();
    u8 cur_status;
    if (linein_dev->ad_channel == (u32)NO_CONFIG_PORT) {
        cur_status = gpio_read(linein_dev->port) ? false : true;
    } else {
        u32 ad_value = adc_get_value_blocking(linein_dev->ad_channel);
        cur_status = ad_value > linein_dev->ad_vol ? false : true;
    }

    /* putchar('A'+cur_status); */
    /* putchar(cur_status); */
    linein_io_stop();
    return cur_status;
}

/**
 * @brief 注册的定时器回调检测函数
 * @param 驱动句柄
 * @note 定时进行检测
 */
static void linein_detect(void *arg)
{
    int cur_status;
    extern u8 bt_trim_status_get();
    if (bt_trim_status_get()) {
        sys_timer_modify(__this->timer, 50);//bt trim 延后检测
        return;
    }

#if ((TCFG_LINEIN_MULTIPLEX_WITH_SD == ENABLE))
    if (sd_io_suspend(TCFG_LINEIN_SD_PORT, 0) == 0) {//判断sd 看是否空闲
        cur_status = linein_sample_mult_sd(arg);
        sd_io_resume(TCFG_LINEIN_SD_PORT, 0);//使用完，回复sd
    } else {
        return;
    }
#else
    if (__this->detect_frequency == LINEIN_DETECT_FREQUENCY_500MS) {
        __this->detect_frequency = LINEIN_DETECT_FREQUENCY_30MS;
        sys_timer_modify(__this->timer, LINEIN_DETECT_FREQUENCY_30MS);
        return ;
    }
    cur_status = linein_sample_detect(arg);
    if (!__this->active) {
        __this->detect_frequency = LINEIN_DETECT_FREQUENCY_500MS;
        sys_timer_modify(__this->timer, LINEIN_DETECT_FREQUENCY_500MS);
    }
#endif

    if (cur_status != __this->status) {
        __this->status = cur_status;
        __this->cnt = 0;
        __this->active = 1;
    } else {
        __this->cnt ++;
    }

    if (__this->cnt < LINEIN_DETECT_CNT) {//滤波计算
        return;
    }

    __this->active = 0;//检测一次完成

    /* putchar(cur_status); */

    if ((linein_is_online()) && (!__this->status)) {
        linein_set_online(false);
        linein_event_notify(DEVICE_EVENT_OUT);//发布下线消息
    } else if ((!linein_is_online()) && (__this->status)) {
        linein_set_online(true);
        linein_event_notify(DEVICE_EVENT_IN);//发布上线消息
    }
    return;
}

static int linein_driver_init(const struct dev_node *node,  void *arg)
{
    struct linein_dev_data *linein_dev = (struct linein_dev_data *)arg;

    if (!linein_dev->enable) {
        linein_set_online(true);
        return 0;
    }
#if TCFG_LINEIN_DETECT_ENABLE == 0
    linein_set_online(true);//没有配置detcct 默认在线
    return 0;
#else
    linein_dev->port    =  TCFG_LINEIN_DETECT_IO;
    linein_dev->up      =  TCFG_LINEIN_DETECT_PULL_UP_ENABLE;
    linein_dev->down    =  TCFG_LINEIN_DETECT_PULL_DOWN_ENABLE;
#if TCFG_LINEIN_AD_DETECT_ENABLE
    linein_dev->ad_channel = adc_io2ch(linein_dev->port);
    linein_dev->ad_vol =  TCFG_LINEIN_AD_DETECT_VALUE;
#endif

    if (linein_dev->port == (u8)NO_CONFIG_PORT) {
        linein_set_online(true);//配置的io 不在范围 ，默认在线
        return 0;
    }
    linein_set_online(false);
    __this->dev = linein_dev;

#if (!(TCFG_LINEIN_MULTIPLEX_WITH_SD))//复用情况和io检测仅在使用的时候配置io
    if (linein_dev->ad_channel != (u32)NO_CONFIG_PORT) {
        linein_io_start();//初始化io
        adc_add_sample_ch(linein_dev->ad_channel);
    }
#endif

    __this->timer = sys_timer_add(arg, linein_detect, LINEIN_DETECT_FREQUENCY_500MS);
#endif //TCFG_LINEIN_DETECT_ENABLE

    return 0;
}

const struct device_operations linein_dev_ops = {
    .init = linein_driver_init,
};

/**
 * @brief   注册的定功耗检测函数
 * @note   用于防止检测一次未完成进入了低功耗
 */
static u8 linein_dev_idle_query(void)
{
    return !__this->active;
}

REGISTER_LP_TARGET(linein_dev_lp_target) = {
    .name = "linein_dev",
    .is_idle = linein_dev_idle_query,
};

#endif
