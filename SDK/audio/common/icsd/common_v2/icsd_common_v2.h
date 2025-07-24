#ifndef _ICSD_COMMON_V2_H
#define _ICSD_COMMON_V2_H

#include "generic/typedef.h"
#include "math.h"
#include "stdlib.h"
#include "timer.h"
#include "asm/math_fast_function.h"

#define ICSD_WIND_HEADSET           1
#define ICSD_WIND_TWS		        2

#define ICSD_WIND_LFF_TALK          1
#define ICSD_WIND_LFF_RFF           2
#define ICSD_WIND_LFF_LFB           3
#define ICSD_WIND_LFF_LFB_TALK      4
#define ICSD_WIND_RFF_TALK      	5


unsigned long jiffies_usec(void);


enum {
    DEBUG_FUNCTION_ICSD = 0,
    DEBUG_FUNCTION_APP,
};

enum {
    WIND_DATA = 0,
    WIND_ALG,
};


enum {
    norm_rtanc = 0,	//正常模式
    tidy_rtanc,		//轻量模式，节省RAM
};

enum {
    DEBUG_FUNCTION_PRINTF = 0,
    DEBUG_FUNCTION_ADT,
    DEBUG_FUNCTION_WIND,
    DEBUG_FUNCTION_EIN,
};

enum {
    ADT_ANC_ON = 0,
    ADT_ANC_OFF,
};

enum {
    RESUME_ANCMODE = 0,
    RESUME_BYPASSMODE,
};

enum {
    ANC_PZ = 0,
    ANC_SZ,
};

enum {
    RT_ANC_CH01 = 0,
    RT_ANC_CH23,
};

enum {
    ADT_TWS = 0,
    ADT_HEADSET,
};
extern const u8  ICSD_ADT_EP_TYPE;

enum {
    DEBUG_ADT_STATE = 0,
    DEBUG_ADT_WIND,
    DEBUG_ADT_WIND_MIC_DATA,
    DEBUG_ADT_ENVNL,
    DEBUG_ADT_INF,
    DEBUG_ADT_VDT_TRAIN,
    DEBUG_ADT_RESET,
    DEBUG_ADT_DP_STATE,
    DEBUG_ADT_WIND_RUN_DATA,
    DEBUG_ADT_VDT_DATA,
};

enum {
    WAT_DEBUG = 0,
    WAT_GET_MAX_RANGE,
    WAT_SEND_MAX_RANGE,
    ADT_WIND_DATA_SYNC,
    LIB_DEMO_TWS,
    LIB_WIND_TWS,
    ADT_TWS_TONE_PLAY,
};

enum {
    ICSD_FULL_INEAR = 0,
    ICSD_HALF_INEAR,
    ICSD_HEADSET,
};

enum {
    ICSD_BR50 = 0,
    ICSD_BR28,
};



typedef struct {
    int len;	//float points
    float *out;
    float *msc;
} __afq_data;

//算法参数输出结构
typedef struct {
    u8 state;
    __afq_data *sz_l;
    __afq_data *sz_r;
} __afq_output;

typedef struct {
    float spl_db;
    float spl_db_err;
    float spl_min;
    u8	  spl_st;
    u8	  spl_jt;
    int   spl_freq;
} spl_parm;

// target pre process
struct icsd_target_param {
    int cmp_idx_begin;
    int cmp_idx_end;
    int cmp_en;
    int cmp_total_len;
    int target_cmp_num;

    float  pz_gain;
    float *target_sv;
    float *target_cmp_dat;
};


// self talk dector
typedef struct {
    s16   dsf_buf[256];
    u32   win_sum_buf[16];
    u8    dir;
    u16   dov;
    s16   *data;
    s16   *angle_thr;
    float *hz_angle;
} __self_talk_ram;


struct aeq_seg_info {
    u16 index;
    u16 iir_type;
    int freq;
    float gain;
    float q;
};

struct aeq_default_seg_tab {
    float global_gain;
    int seg_num;
    struct aeq_seg_info *seg;
};


#define ADT_DMA_BUF_LEN     	512
#define ADT_FFT_LENS   			256

#define TARLEN2   				(120)// + 318)
#define TARLEN2_L				0 //40
#define DRPPNT2  				0 //10
#define MEMLEN                  50
#define MSELEN                  (44*2)

#define FS 375000

#define FS_TANC   46875
#define FFTLEN   				1024
#define FFTLEN_L 				4096
#define SWARM_NUM 60
#define FLEN_V2 ((TARLEN2+TARLEN2_L-DRPPNT2)/2)  //96

