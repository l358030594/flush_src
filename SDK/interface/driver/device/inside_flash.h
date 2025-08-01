#ifndef __INSIDE_FLASH_H__
#define __INSIDE_FLASH_H__

#include "typedef.h"
#include "device.h"

struct flash_wp_arg {   //写保护配置信息
    u8 numOfwp_array;//写保护参数的个数
    u8 wp_sr1_mask; //sr1要保留或修改的bit
    u8 wp_sr2_mask; //sr2要保留或修改的bit
    struct {
        u8 wp_sr1; //写保护sr1取值
        u8 wp_sr2;//写保护sr2取值
        u16 wp_addr;//写保护结束地址,单位K
    } wp_array[32]; //写保护的组数，修改可变长
} __attribute__((packed));

struct flash_otp_arg {//flash otp信息
    u8 otp_lock_sr1_mask;//sr1要保留或修改的bit
    u8 otp_lock_sr2_mask;//sr2要保留或修改的bit
    u8 otp_lock_sr1;//otp sr1的取值
    u8 otp_lock_sr2;//otp sr2的取值
    u16 otp_NumberOfpage;//otp的page数量
    u16 otp_page_size;//otp的page大小
    u32 otp_offset[5];//otp page的偏移地址组数
} __attribute__((packed));

struct flash_io_drv {
    u8 cs_drv: 2;
    u8 clk_drv: 2;
    u8 do_drv: 2;
    u8 di_drv: 2;
    u8 d2_drv: 2;
    u8 d3_drv: 2;
    u8 input_delay: 4;
} __attribute__((packed));

struct	flash_config_info { //flash配置信息的结构体
    u8 write_en_use_50h;//写使能配置0:06H		1:50H
    u8 wr_sr_cmd[2];//寄存器的写命令，当两个命令是一样的，就使用连续写模式
    struct	flash_wp_arg wp;// flash 写保护参数
    struct	flash_otp_arg otp; //flash otp 参数
    struct  flash_io_drv drv; // flash drv 参数
} __attribute__((packed));
int  norflash_set_write_protect(u32 enable, u32 wp_addr);
u32 syscfg_read_otp(u32 id, u8 *buf, u32 len);
#endif
