#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".tuya_ble_port_JL.data.bss")
#pragma data_seg(".tuya_ble_port_JL.data")
#pragma const_seg(".tuya_ble_port_JL.text.const")
#pragma code_seg(".tuya_ble_port_JL.text")
#endif
/**
 * \file tuya_ble_port.c
 *
 * \brief
 */
/*
 *  Copyright (C) 2014-2019, Tuya Inc., All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of tuya ble sdk
 */

#include "tuya_ble_port.h"
#include "tuya_ble_port_JL.h"
#include "tuya_ble_main.h"
#include "tuya_ble_log.h"
#include "cpu.h"
#include "timer.h"
#include "os/os_api.h"
#include "system/includes.h"
#include "update.h"
#include "update_loader_download.h"
#include "aes.h"
#include "md5.h"
#include "tuya_ble_data_handler.h"
#include "circular_buf.h"
#include "btstack/le/le_user.h"
#include "app_ble_spp_api.h"

#undef __TUYA_BLE_WEAK
#define __TUYA_BLE_WEAK

bool external_set_adv_data(u8 *data, u8 len);
bool external_set_rsp_data(u8 *data, u8 len);

extern bt_state_info_t bt_state_info;
extern int ble_gatt_server_adv_enable(u32 en);
extern uint32_t m_cb_queue_table[TUYA_BLE_MAX_CALLBACKS];
extern void tuya_app_cb_handler(tuya_ble_cb_evt_param_t *event);
extern const char *bt_get_local_name();
extern void get_random_number(u8 *ptr, u8 len);
extern const u8 *bt_get_mac_addr();
extern void tuya_tws_bind_info_sync();
extern uint32_t tuya_ble_storage_save_sys_settings(void);
extern void tuya_set_adv_disable();
extern void tuya_set_adv_enable();
extern void tuya_set_adv_data(u8 *data, u8 length);
extern void tuya_set_rsp_data(u8 *data, u8 length);
extern void *tuya_ble_hdl;

//涂鸦会调我们的接口开task，这个结构体用来保存一下信息
struct ble_task_param ble_task = {
    .ble_task_name = NULL,
};