//ANC_TRAIN
#define FREQ_POINT 25

typedef struct {
    float pzh_out[TARLEN2 / 2];
    float pzm_out[TARLEN2 / 2];
    float pzl_out[TARLEN2 / 2];
} __icsd_pnc_cmp;

#include "anc_DeAlg_v2.h"
//LIB调用的算术函数
float angle_float(float x, float y);

unsigned int hw_fft_config(int N, int log2N, int is_same_addr, int is_ifft, int is_real);
void hw_fft_run(unsigned int fft_config, const int *in, int *out);
void  icsd_common_version();

//---------------------------
// multi float  conj
//---------------------------
//---------------------------
// multi conj
//---------------------------

//---------------------------
// div
//---------------------------
//hard upspeed

extern double log10_anc(double x);

float icsd_anc_pow10(float n);
double icsd_log10_anc(double x);
double icsd_sqrt_anc(double x);

void icsd_complex_1plus_divf(float *input1, float *input2, float *out);
void icsd_complex_muln_dsmp(float *input1, float *input2, float *out, int len, int downsample);
void icsd_complex_conj_mulf_1p(float *input1, float *input2, float *out);

void icsd_anc_fft256(int *in, int *out);
void icsd_anc_fft128(int *in, int *out);
void icsd_anc_fft64(int *in, int *out);
void icsd_anc_fft(int *in, int *out);
//--------------------------------------------
// biquad_operation
//--------------------------------------------
//void ff_fgq_2_aabb(double *iir_ab, float *ff_fgq);
void icsd_cal_wz_with_dsmp(float *ab, float gain, int tap, float *freq, float fs, float *wz, float *w1, float *w2, int len, int downsample);
void icsd_biquad2ab_out_3(float f, float gain, float q, float fs, float *a0, float *a1, float *a2, float *b0, float *b1, float *b2, u8 type);
void icsd_fgq_2_aabb(float *iir_ab, float *ff_fgq, u8 *ff_type, float fs, int order);
void icsd_fgq2hz(float gain, float *ff_fgq, u8 *ff_type, int order, float *freq, float *hz, float *w1, float *w2, int len, float fs, int downsample);
void icsd_gfq2hz(float gain, float *ff_gfq, u8 *ff_type, int order, float *freq, float *hz, float *w1, float *w2, int len, float fs, int downsample);
void icsd_cal_wz(double *ab, float gain, int tap, float *freq, float fs, float *wz, int len);
void icsd_cal_wz_float(float *ab, float gain, int tap, float *freq, float fs, float *wz, int len);
//
void icsd_fgq2hz_v2(float total_gain, float *fgq, u8 *type, u8 order, float *freq, float *hz, int len, float fs);
void icsd_gfq2hz_v2(float total_gain, float *gfq, u8 *type, u8 order, float *freq, float *hz, int len, float fs);

void icsd_biquad2ab_out_2(float gain, float f, float q, u8 type, float fs, double *a0, double *a1, double *a2, double *b0, double *b1, double *b2);
void icsd_biquad2ab_double_v2(float gain, float f, float q, double *a0, double *a1, double *a2, double *b0, double *b1, double *b2, u8 type, float fs);
void icsd_biquad2ab_out_v2(float gain, float f, float fs, float q, double *a0, double *a1, double *a2, double *b0, double *b1, double *b2, u8 type);
void icsd_biquad2ab_double_pn(float gain, float f, float q, double *a0, double *a1, double *a2,
                              double *b0, double *b1, double *b2, u8 type);
//

//--------------------------------------------
// freq_response
//--------------------------------------------
float hz_diff_db(float *_src1, float *_src2, int sp, int ep);
void fre_resp_resample_v3(float *_src, float *_dest);
void icsd_anc_h_freq_init(float *freq, u8 mode);
void icsd_hz2pxdB(float *hz, float *px, int len);
void icsd_pwr2pxdB(float *pwr, float *px, int len);
void icsd_HanningWin_pwr2(s16 *input, int *output, int len);
void icsd_HanningWin_pwr_float(float *input, int *output, int len);
void icsd_HanningWin_pwr_s1(s16 *input, int *output, int len);
void icsd_HanningWin_pwr_s2(s16 *input, int *output, int len);
void icsd_FFT_radix1024(int *in_cur, int *out);
void icsd_FFT_radix256(int *in_cur, int *out);
void icsd_FFT_radix128(int *in_cur, int *out);
void icsd_FFT_radix64(int *in_cur, int *out);
void icsd_FFT_lowfreq4096(s16 *in_cur, float *out, int len);
void icsd_htarget(float *Hpz, float *Hsz, float *Hflt, int len);
void icsd_pxydivpxxpyy(float *pxy, float *pxx, float *pyy, float *out, int len);

