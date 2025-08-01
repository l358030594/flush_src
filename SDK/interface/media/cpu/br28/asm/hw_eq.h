
#ifndef __HW_EQ_H
#define __HW_EQ_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "os/os_api.h"
#include "effects/eq_func_define.h"
#include "system/spinlock.h"



enum {				//运行模式
    NORMAL = 0,		//正常模式
    MONO,			//单声道模式
    STEREO			//立体声模式
};

enum {			 	//输出数据类型
    DATO_SHORT = 0, //short
    DATO_INT,		//int
    DATO_FLOAT		//float
};
enum {				//输入数据类型
    DATI_SHORT = 0, //short
    DATI_INT,		// int
    DATI_FLOAT		//float
};
enum {					//输入数据存放模式
    BLOCK_DAT_IN = 0, 	//块模式，例如输入数据是2通道，先存放完第1通道的所有数据，再存放第2通道的所有数据
    SEQUENCE_DAT_IN,	//序列模式，例如输入数据是2通道，先存放第通道的第一个数据，再存放第2个通道的第一个数据，以此类推。
};
enum {				 //输出数据存放模式
    BLOCK_DAT_OUT = 0,//块模式，例如输出数据是2通道，先存放完第1通道的所有数据，再存放第2通道的所有数据
    SEQUENCE_DAT_OUT, //序列模式，例如输入数据是2通道，先存放第通道的第一个数据，再存放第2个通道的第一个数据，以此类推。
};


/*eq IIR type*/
typedef enum {
    EQ_IIR_TYPE_HIGH_PASS = 0x00,
    EQ_IIR_TYPE_LOW_PASS,
    EQ_IIR_TYPE_BAND_PASS,
    EQ_IIR_TYPE_HIGH_SHELF,
    EQ_IIR_TYPE_LOW_SHELF,
    EQ_IIR_TYPE_HIGH_SHELF_Q_TYPE,
    EQ_IIR_TYPE_LOW_SHELF_Q_TYPE,
    EQ_IIR_TYPE_HIGH_PASS_ADVANCE,//在gain位置存阶数，0~6阶级，0：disbale
    EQ_IIR_TYPE_LOW_PASS_ADVANCE,//在gain位置存阶数，0~6阶级，0：disbale

} EQ_IIR_TYPE;

struct eq_seg_info {
    u16 index;
    u16 iir_type; ///<EQ_IIR_TYPE
    int freq;
    float gain;
    float q;
};

struct eq_coeff_info {
    u8 nsection;     //eq段数
    u8 no_coeff;	  //不是滤波系数
#ifdef CONFIG_EQ_NO_USE_COEFF_TABLE
    u32 sr;               //采样率
#endif
    float *L_coeff;
    float *R_coeff;
    float L_gain;
    float R_gain;
    float *N_coeff[8];
    float N_gain[8];
};

struct hw_eq_ch;

struct hw_eq_cfg {
    u16 irq_index;
    u8 hw_section;
    u8 cpu_section;
    u32 hw_ram_addr;
    u32 cpu_ram_addr;
    int hw_ram_size;
    JL_EQ_TypeDef *regs;
    void (*irq_handler)(void);
};
struct hw_eq {
    struct list_head head;            //链表头
    struct list_head frame_head;            //数据帧链表头
    volatile struct hw_eq_ch *cur_ch; //当前需要处理的eq通道
    volatile struct hw_eq_cfg *cfg;   //硬件寄存器相关配置
    u32 core;
    /*****增加eq name标识，用于申请时钟数*/
    u32 cnt;
    /*连续的dma_buf, 每个硬件都有一块，每次以最大使用量做申请*/
    s32 *dma_addr;
    s32 dma_len;//byte
};

enum {
    HW_EQ_CMD_CLEAR_MEM = 0xffffff00,
    HW_EQ_CMD_CLEAR_MEM_L,
    HW_EQ_CMD_CLEAR_MEM_R,
};

struct hw_eq_handler {
    int (*eq_probe)(struct hw_eq_ch *);
    int (*eq_output)(struct hw_eq_ch *, s16 *, u16);
    int (*eq_post)(struct hw_eq_ch *);
    int (*eq_input)(struct hw_eq_ch *, void **, void **);
};

