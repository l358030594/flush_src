#ifndef _IIC_HW_V1_H_
#define _IIC_HW_V1_H_

#include "system/generic/typedef.h"
#include "gpio.h"

#define IIC_WHILE_TIMEOUT_CNT_ 2120000 //24Mhz:400ms

#define MAX_HW_IIC_NUM                  1
// #define IIC_PORT_GROUP_NUM          4


#define iic_enable(reg)             (reg->CON0 |= BIT(0))
#define iic_disable(reg)            (reg->CON0 &= ~BIT(0))
#define is_iic_enable(reg)          (reg->CON0 & BIT(0))
#define iic_role_host(reg)          (reg->CON0 &= ~BIT(1))
#define iic_role_slave(reg)         (reg->CON0 |= BIT(1))
#define iic_cfg_done(reg)           (reg->CON0 |= BIT(2))
#define iic_dir_out(reg)            (reg->CON0 &= ~BIT(3))
#define iic_dir_in(reg)             (reg->CON0 |= BIT(3))
#define iic_preset_end(reg)         (reg->CON0 |= BIT(4))
#define iic_preset_restart(reg)     (reg->CON0 |= BIT(5))
#define iic_recv_ack(reg)           (reg->CON0 &= ~BIT(6))
#define iic_recv_nack(reg)          (reg->CON0 |= BIT(6))
#define iic_send_is_ack(reg)        (!(reg->CON0 & BIT(7)))
#define iic_isel_direct(reg)        (reg->CON0 &= ~BIT(9))
#define iic_isel_filter(reg)        (reg->CON0 |= BIT(9))
#define iic_isel_filter_en(reg)     (reg->CON0 &= ~BIT(11))//only br27
#define iic_isel_bypass(reg)        (reg->CON0 |= BIT(11)) //only br27
#define iic_si_mode_en(reg)         (reg->CON1 |= BIT(13))
#define iic_si_mode_dis(reg)        (reg->CON1 &= ~BIT(13))

#define iic_set_ie(reg)             (reg->CON0 |= BIT(8))
#define iic_clr_ie(reg)             (reg->CON0 &= ~BIT(8))
#define iic_pnd(reg)                (reg->CON0 & BIT(15))
#define iic_pnd_clr(reg)            (reg->CON0 |= BIT(14))

#define iic_set_end_ie(reg)         (reg->CON0 |= BIT(10))
#define iic_clr_end_ie(reg)         (reg->CON0 &=~BIT(10))
#define iic_end_pnd(reg)            (reg->CON0 & BIT(13))
#define iic_end_pnd_clr(reg)        (reg->CON0 |= BIT(12))

#define iic_start_pnd(reg)          (reg->CON1 & BIT(15))
#define iic_start_pnd_clr(reg)      (reg->CON1 |= BIT(14))

#define iic_reset0(reg)        (reg->CON0 = (BIT(14)|BIT(12)))
#define iic_reset1(reg)        (reg->CON1 = BIT(14))
#define iic_baud_reg(reg)           (reg->BAUD)
#define iic_buf_reg(reg)            (reg->BUF)

typedef enum {
    I2C_PND_TASK_DONE,
    I2C_PND_START,//no ie
    I2C_PND_STOP,
} i2c_pnd_typedef;

enum {
    HW_IIC_0,
};

typedef const u8 hw_iic_dev;
#include "iic_api.h"

struct hw_iic_slave_config {
    struct iic_master_config config;
    void (*iic_slave_irq_func)(void);
    u8 slave_addr;//bit7~bit1
};

struct iic_master_config *get_hw_iic_config(hw_iic_dev iic);
enum iic_state_enum hw_iic_init(hw_iic_dev iic, struct iic_master_config *i2c_config);
enum iic_state_enum hw_iic_deinit(hw_iic_dev iic);
enum iic_state_enum hw_iic_resume(hw_iic_dev iic);
enum iic_state_enum hw_iic_suspend(hw_iic_dev iic);
enum iic_state_enum hw_iic_check_busy(hw_iic_dev iic);
enum iic_state_enum hw_iic_start(hw_iic_dev iic);
void hw_iic_stop(hw_iic_dev iic);
void hw_iic_reset(hw_iic_dev iic);//无效
void iic_hw_err_reset(hw_iic_dev iic);
u8 hw_iic_tx_byte(hw_iic_dev iic, u8 byte);
u8 hw_iic_rx_byte(hw_iic_dev iic, u8 ack);
//return: =len:ok
int hw_iic_read_buf(hw_iic_dev iic, void *buf, int len);
//return: =len:ok
int hw_iic_write_buf(hw_iic_dev iic, const void *buf, int len);

int hw_iic_set_baud(hw_iic_dev iic, u32 baud);

void hw_iic_set_ie(hw_iic_dev iic, i2c_pnd_typedef png, u8 en);
u8 hw_iic_get_pnd(hw_iic_dev iic, i2c_pnd_typedef png);
void hw_iic_clr_pnd(hw_iic_dev iic, i2c_pnd_typedef png);
void hw_iic_clr_all_pnd(hw_iic_dev iic);




//从机接口:
enum iic_slave_rx_state {
    IIC_SLAVE_RX_PREPARE_TIMEOUT = -1,
    IIC_SLAVE_RX_PREPARE_OK = 0,
    IIC_SLAVE_RX_PREPARE_END_OK = 1,
    IIC_SLAVE_RX_ADDR_NO_MATCH = -2,
    IIC_SLAVE_RX_ADDR_TX = 2,
    IIC_SLAVE_RX_ADDR_RX = 3,
    IIC_SLAVE_RX_DATA = 4,
};

enum iic_state_enum hw_iic_slave_init(hw_iic_dev iic, struct hw_iic_slave_config *i2c_config);
void hw_iic_slave_set_addr(hw_iic_dev iic, u8 addr, u8 addr_ack);
void hw_iic_slave_set_isr_func(hw_iic_dev iic, void (*iic_slave_irq_func)(void));
u8 hw_iic_slave_get_addr(hw_iic_dev iic);
enum iic_slave_rx_state hw_iic_slave_rx_prepare(hw_iic_dev iic, u8 ack, u32 wait_time);//轮询, 准备收
//判断地址，返回数据类型, 不检查结束位
enum iic_slave_rx_state hw_iic_slave_rx_byte(hw_iic_dev iic, u8 *rx_byte);
int hw_iic_slave_rx_nbyte(hw_iic_dev iic, u8 *rx_buf);//轮询,含结束位
u8 hw_iic_slave_tx_check_ack(hw_iic_dev iic);//return:1:ack; 0:no ack
void hw_iic_slave_tx_byte(hw_iic_dev iic, u8 byte);//准备发
int hw_iic_slave_tx_nbyte(hw_iic_dev iic, u8 *tx_buf);//轮询,含结束位
#endif
