#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".audio_ns.data.bss")
#pragma data_seg(".audio_ns.data")
#pragma const_seg(".audio_ns.text.const")
#pragma code_seg(".audio_ns.text")
#endif
/*
 ******************************************************************
 *				Noise Suppress Module(降噪模块)
 *Notes:
 *(1)支持降噪的数据采样率：8k/16k
 *(2)增加多5k左右代码
 *(3)内存消耗跟数据采样率有关，具体可通过以下接口查询：
 *   int ns_mem_size = noise_suppress_mem_query(&ans->ns_para);
 ******************************************************************
 */

#include "audio_ns.h"
#include "overlay_code.h"
#include "audio_cvp_def.h"

#ifdef CONFIG_MEDIA_NEW_ENABLE

/*16k采样率时，处理帧长320
   8k采样率时，处理帧长160*/

/*数据写卡导出使能
 #define TCFG_AUDIO_DATA_EXPORT_DEFINE	AUDIO_DATA_EXPORT_VIA_UART时才可用
 不能和通话上行写卡同时使用，即CONST_AEC_EXPORT = 0*/
const u8 CONST_NS_DATA_EXPORT = 0;

extern void aec_code_movable_load(void);
extern void aec_code_movable_unload(void);

/*
*********************************************************************
*                  NoiseSuppress Process
* Description: 降噪处理主函数
* Arguments  : ns	降噪句柄
*			   in	输入数据
*			   out	输出数据
*			   len  输入数据长度
* Return	 : 降噪输出长度
* Note(s)    : 由于降噪是固定处理帧长的，所以如果输入数据长度不是降噪
*			   帧长整数倍，则某一次会输出0长度，即没有输出
*********************************************************************
*/
int audio_ns_run(void *ns, short *in, short *out, u16 len)
{
    if (ns == NULL) {
        return len;
    }
#if 0
    int wlen = cbuf_write(&ns->cbuf, in, len);
    if (wlen != len) {
        printf("ns_cbuf full\n");
    }
    if (ns->cbuf.data_len >= NS_FRAME_SIZE) {
        cbuf_read(&ns->cbuf, out, NS_FRAME_SIZE);
        putchar('.');
        noise_suppress_run(out, out, NS_FRAME_POINTS);
        return NS_FRAME_SIZE;
    } else {
        return 0;
    }
#else
    //putchar('.');
    int nOut = noise_suppress_run(in, out, (len / 2));
    return (nOut << 1);
#endif
}

/*
 *********************************************************************
 *                  	Noise Suppress Open
 * Description: 初始化降噪模块
 * Arguments  : sr				数据采样率
 *			   mode				降噪模式(0,1,2:越大越耗资源，效果越好)
 *			   NoiseLevel	 	初始噪声水平(评估初始噪声，加快收敛)
 *			   AggressFactor	降噪强度(越大越强:1~2)
 *			   MinSuppress		降噪最小压制(越小越强:0~1)
 * Return	 : 降噪模块句柄
 * Note(s)    : (1)采样率只支持8k、16k
 *			   (2)如果内存不足，可以考虑wideband强制为0，即只做窄带降噪
 *				  ans-ns_para.wideband = 0;
 *********************************************************************
 */
void *audio_ns_open(u16 sr, u8 mode, float NoiseLevel, float AggressFactor, float MinSuppress)
{
    void *ns = NULL;
    overlay_load_code(OVERLAY_AEC);
    aec_code_movable_load();
    audio_ans_t *ans = zalloc(sizeof(audio_ans_t));
    //cbuf_init(&ans->cbuf, ans->in, sizeof(ans->in));

    ans->ns_para.wideband = (sr == 16000) ? 1 : 0;
    ans->ns_para.mode = mode;
    ans->ns_para.NoiseLevel = NoiseLevel;
    ans->ns_para.AggressFactor = AggressFactor;
    ans->ns_para.MinSuppress = MinSuppress;
    printf("ns wideband:%d\n", ans->ns_para.wideband);
    //int ns_mem_size = noise_suppress_mem_query(&ans->ns_para);
    //printf("ns mem_size:%d\n", ns_mem_size);
    noise_suppress_open(&ans->ns_para);
    float lowcut = -60.f;
    noise_suppress_config(NS_CMD_LOWCUTTHR, 0, &lowcut);

    ns = (void *)ans;
    printf("audio_ns_open ok\n");
    return ns;
}

/*
 *********************************************************************
 *                  	Noise Suppress Close
 * Description: 关闭降噪模块
 * Arguments  : ns 降噪模块句柄
 * Return	 : 0 成功 其他 失败
 * Note(s)    : None.
 *********************************************************************
 */
int audio_ns_close(void *ns)
{
    noise_suppress_close();
    if (ns) {
        free(ns);
    }

    aec_code_movable_unload();

    printf("esco_ans_close ok\n");
    return 0;
}

#endif /*CONFIG_MEDIA_NEW_ENABLE*/