struct hw_eq_info {
    unsigned char run_mode;           //0按照输入的数据排布方式 ，输出数据 1:单入多出，  2：立体声入多出
    unsigned char in_mode;            //输入数据的位宽 0：short  1:int  2:float
    unsigned char out_mode;          //输出数据的位宽 0：short  1:int  2:float
    unsigned char out_channels;       //输出通道数
    unsigned char data_in_mode;       //输入数据存放模式
    unsigned char data_out_mode;      //输入数据存放模式
};

struct hw_eq_ch {
    unsigned char updata;             //更新参数以及中间数据
    unsigned char updata_coeff_only;  //只更新参数，不更新中间数据
    unsigned char no_wait;            //是否是异步eq处理  0：同步的eq  1：异步的eq
    unsigned char channels;           //输入通道数
    unsigned char nsection;           //eq段数
    unsigned char no_coeff;	          // 非滤波系数
    volatile unsigned char active;    //已启动eq处理  1：busy  0:处理结束
    volatile unsigned char need_run;  //多eq同时使用时，未启动成功的eq，是否需要重新唤醒处理  1：需要  0：否
    unsigned char run_mode;           //0按照输入的数据排布方式 ，输出数据 1:单入多出，  2：立体声入多出
    unsigned char in_mode;            //输入数据的位宽 0：short  1:int  2:float
    unsigned char out_32bit;          //输出数据的位宽 0：short  1:int  2:float
    unsigned char out_channels;       //输出通道数
    unsigned char data_in_mode;       //输入数据存放模式
    unsigned char data_out_mode;      //输入数据存放模式
    float *L_coeff;
    float *R_coeff;
    float L_gain;
    float R_gain;
    float *N_coeff[8];
    float N_gain[8];
    float *eq_LRmem;
    s16 *out_buf;
    s16 *in_buf;
    int in_len;
    void *priv;
    volatile OS_SEM sem;
    struct list_head entry;
    struct hw_eq *eq;
    const struct hw_eq_handler *eq_handler;
    void *irq_priv;                  //eq管理层传入的私有指针
    void (*irq_callback)(void *priv);//需要eq中断执行的回调函数

    void *eq_ch;
    int (*irq_eq_process)(void *eq_ch, void *indata, void *outdata, int inlen);
    struct list_head frame_entry;

    char name[8];
};

//系数计算子函数

/*
fc: 低通滤波器-3dB衰减频点
fs: 采样率
nSOS: 生成nSOS阶IIR
coeff: 输出系数地址 （ 大小为 ((nSOS & 1) + (nSOS >> 1))*5 , 顺序是按照硬件EQ摆放了 )
段数：((nSOS & 1) + (nSOS >> 1))*/
/*
 * 多阶级低通滤波器
fc: 低通滤波器-3dB衰减频点
fs: 采样率
nSOS: 生成多少个2阶IIR
coeff: 输出系数地址 （ 大小为 nSOS*5 , 顺序是按照硬件EQ摆放了 )*/
extern void  butterworth_lp_design(int fc, int fs, int nSOS, float *coeff);

/*
fc: 通滤波器-3dB衰减频点
fs: 采样率
nSOS: 生成nSOS阶IIR
coeff: 输出系数地址 （ 大小为((nSOS & 1) + (nSOS >> 1))*5 , 顺序是按照硬件EQ摆放了 )
段数：((nSOS & 1) + (nSOS >> 1))*/
extern void butterworth_hp_design(int fc, int fs, int nSOS, float *coeff);


