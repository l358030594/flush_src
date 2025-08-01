#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_anc_fade_ctr.data.bss")
#pragma data_seg(".audio_anc_fade_ctr.data")
#pragma const_seg(".audio_anc_fade_ctr.text.const")
#pragma code_seg(".audio_anc_fade_ctr.text")
#endif

/*
 ****************************************************************
 *							AUDIO ANC
 * File  : audio_anc_fade_ctr.c
 * By    : Junhao
 * Notes : ANC fade gain管理，支持多场景并行使用
 *
 ****************************************************************
 */

#include "app_config.h"

//目前仅支持BR28、BR50; BR36暂未支持
#if TCFG_AUDIO_ANC_ENABLE && (!(defined CONFIG_CPU_BR36))

#include "audio_anc.h"
#include "audio_anc_fade_ctr.h"

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
#include "rt_anc_app.h"
#endif

#if 0
#define anc_fade_log	printf
#else
#define anc_fade_log(...)
#endif/*log_en*/


struct anc_fade_context {
    struct list_head entry;
    enum anc_fade_mode_t mode;
    u8 ch;
    u16 fade_gain;
};

static LIST_HEAD(anc_fade_head);

static spinlock_t anc_fade_lock;

static void anc_fade_ctr_list_del(enum anc_fade_mode_t mode)
{
    struct anc_fade_context *ctx;
    if (list_empty(&anc_fade_head)) {
        return;
    }
    list_for_each_entry(ctx, &anc_fade_head, entry) {
        if (ctx->mode == mode) {
            goto __fade_del;
        }
    }
    return;
__fade_del:
    list_del(&ctx->entry);
    free(ctx);
}

static void anc_fade_ctr_list_add(enum anc_fade_mode_t mode, u8 ch, u16 gain)
{
    struct anc_fade_context *new_ctx = malloc(sizeof(struct anc_fade_context));
    new_ctx->mode = mode;
    //通道检测，有些芯片 fade_gain 不区分通道
    new_ctx->ch = anc_fade_ctr_ch_check(ch);
    new_ctx->fade_gain = gain;
    list_add_tail(&new_ctx->entry, &anc_fade_head);
}

static int anc_fade_ctr_update(enum anc_fade_mode_t mode, u8 ch, u16 gain)
{
    struct anc_fade_context *ctx;
    if (list_empty(&anc_fade_head)) {
        return 1;
    }
    list_for_each_entry(ctx, &anc_fade_head, entry) {
        if (ctx->mode == mode) {
            if (ctx->ch != ch) {
                printf("ERR!!!anc_fade_gain mode&ch is no \"one to one\" \n");
            }
            ctx->fade_gain = gain;
            return 0;
        }
    }
    return 1;
}

void audio_anc_fade_ctr_init(void)
{
    spin_lock_init(&anc_fade_lock);
}

void audio_anc_fade_ctr_set(enum anc_fade_mode_t mode, u8 ch, u16 gain)
{
    u16 lff_gain, lfb_gain, rff_gain, rfb_gain;
    struct anc_fade_context *ctx;
    u8 fade_en = anc_api_get_fade_en();
    /* anc_fade_log("fade in mode %d, gain %d\n", mode, gain); */

    lff_gain = AUDIO_ANC_FADE_GAIN_DEFAULT;
    lfb_gain = AUDIO_ANC_FADE_GAIN_DEFAULT;
    rff_gain = AUDIO_ANC_FADE_GAIN_DEFAULT;
    rfb_gain = AUDIO_ANC_FADE_GAIN_DEFAULT;

    spin_lock(&anc_fade_lock);
    if (gain == AUDIO_ANC_FADE_GAIN_DEFAULT) {
        anc_fade_ctr_list_del(mode);					//删除
    } else {
        if (anc_fade_ctr_update(mode, ch, gain)) {		//更新
            anc_fade_ctr_list_add(mode, ch, gain);		//新增
        }
    }

    //遍历链表
    if (!list_empty(&anc_fade_head)) {
        list_for_each_entry(ctx, &anc_fade_head, entry) {
            /* anc_fade_log("fade list : mode %d, gain %d\n", ctx->mode, ctx->fade_gain); */
            if ((ctx->ch & AUDIO_ANC_FADE_CH_LFF) && (ctx->fade_gain < lff_gain)) {
                lff_gain = ctx->fade_gain;
            }
            if ((ctx->ch & AUDIO_ANC_FADE_CH_LFB) && (ctx->fade_gain < lfb_gain)) {
                lfb_gain = ctx->fade_gain;
            }
            if ((ctx->ch & AUDIO_ANC_FADE_CH_RFF) && (ctx->fade_gain < rff_gain)) {
                rff_gain = ctx->fade_gain;
            }
            if ((ctx->ch & AUDIO_ANC_FADE_CH_RFB) && (ctx->fade_gain < rfb_gain)) {
                rfb_gain = ctx->fade_gain;
            }
        }
    }
    spin_unlock(&anc_fade_lock);

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
    struct rt_anc_fade_gain_ctr rt_anc_fade;
    rt_anc_fade.lff_gain = lff_gain;
    rt_anc_fade.lfb_gain = lfb_gain;
    rt_anc_fade.rff_gain = rff_gain;
    rt_anc_fade.rfb_gain = rfb_gain;
    audio_rtanc_fade_gain_suspend(&rt_anc_fade);
#endif

    anc_fade_log("fade lff %d, lfb %d, rff %d, rfb %d\n", lff_gain, lfb_gain, rff_gain, rfb_gain);
    audio_anc_fade_cfg_set(fade_en, 1, 0);
    audio_anc_fade(AUDIO_ANC_FADE_CH_LFF, lff_gain);
    audio_anc_fade(AUDIO_ANC_FADE_CH_LFB, lfb_gain);
    audio_anc_fade(AUDIO_ANC_FADE_CH_RFF, rff_gain);
    audio_anc_fade(AUDIO_ANC_FADE_CH_RFB, rfb_gain);

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
    audio_rtanc_fade_gain_resume();
#endif
}

void audio_anc_fade_ctr_del(enum anc_fade_mode_t mode)
{
    audio_anc_fade_ctr_set(mode, 0, AUDIO_ANC_FADE_GAIN_DEFAULT);
}

#endif/*TCFG_AUDIO_ANC_ENABLE*/