//--------------------------------------------
//math
//--------------------------------------------
float icsd_get_abs(float a);
float icsd_abs_float(float f);
float icsd_cos_hq_v2(float x);
float icsd_sin_hq_v2(float x);
void icsd_complex_mulf(float *input1, float *input2, float *out, int len);
void icsd_complex_mul_v2(int *input1, float *input2, float *out, int len);
void icsd_unwrap(float *pha1, float *pha2);
void icsd_complex_div_v2(float *input1, float *input2, float *out, int len);
float icsd_mean_cal(float *data0, u8 len);
float icsd_min_cal(float *data, u8 len);
double icsd_devide_float(float data_0, float data_1);
double icsd_devide_double(double data_0, double data_1);
void icsd_complex_mul_v2(int *input1, float *input2, float *out, int len);
void icsd_complex_muln(float *input1, float *input2, float *out, int len);
void icsd_complex_muln_2(float *input1, float *input2, float *input3, float *out, int len, float alpha);
void icsd_dsf8_2ch(int *ptr, int len, s16 *datah, s16 *datal);
void icsd_dsf8_4ch(int *ptr, int len, s16 *datah_l, s16 *datal_l, s16 *datah_r, s16 *datal_r);
void icsd_dsf4_2chto2ch(s16 *in1, s16 *in2, int len, s16 *datah, s16 *datal);
u8 icsd_cal_score(u8 valud, u8 *buf, u8 ind, u8 len);
void icsd_cic8_4ch(int *ptr, int len, int *dbufa, int *cbufa, s16 *output1, s16 *output2, s16 *output3, s16 *output4);
void icsd_cic8_2ch(int *ptr, int len, int *dbufa, int *cbufa, s16 *output1, s16 *output2);
void icsd_cic8_2ch_4order(int *ptr, int len, int *dbufa, int *cbufa, s16 *output1, s16 *output2);
float icsd_abs_float(float f);
void biquad2ab_common(float gain, float f, float q, float fs, float *a0, float *a1, float *a2, float *b0, float *b1, float *b2, u8 type);

void target_preprocess_v2(float *target, float *target_flt, int len);
int icsd_printf_off(const char *format, ...);
void DeAlorithm_enable();
void DeAlorithm_disable();
float db_diff_v2(float *in1, int in1_idx, float *in2, int in2_idx);
void target_cmp_out(void *_tar_param, float *target, float *freqz, float *sz, int *tight_degree, int ear_mem_en, float *alg_buf);
void icsd_aeq_fgq2eq(struct aeq_default_seg_tab *eq_par, float *iir_ab, float *hz, float *freq, float fs, int len);

extern const u8 ICSD_ANC_CPU;



void icsd_anc_HpEst_part1_v2(s16 *input0, s16 *input1, float *out0_sum, float *out1_sum, float *out2_sum, int *in_cur, float *in_cur0);
float db_diff(float *in1, int in1_idx, float *in2, int in2_idx);

void data_dsf_4to1(s16 *in, s16 *out, s16 olen, u8 iir_en);
u8 self_talk_dector(void *__hdl);




struct icsd_de_libfmt {
    int lib_alloc_size;  //算法ram需求大小
};
struct icsd_de_infmt {
    void *alloc_ptr;     //外部申请的ram地址
    int lib_alloc_size;  //算法ram需求大小
};
void icsd_de_get_libfmt(struct icsd_de_libfmt *libfmt);
void icsd_de_set_infmt(struct icsd_de_infmt *fmt);
void icsd_sde_get_libfmt(struct icsd_de_libfmt *libfmt);
void icsd_sde_set_infmt(struct icsd_de_infmt *fmt);
void icsd_de_malloc();
void icsd_de_free();
void icsd_sde_malloc();
void icsd_sde_free();


void icsd_common_ancdma_4ch_cic8(int *r_ptr, s16 *__wptr_dma1_h, s16 *__wptr_dma1_l, s16 *__wptr_dma2_h, s16 *__wptr_dma2_l, u16 inpoints);
#endif