/*----------------------------------------------------------------------------*/
/**@brief    低通滤波器
   @param    fc:中心截止频率
   @param    fs:采样率
   @param    quality_factor:q值
   @param    coeff:计算后，系数输出地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
extern void design_lp(int fc, int fs, float quality_factor, float *coeff);
/*----------------------------------------------------------------------------*/
/**@brief    高通滤波器
   @param    fc:中心截止频率
   @param    fs:采样率
   @param    quality_factor:q值
   @param    coeff:计算后，系数输出地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
extern void design_hp(int fc, int fs, float quality_factor, float *coeff);
/*----------------------------------------------------------------------------*/
/**@brief    带通滤波器
   @param    fc:中心截止频率
   @param    fs:采样率
   @param    gain:增益
   @param    quality_factor:q值
   @param    coeff:计算后，系数输出地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
extern void design_pe(int fc, int fs, float gain, float quality_factor, float *coeff);
/*----------------------------------------------------------------------------*/
/**@brief    低频搁架式滤波器
   @param    fc:中心截止频率
   @param    fs:采样率
   @param    gain:增益
   @param    quality_factor:q值
   @param    coeff:计算后，系数输出地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
extern void design_ls(int fc, int fs, float gain, float quality_factor, float *coeff);
/*----------------------------------------------------------------------------*/
/**@brief    高频搁架式滤波器
   @param    fc:中心截止频率
   @param    fs:采样率
   @param    gain:增益
   @param    quality_factor:q值
   @param    coeff:计算后，系数输出地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
extern void design_hs(int fc, int fs, float gain, float quality_factor, float *coeff);
/*----------------------------------------------------------------------------*/
/**@brief    滤波器系数检查
   @param    coeff:滤波器系数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
extern int eq_stable_check(float *coeff);
float eq_db2mag(float x);

/*----------------------------------------------------------------------------*/
/**@brief    获取直通的滤波器系数
   @param    coeff:滤波器系数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
extern void eq_get_AllpassCoeff(void *Coeff);

/*----------------------------------------------------------------------------*/
/**@brief    在EQ中断中调用
   @param    *eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_hw_eq_irq_handler(struct hw_eq *eq);

/*----------------------------------------------------------------------------*/
/**@brief    EQ初始化
   @param    *eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_hw_eq_init(struct hw_eq *eq, u32 eq_section_num);


/*----------------------------------------------------------------------------*/
/**@brief    打开一个通道
   @param    *ch:通道句柄
   @param    *eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_hw_eq_ch_open(struct hw_eq_ch *ch, struct hw_eq *eq);
/*----------------------------------------------------------------------------*/
/**@brief    设置回调接口
   @param    *ch:通道句柄
   @param    *handler:回调的句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_hw_eq_ch_set_handler(struct hw_eq_ch *ch, struct hw_eq_handler *handler);
//
/*----------------------------------------------------------------------------*/
/**@brief    设置通道基础信息
   @param    *ch:通道句柄
   @param    channels:通道数
   @param    out_32bit:是否输出32bit位宽数据 1：是  0：16bit位宽
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_hw_eq_ch_set_info(struct hw_eq_ch *ch, u8 channels, u8 out_32bit);
int audio_hw_eq_ch_set_info_new(struct hw_eq_ch *ch, struct hw_eq_info *info);
/*----------------------------------------------------------------------------*/
/**@brief    设置硬件转换系数
   @param    *ch:通道句柄
   @param    *info:系数、增益等信息
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_hw_eq_ch_set_coeff(struct hw_eq_ch *ch, struct eq_coeff_info *info);

/*----------------------------------------------------------------------------*/
/**@brief    启动一次转换
   @param   *ch:eq句柄
   @param  *input:输入数据地址
   @param  *output:输出数据地址
   @param  len:输入数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_hw_eq_ch_start(struct hw_eq_ch *ch, void *input, void *output, int len);

/*----------------------------------------------------------------------------*/
/**@brief  关闭一个通道
   @param   *ch:eq句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_hw_eq_ch_close(struct hw_eq_ch *ch);

/*----------------------------------------------------------------------------*/
/**@brief  获取eq是否正在运行状态
   @param   *ch:eq句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_hw_eq_is_running(struct hw_eq *eq);
void audio_hw_eq_spin_lock(struct hw_eq *eq);
void audio_hw_eq_spin_unlock(struct hw_eq *eq);
void design_ls_Q(int fc, int fs, float gain, float quality_factor, float *coeff);
void design_hs_Q(int fc, int fs, float gain, float quality_factor, float *coeff);

#endif /*__HW_EQ_H*/