static void (*app_set_adv_data)(u8 *adv_data, u8 adv_len) = NULL;
static void (*app_set_rsp_data)(u8 *rsp_data, u8 rsp_len) = NULL;
void tuya_app_set_adv_data_register(void (*handler)(u8 *adv_data, u8 adv_len))
{
    app_set_adv_data = handler;
}
void tuya_app_set_rsp_data_register(void (*handler)(u8 *rsp_data, u8 rsp_len))
{
    app_set_rsp_data = handler;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_gap_advertising_adv_data_update(uint8_t const *p_ad_data, uint8_t ad_len)
{
    tuya_set_adv_disable();
    tuya_set_adv_data((u8 *)p_ad_data, ad_len);
    tuya_set_adv_enable();
    return TUYA_BLE_SUCCESS;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_gap_advertising_scan_rsp_data_update(uint8_t const *p_sr_data, uint8_t sr_len)
{
    tuya_set_adv_disable();
    tuya_set_rsp_data((u8 *)p_sr_data, sr_len);
    tuya_set_adv_enable();
    return TUYA_BLE_SUCCESS;
}

static struct ble_server_operation_t *ble_operation = NULL;
void tuya_ble_operation_register(struct ble_server_operation_t *operation)
{
    ble_operation = operation;
}

extern tuya_ble_parameters_settings_t tuya_ble_current_para;
void tuya_one_click_connect_init()
{
    bt_state_info.status = 0;
    bt_state_info.ability = 0;
    bt_state_info.dev_info = 0;
    bt_state_info.connect_info = 0;
    bt_state_info.name_len = strlen(bt_get_local_name());
    memcpy(bt_state_info.name, bt_get_local_name(), strlen(bt_get_local_name()));
    memcpy(bt_state_info.mac, bt_get_mac_addr(), 6);
}

void tuya_one_click_connect_state_set(uint8_t state)
{
    bt_state_info.dev_info = state;
    bt_state_info.connect_info = state ? 2 : 0;
}

void tuya_one_click_connect_info_update()
{
    bt_state_info.name_len = strlen(bt_get_local_name());
    memcpy(bt_state_info.name, bt_get_local_name(), strlen(bt_get_local_name()));
    memcpy(bt_state_info.mac, (void *)bt_get_mac_addr(), 6);
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_gap_disconnect(void)
{
    /* struct ble_server_operation_t *ble_operation = NULL; */
    /* TUYA_BLE_LOG_DEBUG("%s\n", __func__); */
    /* tuya_ble_get_server_operation_table(&ble_operation); */
    if (ble_operation && ble_operation->disconnect) {
        ble_operation->disconnect(NULL);
        return TUYA_BLE_SUCCESS;
    }

    return TUYA_BLE_ERR_INVALID_STATE;
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_gatt_send_data(const uint8_t *p_data, uint16_t len)
{
    /* struct ble_server_operation_t *ble_operation = NULL; */
    /* TUYA_BLE_LOG_DEBUG("%s\n", __func__); */
    /* tuya_ble_get_server_operation_table(&ble_operation); */
    if (ble_operation && ble_operation->send_data) {
        int ret = ble_operation->send_data(NULL, (void *)p_data, len);
        TUYA_BLE_LOG_DEBUG("%d\n", ret);
        return ret ? TUYA_BLE_ERR_UNKNOWN : TUYA_BLE_SUCCESS;
    }
    return TUYA_BLE_ERR_INVALID_STATE;
}

struct timer_param {
    tuya_ble_timer_mode mode;
    tuya_ble_timer_handler_t timeout_handler;
    uint32_t timeout_value_ms;
};

static struct timer_param ble_timer;

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_timer_create(void **p_timer_id, uint32_t timeout_value_ms, tuya_ble_timer_mode mode, tuya_ble_timer_handler_t timeout_handler)
{
    *p_timer_id = malloc(2);
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (*p_timer_id) {
        //我们的timer不需要create，所以这里只需要保存一下信息
        *(u16 *)(*p_timer_id) = 0;
        ble_timer.mode = mode;
        ble_timer.timeout_handler = timeout_handler;
        ble_timer.timeout_value_ms = timeout_value_ms;
        return TUYA_BLE_SUCCESS;
    }

    return TUYA_BLE_ERR_NO_MEM;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_timer_delete(void *timer_id)
{
    u16 id = *(u16 *)timer_id;
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (id) {
        if (ble_timer.mode == TUYA_BLE_TIMER_SINGLE_SHOT) {
            sys_timeout_del(id);
        } else {
            sys_timer_del(id);
        }
    }
    free(timer_id);
    return TUYA_BLE_SUCCESS;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_timer_start(void *timer_id)
{
    u16 id = *(u16 *)timer_id;
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (id) {
        if (ble_timer.mode == TUYA_BLE_TIMER_SINGLE_SHOT) {
            sys_timeout_del(id);
        } else {
            sys_timer_del(id);
        }
    }
    u16(*sys_timer_create)(void *priv, void (*func)(void *priv), u32 msec);
    sys_timer_create = (ble_timer.mode == TUYA_BLE_TIMER_SINGLE_SHOT) ? sys_timeout_add : sys_timer_add;
    *(u16 *)timer_id = sys_timer_create(NULL, ble_timer.timeout_handler, ble_timer.timeout_value_ms);
    return TUYA_BLE_SUCCESS;
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_timer_restart(void *timer_id, uint32_t timeout_value_ms)
{
    u16 id = *(u16 *)timer_id;
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    sys_timer_modify(id, timeout_value_ms);
    sys_timer_re_run(id);
    return TUYA_BLE_SUCCESS;
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_timer_stop(void *timer_id)
{
    return TUYA_BLE_SUCCESS;
}


/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK void tuya_ble_device_delay_ms(uint32_t ms)
{
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (ms < 10) {
        ms = 10;
    }
    os_time_dly(ms / 10);
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK void tuya_ble_device_delay_us(uint32_t us)
{
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    //delay_us(us);
    while (us--) {
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
        asm("nop");
    }
}


/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_rand_generator(uint8_t *p_buf, uint8_t len)
{
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    get_random_number(p_buf, len);
    return TUYA_BLE_SUCCESS;
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_device_reset(void)
{
    return TUYA_BLE_SUCCESS;
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_gap_addr_get(tuya_ble_gap_addr_t *p_addr)
{
    u8 ret = 0;
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (p_addr->addr_type == TUYA_BLE_ADDRESS_TYPE_PUBLIC) {
        ret = le_controller_get_mac(p_addr->addr);
    } else if (p_addr->addr_type == TUYA_BLE_ADDRESS_TYPE_RANDOM) {
        ret = le_controller_get_random_mac(p_addr->addr);
    }
    return ret ? TUYA_BLE_ERR_NOT_FOUND : TUYA_BLE_SUCCESS;
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_gap_addr_set(tuya_ble_gap_addr_t *p_addr)
{
    u8 ret = 0;
    tuya_set_adv_disable();
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    TUYA_BLE_LOG_DEBUG("tuya_ble_gap_addr_set=%d\n", p_addr->addr_type);
    TUYA_BLE_LOG_HEXDUMP_DEBUG("addr", p_addr->addr, 6);
    app_ble_adv_address_type_set(tuya_ble_hdl, p_addr->addr_type);
    if (p_addr->addr_type == TUYA_BLE_ADDRESS_TYPE_PUBLIC) {
        ret = app_ble_set_mac_addr(tuya_ble_hdl, (void *)p_addr->addr);
    } else if (p_addr->addr_type == TUYA_BLE_ADDRESS_TYPE_RANDOM) {
        ret = app_ble_set_mac_addr(tuya_ble_hdl, (void *)p_addr->addr);
    }
    tuya_set_adv_enable();
    return ret ? TUYA_BLE_ERR_INVALID_ADDR : TUYA_BLE_SUCCESS;
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK void tuya_ble_device_enter_critical(void)
{
    OS_ENTER_CRITICAL();
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK void tuya_ble_device_exit_critical(void)
{
    OS_EXIT_CRITICAL();
}

/*
*@brief
*@param
*
*@note
*
* */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_rtc_get_timestamp(uint32_t *timestamp, int32_t *timezone)
{
    //目前用不到
    *timestamp = 0;
    *timezone = 0;
    return TUYA_BLE_SUCCESS;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_rtc_set_timestamp(uint32_t timestamp, int32_t timezone)
{
    return TUYA_BLE_SUCCESS;
}

typedef enum _FLASH_ERASER {
    CHIP_ERASER,
    BLOCK_ERASER,//4k
    SECTOR_ERASER,//64k
} FLASH_ERASER;


//
extern bool sfc_erase(FLASH_ERASER cmd, u32 addr);
extern u32 sdfile_cpu_addr2flash_addr(u32 offset);

#define USER_FILE_NAME       SDFILE_APP_ROOT_PATH"USERIF"
#define NV_MODE_FILE         0 //固定放在一个指定的区域，一般情况下不会被意外擦除，flash比较大的方案建议用这个
#define NV_MODE_VM           1 //用VM存，被意外擦除的概率比较高，比如升级的时候，flash空间不够的时候用这个
#define TUYA_BLE_NV_MODE     NV_MODE_VM
//使用文件的的方式保存数据，需要在ini文件添加下面的配置
/*
USERIF_ADR=AUTO;
USERIF_LEN=0x4000;
USERIF_OPT=1;
*/

FILE *code_fp = NULL;

struct vfs_attr code_attr = {0};
struct vfs_attr attr;

typedef struct __tuya_addr_to_vfs {
    u32 tuya_start_addr;//0
    u32 vfs_satrt_addr;//1
} tuya_addr_to_vfs;

static tuya_addr_to_vfs addr_sw;
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_nv_init(void)
{
#if (TUYA_BLE_NV_MODE == NV_MODE_FILE)
    if (code_fp) {
        return TUYA_BLE_SUCCESS;
    }
    code_fp = fopen(USER_FILE_NAME, "r+w");
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (code_fp == NULL) {
        TUYA_BLE_LOG_DEBUG("file open err!!!");
        return TUYA_BLE_ERR_RESOURCES;
    }

    fget_attrs(code_fp, &attr);
    if (attr.fsize < 2048) {
        TUYA_BLE_LOG_DEBUG("file size err!!!");
    }
    addr_sw.tuya_start_addr = 0;
    addr_sw.vfs_satrt_addr = attr.sclust;
    TUYA_BLE_LOG_DEBUG("addr %x,%x\n", attr.sclust, addr_sw.tuya_start_addr);
    TUYA_BLE_LOG_DEBUG("tuya_ble_nv_initok\n");
#endif

    return TUYA_BLE_SUCCESS;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_nv_erase(uint32_t addr, uint32_t size)
{
#if (TUYA_BLE_NV_MODE == NV_MODE_FILE)
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (code_fp == NULL) {
        TUYA_BLE_LOG_DEBUG("file ptr err!!!");
        return TUYA_BLE_ERR_RESOURCES;
    }
    u32 flash_addr = sdfile_cpu_addr2flash_addr(addr_sw.vfs_satrt_addr + addr);
    sfc_erase(SECTOR_ERASER, flash_addr);// 4k
#endif
    return TUYA_BLE_SUCCESS;
}

static uint8_t (*tuya_get_vm_id)(u32 addr) = NULL;
void tuya_get_vm_id_register(uint8_t (*handler)(u32 addr))
{
    tuya_get_vm_id = handler;
}

static uint8_t tuya_get_vm_id_func(u32 addr)
{
    if (tuya_get_vm_id) {
        return tuya_get_vm_id(addr);
    }
    printf("tuya_get_vm_id no regitster");
    return 0;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_nv_write(uint32_t addr, const uint8_t *p_data, uint32_t size)
{
#if (TUYA_BLE_NV_MODE == NV_MODE_FILE)
    FILE *write_fp = code_fp;
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (code_fp == NULL) {
        TUYA_BLE_LOG_DEBUG("file ptr err!!!");
        return TUYA_BLE_ERR_RESOURCES;
    }
    int ret = 0;
    ret = fseek(write_fp, addr, SEEK_SET);
    TUYA_BLE_LOG_DEBUG("fseek=%x,addr_star=%d,size=%d\n", addr, ret, size);
    int r = fwrite(write_fp, p_data, size);//更新数据到自定义区，写数据前需要擦除，确保处于FF状态才能成功写入
    if (r != size) {
        TUYA_BLE_LOG_DEBUG("write file error code %x", r);
    }
#else
    u8 index;
    switch (addr) {
    case TUYA_BLE_AUTH_FLASH_ADDR:
        index = tuya_get_vm_id_func(addr);
        break;
    case TUYA_BLE_AUTH_FLASH_BACKUP_ADDR:
        index = tuya_get_vm_id_func(addr);
        break;
    case TUYA_BLE_SYS_FLASH_ADDR:
        index = tuya_get_vm_id_func(addr);
        break;
    case TUYA_BLE_SYS_FLASH_BACKUP_ADDR:
        index = tuya_get_vm_id_func(addr);
        break;

    default:
        return TUYA_BLE_ERR_INVALID_ADDR;
    }
    if (syscfg_write(index, p_data, size) != size) {
        printf("tuya vm_write error");
        return TUYA_BLE_ERR_INVALID_ADDR;
    }
#endif
    //发消息，用来给TWS同步app配对数据
    if (addr == TUYA_BLE_AUTH_FLASH_ADDR) {
        if (ble_task.ble_task_name) {
            os_taskq_post_msg(ble_task.ble_task_name, 1, BLE_TASK_MSG_INFO_SYNC_AUTH);
        }
    } else if (addr == TUYA_BLE_SYS_FLASH_ADDR) {
        if (ble_task.ble_task_name) {
            os_taskq_post_msg(ble_task.ble_task_name, 1, BLE_TASK_MSG_INFO_SYNC_SYS);
        }
    }
    return TUYA_BLE_SUCCESS;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_nv_read(uint32_t addr, uint8_t *p_data, uint32_t size)
{
#if (TUYA_BLE_NV_MODE == NV_MODE_FILE)
    FILE *read_fp = code_fp;
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (code_fp == NULL) {
        TUYA_BLE_LOG_DEBUG("file ptr err!!!");
        return TUYA_BLE_ERR_RESOURCES;
    }
    fseek(read_fp, addr, SEEK_SET);
    fread(read_fp, p_data, size);//文件模式读取自定义数据区
    TUYA_BLE_LOG_HEXDUMP_DEBUG("file read:", p_data, size);
#else
    u8 index;
    switch (addr) {
    case TUYA_BLE_AUTH_FLASH_ADDR:
        index = tuya_get_vm_id_func(addr);
        break;
    case TUYA_BLE_AUTH_FLASH_BACKUP_ADDR:
        index = tuya_get_vm_id_func(addr);
        break;
    case TUYA_BLE_SYS_FLASH_ADDR:
        index = tuya_get_vm_id_func(addr);
        break;
    case TUYA_BLE_SYS_FLASH_BACKUP_ADDR:
        index = tuya_get_vm_id_func(addr);
        break;

    default:
        return TUYA_BLE_ERR_INVALID_ADDR;
    }
    syscfg_read(index, p_data, size);
#endif
    return TUYA_BLE_SUCCESS;
}

bool uart_dev_test_init_api(void);
u16 uart_dev_test_send_api(void *data, u16 len);
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_common_uart_init(void)
{
    /* bool ret = uart_dev_test_init_api(); */
    /* return ret ? TUYA_BLE_SUCCESS : TUYA_BLE_ERR_BUSY; */
    return TUYA_BLE_SUCCESS;
}

extern tuya_ble_status_t tuya_ble_gatt_receive_data(uint8_t *p_data, uint16_t len);
u8 JL_tuya_ble_gatt_receive_data(u8 *p_data, u16 len)
{
    return tuya_ble_gatt_receive_data(p_data, len);
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK tuya_ble_status_t tuya_ble_common_uart_send_data(const uint8_t *p_data, uint16_t len)
{
    /* u16 wlen = uart_dev_test_send_api(p_data, len); */
    /* return wlen == len ? TUYA_BLE_SUCCESS : TUYA_BLE_ERR_BUSY; */
    return TUYA_BLE_SUCCESS;
}


/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK bool tuya_ble_os_task_create(void **pp_handle, char *p_name, void (*p_routine)(void *), void *p_param, uint16_t stack_size, uint16_t priority)
{
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);

    int ret = task_create(p_routine, p_param, p_name);
    if (ret == 0) {
        ble_task.ble_task_name = p_name;
        os_sem_create(&ble_task.ble_sem, 0);
        TUYA_BLE_LOG_DEBUG("%s succ\n", __func__);
    }
    return ret ? false : true;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK bool tuya_ble_os_task_delete(void *p_handle)
{
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (ble_task.ble_task_name) {
        os_task_del(ble_task.ble_task_name);
        os_sem_del(&ble_task.ble_sem, 0);
        ble_task.ble_task_name = NULL;
    }
    return true;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK bool tuya_ble_os_task_suspend(void *p_handle)
{
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (ble_task.ble_task_name) {
        os_sem_pend(&ble_task.ble_sem, 0);
    }
    return true;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK bool tuya_ble_os_task_resume(void *p_handle)
{
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (ble_task.ble_task_name) {
        os_sem_set(&ble_task.ble_sem, 1);
        os_sem_post(&ble_task.ble_sem);
    }
    return true;
}

struct ble_msg_queue {
    cbuffer_t queue_hdl;
    u8 *queue;
    u8 enable;
    u32 msg_size;
};

struct ble_msg_queue ble_queue;

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK bool tuya_ble_os_msg_queue_create(void **pp_handle, uint32_t msg_num, uint32_t msg_size)
{
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    memset(&ble_queue, 0x00, sizeof(ble_queue));
    ble_queue.queue = malloc(msg_size * msg_num);
    if (ble_queue.queue) {
        *pp_handle = (void *)ble_queue.queue;
        ble_queue.msg_size = msg_size;
        cbuf_init(&ble_queue.queue_hdl, ble_queue.queue, msg_size * msg_num);
        ble_queue.enable = 1;
        return true;
    }
    return false;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK bool tuya_ble_os_msg_queue_delete(void *p_handle)
{
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    if (ble_queue.enable) {
        ble_queue.enable = 0;
        free(ble_queue.queue);
        ble_queue.queue = NULL;
    }
    return true;
}
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK bool tuya_ble_os_msg_queue_peek(void *p_handle, uint32_t *p_msg_num)
{
    return true;
}


/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK bool tuya_ble_os_msg_queue_send(void *p_handle, void *p_msg, uint32_t wait_ms)
{
    if (ble_queue.enable) {
        if ((uint32_t)p_handle == m_cb_queue_table[0]) {
            int argv[3];
            argv[0] = (int)tuya_app_cb_handler;
            argv[1] = 1;
            argv[2] = (int)p_msg;
            os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
        } else {
            //TUYA_BLE_LOG_DEBUG("%s\n", __func__);
            cbuf_write(&ble_queue.queue_hdl, p_msg, ble_queue.msg_size);
            os_taskq_post_msg(ble_task.ble_task_name, 2, BLE_TASK_MSG_MSG_COMES, (u32)p_handle);
        }

    }
    return true;
}

static bool tuya_ble_os_msg_queue_recv_sub_deal(void *p_msg)
{
    tuya_ble_evt_param_t *evt = (tuya_ble_evt_param_t *)p_msg;
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    bool ret = false;
    switch (evt->hdr.event) {
    case TUYA_BLE_EVT_DATA_PASSTHROUGH:
        break;

    default:
        return true;
    }

    return ret;
}

void (*tuya_cb_handler)(tuya_ble_cb_evt_param_t *event) = NULL;
void tuya_event_to_earphone(u8 msg, u8 sub_msg, u32 value);
/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK bool tuya_ble_os_msg_queue_recv(void *p_handle, void *p_msg, uint32_t wait_ms)
{
    int ret;
    int msg[16];
    ret = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
    if (ret != OS_TASKQ) {
        return false;
    }
    if (msg[0] != Q_MSG) {
        return false;
    }

    //TUYA_BLE_LOG_DEBUG("tuya_ble_os_msg_queue_recv\n");

    bool b_ret = false;

    switch (msg[1]) {
    case BLE_TASK_MSG_MSG_COMES:
        cbuf_read(&ble_queue.queue_hdl, p_msg, ble_queue.msg_size);
        if ((u32)msg[2] == (u32)ble_queue.queue) {
            b_ret = tuya_ble_os_msg_queue_recv_sub_deal(p_msg);
        } else {
            printf("error MSG_COMES");
        }
        break;

    case BLE_TASK_MSG_INFO_SYNC_AUTH:
        //设备信息变更（比如mac变更，但是正常使用不会跑这里）要同步tws
        // tuya_event_to_earphone(TUYA_BLE_TO_EARPHONE_SYNC_AUTH_MSG, 0, 0);
        break;

    case BLE_TASK_MSG_INFO_SYNC_SYS:
        printf("BLE_TASK_MSG_INFO_SYNC_SYS");
        tuya_tws_bind_info_sync();
        //app配对信息变更要同步tws
        //tuya_event_to_earphone(TUYA_BLE_TO_EARPHONE_SYNC_SYS_MSG, 0, 0);
        break;

    case BLE_TASK_PAIR_INFO_SYNC_SYS:
        printf("tws_pair_info_store");
        tuya_ble_storage_save_sys_settings();
        tuya_ble_adv_change();
        break;
    default:
        break;
    }

    //TUYA_BLE_LOG_DEBUG("deal ret %d\n", b_ret);

    return b_ret;
}

uint8_t ble_characteristic_value[64];
uint16_t ble_characteristic_value_len;

uint8_t *tuya_get_ble_characteristic_value(uint16_t *len)
{
    *len = ble_characteristic_value_len;
    return ble_characteristic_value;
}

tuya_ble_status_t tuya_ble_device_info_characteristic_value_update(uint8_t const *p_data, uint8_t data_len)
{
    printf("tuya_characteristic_value_update, len:%d", data_len);
    put_buf(p_data, data_len);
    ble_characteristic_value_len = data_len;
    memcpy(ble_characteristic_value, p_data, ble_characteristic_value_len);

    return 0;
}

/*
 *@brief
 *@param
 *
 *@note
 *
 * */
__TUYA_BLE_WEAK bool tuya_ble_event_queue_send_port(tuya_ble_evt_param_t *evt, uint32_t wait_ms)
{
    TUYA_BLE_LOG_DEBUG("%s\n", __func__);
    return tuya_ble_os_msg_queue_send((void *)ble_queue.queue, evt, wait_ms);
}



/**
    * @brief  128 bit AES ECB encryption on speicified plaintext and keys
    * @param  input    specifed plain text to be encypted
    * @param  input_len    byte length of the data to be descrypted, must be multiples of 16
    * @param  key          keys to encrypt the plaintext
    * @param  output    output buffer to store encrypted data
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    */
__TUYA_BLE_WEAK bool tuya_ble_aes128_ecb_encrypt(uint8_t *key, uint8_t *input, uint16_t input_len, uint8_t *output)
{
    uint16_t length;
    mbedtls_aes_context aes_ctx;
    //
    if (input_len % 16) {
        return false;
    }

    length = input_len;

    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);

    while (length > 0) {
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, input, output);
        input  += 16;
        output += 16;
        length -= 16;
    }

    mbedtls_aes_free(&aes_ctx);

    return true;
}

/**
    * @brief  128 bit AES ECB decryption on speicified encrypted data and keys
    * @param  input    specifed encypted data to be decypted
    * @param  input_len    byte length of the data to be descrypted, must be multiples of 16
    * @param  key          keys to decrypt the data
    * @param  output    output buffer to store plain data
    * @return decryption results
    * @retval true      successful
    * @retval false     fail
    * @note   least significant octet of encrypted data corresponds to encypted[0]
*/
__TUYA_BLE_WEAK bool tuya_ble_aes128_ecb_decrypt(uint8_t *key, uint8_t *input, uint16_t input_len, uint8_t *output)
{
    uint16_t length;
    mbedtls_aes_context aes_ctx;
    //
    if (input_len % 16) {
        return false;
    }

    length = input_len;

    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_dec(&aes_ctx, key, 128);

    while (length > 0) {
        mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, input, output);
        input  += 16;
        output += 16;
        length -= 16;
    }

    mbedtls_aes_free(&aes_ctx);
    return true;
}
/**
    * @brief  128 bit AES CBC encryption on speicified plaintext and keys
    * @param  input    specifed plain text to be encypted
    * @param  key          keys to encrypt the plaintext
    * @param  output    output buffer to store encrypted data
    * @param  iv         initialization vector (IV) for CBC mode
    * @param  input_len    byte length of the data to be descrypted, must be multiples of 16
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    */
__TUYA_BLE_WEAK bool tuya_ble_aes128_cbc_encrypt(uint8_t *key, uint8_t *iv, uint8_t *input, uint16_t input_len, uint8_t *output)
{
    mbedtls_aes_context aes_ctx;

    if (input_len % 16) {
        return false;
    }

    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);

    mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, input_len, iv, input, output);

    mbedtls_aes_free(&aes_ctx);

    return true;
}
/**
    * @brief  128 bit AES CBC descryption on speicified plaintext and keys
    * @param  input    specifed encypted data to be decypted
    * @param  key          keys to decrypt the data
    * @param  output    output buffer to store plain data
    * @param  iv         initialization vector (IV) for CBC mode
    * @param  input_len    byte length of the data to be descrypted, must be multiples of 16
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note   least significant octet of encrypted data corresponds to encypted[0]
    */
__TUYA_BLE_WEAK bool tuya_ble_aes128_cbc_decrypt(uint8_t *key, uint8_t *iv, uint8_t *input, uint16_t input_len, uint8_t *output)
{
    mbedtls_aes_context aes_ctx;

    if (input_len % 16) {
        return false;
    }

    mbedtls_aes_init(&aes_ctx);

    mbedtls_aes_setkey_dec(&aes_ctx, key, 128);

    mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, input_len, iv, input, output);

    mbedtls_aes_free(&aes_ctx);

    return true;
}
/**
    * @brief  MD5 checksum
    * @param  input    specifed plain text to be encypted
    * @param  output    output buffer to store md5 result data,output data len is always 16
    * @param  input_len    byte length of the data to be encypted
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note
    */
__TUYA_BLE_WEAK bool tuya_ble_md5_crypt(uint8_t *input, uint16_t input_len, uint8_t *output)
{
    mbedtls_md5_context md5_ctx;
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts(&md5_ctx);
    mbedtls_md5_update(&md5_ctx, input, input_len);
    mbedtls_md5_finish(&md5_ctx, output);
    mbedtls_md5_free(&md5_ctx);
    return true;
}

/**
    * @brief          This function calculates the full generic HMAC
    *                 on the input buffer with the provided key.
    * @param  key      The HMAC secret key.
    * @param  key_len  The length of the HMAC secret key in Bytes.
    * @param  input    specifed plain text to be encypted
    * @param  output    output buffer to store the result data
    * @param  input_len    byte length of the data to be encypted
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note
    */
__TUYA_BLE_WEAK bool tuya_ble_hmac_sha1_crypt(const uint8_t *key, uint32_t key_len, const uint8_t *input, uint32_t input_len, uint8_t *output)
{
    return true;
}

/**
    * @brief          This function calculates the full generic HMAC
    *                 on the input buffer with the provided key.
    * @param  key      The HMAC secret key.
    * @param  key_len  The length of the HMAC secret key in Bytes.
    * @param  input    specifed plain text to be encypted
    * @param  output    output buffer to store the result data
    * @param  input_len    byte length of the data to be encypted
    * @return encryption results
    * @retval true      successful
    * @retval false     fail
    * @note
    */
__TUYA_BLE_WEAK bool tuya_ble_hmac_sha256_crypt(const uint8_t *key, uint32_t key_len, const uint8_t *input, uint32_t input_len, uint8_t *output)
{
    return true;
}


/**
 * \brief    Allocate a memory block with required size.
 *
 *
 * \param[in]   size     Required memory size.
 *
 * \return     The address of the allocated memory block. If the address is NULL, the
 *             memory allocation failed.
 */
__TUYA_BLE_WEAK void *tuya_ble_port_malloc(uint32_t size)
{
    return malloc(size);
}



/**
 *
 * \brief    Free a memory block that had been allocated.
 *
 * \param[in]   pv     The address of memory block being freed.
 *
 * \return     None.
 */
__TUYA_BLE_WEAK void tuya_ble_port_free(void *pv)
{
    return free(pv);
}





