#include "app_config.h"
#include "utils/syscfg_id.h"
#include "gpio_config.h"
#include "key/iokey.h"
#include "key_driver.h"
#include "iokey_config.h"

#if TCFG_IOKEY_ENABLE

#ifndef CONFIG_IOKEY_MAX_NUM
#define CONFIG_IOKEY_MAX_NUM 5
#endif



static struct iokey_port iokey_ports[CONFIG_IOKEY_MAX_NUM];
static struct iokey_platform_data platform_data;

static const u16 key_uuid_table[][2] = {
    { 0xaefa, KEY_POWER },
    { 0x368c, KEY_NEXT },
    { 0x842a, KEY_PREV },
    { 0x0bb0, KEY_SLIDER },
    { 0xd212, KEY_MODE },
    { 0x6a23, KEY_PLAY },
};

u8 uuid2keyValue(u16 uuid)
{
    for (int i = 0; i < ARRAY_SIZE(key_uuid_table); i++) {
        if (key_uuid_table[i][0] == uuid) {
            return key_uuid_table[i][1];
        }
    }

    return 0xff;
}

__INITCALL_BANK_CODE
const struct iokey_platform_data *get_iokey_platform_data()
{
    const struct iokey_info *info = g_iokey_info;

    if (platform_data.enable) {
        return &platform_data;
    }

    platform_data.num       = ARRAY_SIZE(g_iokey_info);
    platform_data.port      = iokey_ports;
    platform_data.enable    = 1;

    for (int i = 0; i < platform_data.num; i++) {
        if (info[i].detect == 0) {
            iokey_ports[i].connect_way = ONE_PORT_TO_LOW;
        } else {
            iokey_ports[i].connect_way = ONE_PORT_TO_HIGH;
        }
        iokey_ports[i].key_type.one_io.port = info[i].key_io;
        iokey_ports[i].key_value = info[i].key_value;
        if (info[i].long_press_reset_enable) {
            platform_data.long_press_enable = 1;
            platform_data.long_press_time = info[i].long_press_reset_time;
            platform_data.long_press_port = info[i].key_io;
            platform_data.long_press_level = info[i].detect;
        }
        printf("iokey:%d,prot:%d,value:%d,c_way:%d long_press_en:%d time:%ds\n", i,
               iokey_ports[i].key_type.one_io.port,
               iokey_ports[i].key_value,
               iokey_ports[i].connect_way,
               info[i].long_press_reset_enable,
               info[i].long_press_reset_time);
    }

    return &platform_data;
}

bool is_iokey_press_down()
{
#if TCFG_IOKEY_ENABLE
    if (platform_data.enable == 0) {
        return false;
    }
    for (int i = 0; i < platform_data.num; i++) {
        if (iokey_ports[i].key_value == KEY_POWER) {
            int value = gpio_read(iokey_ports[i].key_type.one_io.port);
            if (iokey_ports[i].connect_way == ONE_PORT_TO_LOW) {
                return value == 0 ? true : false;
            }
            return value == 1 ? true : false;
        }
    }
#endif
    return false;
}

int get_iokey_power_io()
{
#if TCFG_IOKEY_ENABLE
    if (platform_data.enable) {
        return iokey_ports[0].key_type.one_io.port;
    }
#endif
    return -1;
}

#endif

