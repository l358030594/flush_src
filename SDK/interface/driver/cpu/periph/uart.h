#ifndef DEVICE_UART_H
#define DEVICE_UART_H

#include "typedef.h"
#include "device/device.h"
#include "generic/ioctl.h"
#include "system/task.h"
#include "cpu/includes.h"
#include "circular_buf.h"

extern void putbyte(char a);
typedef const int uart_dev;
extern const u32 lib_config_uart_flow_enable;
enum uart_parity : u8 {//uart_v2有效
    UART_PARITY_DISABLE  = 0x0,
    UART_PARITY_all_0,
    UART_PARITY_all_1,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
};

enum uart_event : u8 {
    UART_EVENT_RX_DATA = 1,
    UART_EVENT_RX_FIFO_OVF = 2,       //接收缓冲区溢出
    UART_EVENT_RX_TIMEOUT = 4,        //数据帧接收完成
    UART_EVENT_PARITY_ERR = 8,        //奇偶校验错误
    UART_EVENT_TX_DONE = 16,          //数据发送完成
};

enum uart_state : char {
    UART_OK = 0,
    UART_ERROR_BUSY = -1,
    UART_ERROR_CLOSE = -2,      //接收缓冲区溢出
    UART_ERROR_CONTEXT = -3,      //接收缓冲区溢出
    UART_ERROR_MALLOC = -4,       //数据帧接收完成
    UART_ERROR_BUF_ALIGN4 = -5,       //数据帧接收完成
    UART_ERROR_TIMEOUT = -6,       //数据帧接收完成
    UART_ERROR_OS_CREATE = -7,       //奇偶校验错误
    UART_ERROR_OS_MUTEX = -8,       //奇偶校验错误
    UART_ERROR_BAUD = -9,       //baud错误
    UART_ERROR_RX_BUF_MALLOC = -10,      //rx_buf malloc错误
    UART_ERROR_TX_BUF_MALLOC = -11,      //tx_buf malloc错误
    UART_ERROR_FLOWCTRL = -12,      //硬件流初始化不成功
    UART_ERROR_FLOWCTRL_IO = -13,      //硬件流io错误
};

struct uart_config {
    u32 baud_rate;
    u16 tx_pin; //不使用时配0xffff
    u16 rx_pin; //不使用时配0xffff,当rx_pin == tx_pin 的时候为单线半双工通讯模式
    enum uart_parity parity;//uart_v2有效
    u8 tx_wait_mutex;//1:不支持中断调用,互斥,0:支持中断,不互斥
};


struct uart_dma_config {
    OS_SEM uart_rx;
    cbuffer_t cbuf;
    u32 rx_timeout_thresh;//us, 需大于1byte时间
    u32 frame_size;//一般配置=rx_cbuffer_size
    u32 rx_cbuffer_size;
    void *rx_cbuffer;
    void (*irq_callback)(uart_dev uart_num, enum uart_event);      //推送到调用者的线程执行
    u8 irq_priority;//中断优先级
    enum uart_event event_mask;
};


struct uart_flow_ctrl {
    u16 cts_pin;      //不使用时配0xffff
    u16 rts_pin;      //不使用时配0xffff
    u8 cts_idle_level;     //0:cts空闲电平为低; 1:cts空闲电平为高
    u8 rts_idle_level;     //0:rts空闲电平为低; 1:rts空闲电平为高
    u8 rx_thresh;         // 0~100 %
};


//uart_num = -1则自动分配串口号，
//return 返回值为负数，则初始化失败,>=0:uart号
s32 uart_init(uart_dev uart_num, const struct uart_config *config);
//return <0:error; 0:ok
s32 uart_deinit(uart_dev uart_num);

//return <0:error; 0:ok
s32 uart_dma_init(uart_dev uart_num, const struct uart_dma_config *dma_config);

//打印uart配置信息
void uart_dump();
//return 实际波特率,<0:error; >0:ok
s32 uart_set_baudrate(uart_dev uart_num, u32 baud_rate);

//return 实际波特率,<0:error; >0:ok
s32 uart_set_rx_timeout_thresh(uart_dev uart_num, u32 timeout_thresh);

//发送数据按位取反后发出
//tx_inv_en:1:取反,0:不取反
void uart_tx_data_inv(uart_dev uart_num, u8 tx_inv_en);
//接收数据按位取反后接收
//rx_inv_en:1:取反,0:不取反
void uart_rx_data_inv(uart_dev uart_num, u8 rx_inv_en);

//启动dma发送，要求buffer指向的地址是允许dma的，
//函数启动dma后，立即返回，不会等待数据发送完成
//要求buffer指向的空间在发送结束前是有效的
//return 其他:error; =size:ok
s32 uart_send_bytes(uart_dev uart_num, const void *buffer, u32 size);

// 等待串口发送完成，timeout_ms = 0 就一直等
//return 其他:error; 0:ok
s32 uart_wait_tx_idle(uart_dev uart_num, u32 timeout_ms);

//阻塞式dma发送数据 要求src指向的地址是允许dma的，
//函数会等到数据发送完成，再返回; 阻塞接口不可在中断调用
//timeout_ms = 0就一直等直到发送完成
//return 其他:error; =size:ok
s32 uart_send_blocking(uart_dev uart_num, const void *buffer, u32 size, u32 timeout_ms);
//return <0:error; 0:busy,1:idle
s32 uart_is_send_complete(uart_dev uart_num);//0:busy; 1:idle; -1:error

//return，dma接收的长度 <0:error;
s32 uart_get_recv_len(uart_dev uart_num);
//从缓冲区读取数据
//支持中断调用
//return，返回实际读取的长度 <0:error; 0< <len:ok
s32 uart_recv_bytes(uart_dev uart_num, void *buffer, u32 len);

//从缓冲区读取数据，直到读取到有效长度的数据，或者超时; 阻塞接口不可在中断调用
//return，返回实际读取的长度 <0:error; 0< <len:ok
s32 uart_recv_blocking(uart_dev uart_num, void *buffer, u32 len, u32 timeout_ms);

//dma接收错乱时重置dma及buf，重新接收
//return 见枚举
enum uart_state uart_dma_rx_reset(uart_dev uart_num);
//配置串口延时进低功耗时间(默认延时30ms)
void uart_set_idle_query_wait(u32 ms);

//流控配置，对于没有硬件流控的uart，则使用软件实现，会有比较大的滞后，设置的阈值余量要足够
//return <0:error; 0:ok
s32 uart_flow_ctrl_init(uart_dev uart_num, const struct uart_flow_ctrl *flow_ctrl);
//return <0:error; 0:ok
s32 uart_flow_ctrl_deinit(uart_dev uart_num);

//return <0:error; 0:ok
s32 uart_putbyte(uart_dev uart_num, u32 a);
//printf return <0:error; 0:ok
s32 uart_log_putbyte(uart_dev uart_num, u32 a);
//return <0:error; 其他:ok
s32 uart_getbyte(uart_dev uart_num);
#endif

