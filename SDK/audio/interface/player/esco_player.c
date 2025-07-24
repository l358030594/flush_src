#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".esco_player.data.bss")
#pragma data_seg(".esco_player.data")
#pragma const_seg(".esco_player.text.const")
#pragma code_seg(".esco_player.text")
#endif
#include "jlstream.h"
#include "esco_player.h"
#include "sdk_config.h"
#include "app_config.h"
#include "aec_ref_dac_ch_data.h"

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#include "icsd_adt_app.h"
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE
#include "rt_anc_app.h"
#endif

struct esco_player {
    u8 bt_addr[6];
    struct jlstream *stream;
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    u8 icsd_adt_state;
#endif
};

static struct esco_player *g_esco_player = NULL;
extern const int config_audio_dac_mix_enable;

static void esco_player_callback(void *private_data, int event)
{
    struct esco_player *player = g_esco_player;
    struct jlstream *stream = (struct jlstream *)private_data;

    switch (event) {
    case STREAM_EVENT_START:
        break;
    }
}

int esco_player_open(u8 *bt_addr)
{
    int err;
    struct esco_player *player;

    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"esco");
    if (uuid == 0) {
        return -EFAULT;
    }

    player = zalloc(sizeof(*player));
    if (!player) {
        return -ENOMEM;
    }

    player->stream = jlstream_pipeline_parse(uuid, NODE_UUID_ESCO_RX);
    if (!player->stream) {
        err = -ENOMEM;
        goto __exit0;
    }

#if TCFG_AUDIO_SIDETONE_ENABLE
    if (config_audio_dac_mix_enable) {
        set_aec_ref_dac_ch_name(SIDETONE_ESCO_DAC_NAME);
        aec_ref_dac_ch_data_read_init();
    }
#endif


#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE && AUDIO_RT_ANC_TIDY_MODE_ENABLE
    if (audio_anc_real_time_adaptive_reset(ADT_REAL_TIME_ADAPTIVE_ANC_TIDY_MODE, 1) && !audio_anc_real_time_adaptive_state_get())
#endif
    {
        /*通话前关闭adt*/
        player->icsd_adt_state = audio_icsd_adt_is_running();
        if (player->icsd_adt_state) {
            audio_icsd_adt_close(0, 1);
        }
    }
    printf("esco_player_open, icsd_adt_state %d", player->icsd_adt_state);

#endif

#if TCFG_ESCO_DL_CVSD_SR_USE_16K
    jlstream_node_ioctl(player->stream, NODE_UUID_BT_AUDIO_SYNC, NODE_IOC_SET_PRIV_FMT, TCFG_ESCO_DL_CVSD_SR_USE_16K);
#endif /*TCFG_ESCO_DL_CVSD_SR_USE_16K*/

#if TCFG_AI_TX_NODE_ENABLE
#if TCFG_DEC_OPUS_ENABLE
    /* struct stream_fmt ai_tx_fmt = {0}; */
    /* ai_tx_fmt.sample_rate = 16000; */
    /* ai_tx_fmt.coding_type = AUDIO_CODING_OPUS; */
    /* jlstream_node_ioctl(player->stream, NODE_UUID_AI_TX, NODE_IOC_SET_FMT, (int)&ai_tx_fmt); */
#endif
#if TCFG_DEC_JLA_V2_ENABLE
    struct stream_enc_fmt fmt = {0};
    fmt.coding_type = AUDIO_CODING_JLA_V2;
    fmt.sample_rate = 16000;
    fmt.frame_dms = 200;
    fmt.bit_rate = 16000;
    fmt.channel = 1;
    jlstream_ioctl(player->stream, NODE_IOC_SET_ENC_FMT, (int)&fmt);
#endif
#endif

    jlstream_set_callback(player->stream, player->stream, esco_player_callback);
    jlstream_set_scene(player->stream, STREAM_SCENE_ESCO);
    jlstream_node_ioctl(player->stream, NODE_UUID_SOURCE, NODE_IOC_SET_BTADDR, (int)bt_addr);

#if TCFG_NS_NODE_ENABLE
    jlstream_node_ioctl(player->stream, NODE_UUID_NOISE_SUPPRESSOR, NODE_IOC_SET_BTADDR, (int)bt_addr);
#endif

#if TCFG_DNS_NODE_ENABLE
    jlstream_node_ioctl(player->stream, NODE_UUID_DNS_NOISE_SUPPRESSOR, NODE_IOC_SET_BTADDR, (int)bt_addr);
#endif

    err = jlstream_start(player->stream);
    if (err) {
        goto __exit1;
    }

    memcpy(player->bt_addr, bt_addr, 6);
    g_esco_player = player;


    return 0;

__exit1:
    jlstream_release(player->stream);
__exit0:
    free(player);
    return err;
}

bool esco_player_runing()
{
    return g_esco_player != NULL;
}

int esco_player_get_btaddr(u8 *btaddr)
{
    if (g_esco_player) {
        memcpy(btaddr, g_esco_player->bt_addr, 6);
        return 1;
    }
    return 0;
}
int esco_player_suspend(u8 *bt_addr)
{
    if (g_esco_player && (memcmp(g_esco_player->bt_addr, bt_addr, 6) == 0)) {
        jlstream_ioctl(g_esco_player->stream, NODE_IOC_SUSPEND, 0);
    }
    return 0;
}
int esco_player_start(u8 *bt_addr)
{
    if (g_esco_player && (memcmp(g_esco_player->bt_addr, bt_addr, 6) == 0)) {
        jlstream_ioctl(g_esco_player->stream, NODE_IOC_START, 0);
    }
    return 0;
}

/*
	ESCO状态检测，
	param:btaddr 目标蓝牙地址, 传NULL则只检查耳机状态
 */
int esco_player_is_playing(u8 *btaddr)
{
    int cur_addr = 1;
    if (btaddr && g_esco_player) {	//当前蓝牙地址检测
        cur_addr = (!memcmp(btaddr, g_esco_player->bt_addr, 6));
    }
    if (g_esco_player && cur_addr) {
        return true;
    }
    return false;
}

void esco_player_set_ai_tx_node_func(int (*func)(u8 *, u32))
{
    struct esco_player *player = g_esco_player;

    if (player && player->stream) {
        jlstream_node_ioctl(player->stream, NODE_UUID_AI_TX, NODE_IOC_SET_PRIV_FMT, (int)func);
    }
}

void esco_player_close()
{
    struct esco_player *player = g_esco_player;

    if (!player) {
        return;
    }
#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
    u8 icsd_adt_state = player->icsd_adt_state;
#endif /*TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN*/

    jlstream_stop(player->stream, 0);
    jlstream_release(player->stream);

    free(player);
    g_esco_player = NULL;

#if TCFG_AUDIO_SIDETONE_ENABLE
    if (config_audio_dac_mix_enable) {
        aec_ref_dac_ch_data_read_exit();
    }
#endif

#if TCFG_AUDIO_ANC_ACOUSTIC_DETECTOR_EN
#if TCFG_AUDIO_ANC_REAL_TIME_ADAPTIVE_ENABLE && AUDIO_RT_ANC_TIDY_MODE_ENABLE
    if (audio_anc_real_time_adaptive_reset(ADT_REAL_TIME_ADAPTIVE_ANC_MODE, 1) && !audio_anc_real_time_adaptive_state_get())
#endif
    {
        printf("esco_player_close, icsd_adt_state %d", icsd_adt_state);
        if (icsd_adt_state) {
            audio_icsd_adt_open(0);
        }
    }
#endif

    jlstream_event_notify(STREAM_EVENT_CLOSE_PLAYER, (int)"esco");
}


