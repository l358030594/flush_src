#ifndef _ICSD_ANC_USER_H_
#define _ICSD_ANC_USER_H_

#include "typedef.h"

#include "esco_player.h"
#include "adc_file.h"
#include "audio_adc.h"
#include "audio_anc_fade_ctr.h"

typedef struct {
    u16 mic_ch_sel;
    u16 sample_rate;
    u16 adc_irq_points;
    u16 adc_buf_num;
    u16 mic_gain[AUDIO_ADC_MAX_NUM];
} audio_mic_param_t;

enum {
    ICSD_ADT_TONE_NUM0 = 0,
    ICSD_ADT_TONE_NUM1,
    ICSD_ADT_TONE_NUM2,
    ICSD_ADT_TONE_NUM3,
    ICSD_ADT_TONE_NUM4,
    ICSD_ADT_TONE_NUM5,
    ICSD_ADT_TONE_NUM6,
    ICSD_ADT_TONE_NUM7,
    ICSD_ADT_TONE_NUM8,
    ICSD_ADT_TONE_NUM9,

    ICSD_ADT_TONE_NORMAL,
    ICSD_ADT_TONE_SPKCHAT_ON,
    ICSD_ADT_TONE_SPKCHAT_OFF,
    ICSD_ADT_TONE_WCLICK_ON,
    ICSD_ADT_TONE_WCLICK_OFF,
    ICSD_ADT_TONE_WINDDET_ON,
    ICSD_ADT_TONE_WINDDET_OFF,
};

/* #define TCFG_AUDIO_ANCL_FF_MIC  ANCL_FF_MIC */
/* #define TCFG_AUDIO_ANCL_FB_MIC  ANCL_FB_MIC */
/* #define TCFG_AUDIO_ANCR_FF_MIC  ANCL_FF_MIC */
/* #define TCFG_AUDIO_ANCR_FB_MIC  ANCL_FB_MIC */

int audio_mic_en(u8 en, audio_mic_param_t *mic_param,
                 void (*data_handler)(void *priv, s16 *data, int len));

int icsd_adt_tone_play_callback(u8 index, void (*evt_handler)(void *priv), void *priv);
int icsd_adt_tone_play(u8 index);

void icsd_adt_clock_add();
void icsd_adt_clock_del();
void icsd_set_clk();

void icsd_anc_fade_set(enum anc_fade_mode_t mode, int gain);

void *icsd_adt_src_init(int in_rate, int out_rate, int (*handler)(void *, void *, int));
void icsd_adt_src_write(void *data, int len, void *resample);
void icsd_adt_src_push(void *resample);
void icsd_adt_src_close(void *resample);

void icsd_set_tws_t_sniff(u16 slot);
void icsd_bt_sniff_set_enable(u8 en);
void icsd_adt_tx_unsniff_req();

void icsd_anc_src_push();
void icsd_anc_src_init(int in_rate, int out_rate, int (*handler)(void *, void *, int));
void icsd_anc_src_write(void *data, int len);
void icsd_audio_adc_mic_close();

u8 icsd_get_talk_mic_ch(void);
u8 icsd_get_ref_mic_L_ch(void);
u8 icsd_get_fb_mic_L_ch(void);
u8 icsd_get_ref_mic_R_ch(void);
u8 icsd_get_fb_mic_R_ch(void);
u8 icsd_get_esco_mic_en_map(void);

#endif
