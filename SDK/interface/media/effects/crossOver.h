#ifndef CROSSOVER_H
#define CROSSOVER_H

#include "effects/AudioEffect_DataType.h"

#ifdef WIN32

#define AT_CROSSOVER(x)
#define AT_CROSSOVER_CODE
#define AT_CROSSOVER_SPARSE_CODE
#define AT_CROSSOVER_CONST
#define AT_CROSSOVER_SPARSE_CONST

#else

#define AT_CROSSOVER(x)					__attribute__((section(#x)))
#define AT_CROSSOVER_CODE				AT_CROSSOVER(.crossOver.text.cache.L1)
#define AT_CROSSOVER_SPARSE_CODE		AT_CROSSOVER(.crossOver.text)
#define	AT_CROSSOVER_CONST				AT_CROSSOVER(.crossOver.text.cache.L2.const)
#define AT_CROSSOVER_SPARSE_CONST		AT_CROSSOVER(.crossOver.text.const)
#endif


typedef struct _Filter {
    int channel;
    int nSection;
    int *SOSMatrix;
    int *SOSMem;
    int IndataInc;
    int OutdataInc;

} Filter;

typedef struct _CrossOver {
    Filter filter[3];
    int N;
    int channel;
    int way_num;
    int nSection;
    int sampleRate;
    /*
    int low_fc[2];
    int high_fc[2];
    */
    int low_fc;
    int high_fc;
    int *low_SOSMatrix;       // 低频段系数-a11,-a12,b11,b12,-a21,-a22,b21,b22....
    int *high_SOSMatrix;//高频段系数
    int *band_SOSMatrix;//中频段系数
    int *tempbuf;
    int pool[0];
} crossOver;



typedef struct _CrossOverParam {
    int way_num;    //段数  2或者3
    int N;          //阶数	2或者4
    int low_freq;   //低频分频点
    int high_freq;  //高频分频点 当way_num = 3时才会使用
    int SampleRate; //采样率
    int channel;    //通道数
    af_DataType pcm_info;
} CrossOverParam;

int get_crossOver_buf(CrossOverParam *param);  //buf 大小与通道数(channel) 阶数(N) 段数(way_num)有关
int get_crossOver_tempbuf();
void crossOver_init(void *workBuf, void *tempbuf, CrossOverParam *param);
void crossOver_update(void *workBuf, void *tempbuf, CrossOverParam *param);
int crossOver_run(void *work_buf, int *in, int *low_out, int *mid_out, int *high_out, int npoint_per_channel);

int crossoverCoff_init(void *workbuf, int low_fc, int high_fc, int sampleRate, int N, int way_num);
int crossoverCoff_run(void *workbuf);
/*

int get_crossOver_buf(int way_num, int N);
int get_crossOver_tempbuf();
void set_crossOver_tempbuf(void *workBuf, void *tempbuf);
void crossOver_init(void *workBuf, int *low_freq, int *high_freq, int sampleRate, int N, int way_num, int channel);
void crossOver_update(void *workBuf, int *low_freq, int *high_freq, int sampleRate, int N, int way_num, int channel);
int crossOver_run(void *work_buf, int *in, int *low_out, int *band_out, int *high_out, int npoint_per_channel);

int crossoverCoff_init(void *workbuf, int *low_fc, int *high_fc, int sampleRate, int N, int way_num);
int crossoverCoff_run(void *workbuf);
*/

#endif // !CROSSOVER_H


