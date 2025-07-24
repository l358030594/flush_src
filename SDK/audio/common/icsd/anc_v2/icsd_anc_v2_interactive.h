
#ifndef _ICSD_ANC_INTERACTIVE_H_
#define _ICSD_ANC_INTERACTIVE_H_

#include "asm/anc.h"
#include "icsd_anc_v2.h"

struct anc_ear_adaptive_train_cfg {
    u8 ff_yorder;
    float ff_gain;
    double *ff_coeff;
};

struct icsd_anc_init_cfg {
    u8 tws_sync;         	 		/*左右耳平衡使能*/
    u8 seq;							/*seq计数*/
    audio_anc_t *param;
};

extern anc_packet_data_t *anc_adaptive_data;

u8 anc_ear_adaptive_train_set_bypass(u8 ff_yorder, float bypass_vol, s8 bypass_sign);
u8 anc_ear_adaptive_train_set_bypass_off(void);
u8 anc_ear_adaptive_train_set_pnc(u8 ff_yorder, float bypass_vol, s8 bypass_sign);
struct anc_ear_adaptive_train_cfg *anc_ear_adaptive_train_cfg_get(void);

void anc_ear_adaptive_init(audio_anc_t *param);
void anc_ear_adaptive_cmd_handler(audio_anc_t *param, int *msg);
void anc_ear_adaptive_tone_play_start(void);
void anc_ear_adaptive_tone_play_err_cb(void);
void anc_ear_adaptive_tone_play_cb(void);
void anc_ear_adaptive_develop_set(u8 mode);
void anc_user_train_cb(u8 mode, u8 forced_exit);
int audio_anc_adaptive_data_read(void);
void audio_anc_ear_adaptive_param_init(audio_anc_t *param);
void audio_anc_adaptive_data_packet(struct icsd_anc_v2_tool_data *TOOL_DATA);
void audio_anc_param_map_init(void);
void audio_anc_param_map(u8 coeff_en, u8 gain_en);
void anc_ear_adaptive_power_off_save_data(void);
u8 anc_ear_adaptive_tws_sync_en_get(void);
void anc_ear_adaptive_seq_set(u8 seq);
u8 anc_ear_adaptive_seq_get(void);
void anc_ear_adaptive_sz_output(__afq_output *output);
void *anc_ear_adaptive_iir_get(void);

/* 判断当前是否处于训练中 */
u8 anc_ear_adaptive_busy_get(void);
/* 耳道自适应训练结束 */
void anc_ear_adaptive_mode_end(void);


/* ANC自适应训练处理回调函数 */
void audio_anc_adaptive_result_callback(u8 result_l, u8 result_r);

/*自适应模式-重新检测
 * param: tws_sync_en          1 TWS同步自适应，支持TWS降噪平衡，需左右耳一起调用此接口
 *                             0 单耳自适应, 不支持TWS降噪平衡，可TWS状态下单耳自适应
 */
int audio_anc_mode_ear_adaptive(u8 tws_sync_en);

/*
   强制中断自适应
   param: default_flag		1 退出后恢复默认ANC效果； 0 退出后保持ANC_OFF(避免与下一个切模式流程冲突)
   		  wait_pend			1 阻塞等待自适应退出完毕；0 不等待直接退出
 */
void anc_ear_adaptive_forced_exit(u8 default_flag, u8 wait_pend);

/*
   自适应/普通参数切换(只切换参数，不更新效果)
	param:  mode 		0 使用普通参数; 1 使用自适应参数
			tone_play 	0 不播放提示音；1 播放提示音
 */
int audio_anc_coeff_adaptive_set(u32 mode, u8 tone_play);

/*
   自适应/普通参数切换(切换参数，并立即更新效果)
   前置条件：需在 "ANC_ON" 模式下调用;
	param:  mode 		0 使用普通参数; 1 使用自适应参数
			tone_play 	0 不播放提示音；1 播放提示音
 */
int audio_anc_coeff_adaptive_update(u32 mode, u8 tone_play);

/*ANC滤波器模式循环切换*/
int audio_anc_coeff_adaptive_switch();

/*当前ANC滤波器模式获取 0:普通参数 1:自适应参数*/
int audio_anc_coeff_mode_get(void);

void icsd_set_adaptive_run_busy(u8 busy);

int audio_anc_ear_adaptive_a2dp_suspend_cb(void);

int anc_ear_adaptive_close(void);

int audio_anc_ear_adaptive_tool_data_get(u8 **buf, u32 *len);

#endif/*_ICSD_ANC_INTERACTIVE_H_*/

