#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_online_debug.data.bss")
#pragma data_seg(".audio_online_debug.data")
#pragma const_seg(".audio_online_debug.text.const")
#pragma code_seg(".audio_online_debug.text")
#endif
#include "audio_online_debug.h"
#include "system/includes.h"
#include "generic/list.h"
#include "aud_mic_dut.h"
#include "aud_data_export.h"
#include "aud_spatial_effect_dut.h"
#include "app_config.h"

typedef struct {
    struct list_head parser_head;

} aud_online_ctx_t;
aud_online_ctx_t *aud_online_ctx = NULL;

int audio_online_debug_init(void)
{
    //aud_online_ctx = zalloc(sizeof(aud_online_ctx_t));
    if (aud_online_ctx) {
        //INIT_LIST_HEAD(&aud_online_ctx->parser_head);
    }
    return 0;
}

__INITCALL_BANK_CODE
int audio_online_debug_open()
{
    printf("%s", __func__);
    /*空间音频陀螺仪数据导出*/
#if (defined(TCFG_AUDIO_SPATIAL_EFFECT_ENABLE) && TCFG_AUDIO_SPATIAL_EFFECT_ENABLE &&  (TCFG_SENSOR_DATA_EXPORT_ENABLE == SENSOR_DATA_EXPORT_USE_SPP))
    aud_data_export_open();
#endif /*TCFG_SENSOR_DATA_EXPORT_ENABLE*/

    /*麦克风测试*/
#if (defined(TCFG_AUDIO_MIC_DUT_ENABLE) && (TCFG_AUDIO_MIC_DUT_ENABLE == 1))
    aud_mic_dut_open();
#endif/*TCFG_AUDIO_MIC_DUT_ENABLE*/

    /*空间音效在线调试*/
#if (defined(TCFG_AUDIO_SPATIAL_EFFECT_ENABLE) && TCFG_AUDIO_SPATIAL_EFFECT_ENABLE && TCFG_SPATIAL_EFFECT_ONLINE_ENABLE)
    aud_spatial_effect_dut_open();
#endif /*(defined(TCFG_SPATIAL_EFFECT_ONLINE_ENABLE) && (TCFG_SPATIAL_EFFECT_ONLINE_ENABLE == 1))*/
    return 0;
}
__initcall(audio_online_debug_open);

int audio_online_debug_close()
{
    if (aud_online_ctx) {

    }
    return 0;
}



