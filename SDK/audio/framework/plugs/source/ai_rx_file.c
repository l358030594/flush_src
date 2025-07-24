
#include "source_node.h"
#include "app_config.h"
#include "reference_time.h"
#include "ai_rx_player.h"
#include "rcsp_translator.h"


#if  TCFG_AI_RX_NODE_ENABLE

struct ai_rx_file_handle {
    u8 start;
    void *bt_addr;
    u8 source;
    u8 reference;
    struct stream_node *node;
    u16 play_latency; //us
    struct ai_rx_player_param param;
};


extern u32 bt_audio_conn_clock_time(void *addr);
static enum stream_node_state ai_rx_get_frame(void *file, struct stream_frame **pframe)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)file;
    struct stream_frame *frame;
    struct translator_recv_audio_frame *trans_frame = NULL;

#if RCSP_MODE && RCSP_ADV_TRANSLATOR
    trans_frame = JL_rcsp_translator_recv_ch_get_frame((u8)hdl->source);
#endif
    //cppcheck-suppress knownConditionTrueFalse
    if (trans_frame == NULL) {
        *pframe = NULL;
        return NODE_STA_RUN | NODE_STA_SOURCE_NO_DATA;
    }

    frame = jlstream_get_frame(hdl->node->oport, trans_frame->size);
    if (frame == NULL) {
        *pframe = NULL;
        return NODE_STA_RUN;
    }

    memcpy(frame->data, trans_frame->buf, trans_frame->size);
    frame->len = trans_frame->size;
    if (hdl->reference) {
        frame->timestamp = ((trans_frame->timestamp + hdl->play_latency) & 0xfffffff) * TIMESTAMP_US_DENOMINATOR;
        frame->flags |= FRAME_FLAG_TIMESTAMP_ENABLE | FRAME_FLAG_UPDATE_TIMESTAMP;
    }
#if RCSP_MODE && RCSP_ADV_TRANSLATOR
    JL_rcsp_translator_recv_ch_free_frame((u8)hdl->source, trans_frame);
#endif
    *pframe = frame;

    return NODE_STA_RUN;

}

static void *ai_rx_file_init(void *priv, struct stream_node *node)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)zalloc(sizeof(struct ai_rx_file_handle));

    hdl->node = node;
    node->type |= NODE_TYPE_FLOW_CTRL;//NODE_TYPE_IRQ;

    return hdl;
}

static int ai_rx_file_set_bt_addr(struct ai_rx_file_handle *hdl, void *bt_addr)
{
    hdl->bt_addr = (void *)bt_addr;

    return 0;
}
static void ai_rx_tick_handler(void *priv, u8 source)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)priv;

    if (hdl->start && (hdl->node->state & NODE_STA_SOURCE_NO_DATA)) {
        jlstream_wakeup_thread(NULL, hdl->node, NULL);
    }
}

static int ai_rx_file_get_fmt(struct ai_rx_file_handle *hdl, struct stream_fmt *fmt)
{
#if 0
    fmt->coding_type = AUDIO_CODING_OPUS;
    fmt->sample_rate = 16000;
    fmt->channel_mode = AUDIO_CH_MIX;
#else
    fmt->coding_type = hdl->param.coding_type;
    fmt->sample_rate = hdl->param.sample_rate;
    fmt->channel_mode = hdl->param.channel_mode;
    fmt->frame_dms = hdl->param.frame_dms;
    fmt->bit_rate = hdl->param.bit_rate;
    /* r_printf(" 0x%x, %d, %d, %d, %d\n",fmt->coding_type,fmt->sample_rate,fmt->channel_mode,fmt->frame_dms,fmt->bit_rate) */
#endif
    return 0;
}

static int ai_rx_file_start(struct ai_rx_file_handle *hdl)
{
    if (hdl->start) {
        return 0;
    }

    hdl->start = 1;
#if RCSP_MODE && RCSP_ADV_TRANSLATOR
    JL_rcsp_translator_set_decode_resume_handler(hdl->source, hdl, ai_rx_tick_handler);
#endif

    if (!jlstream_is_contains_node_from(hdl->node, NODE_UUID_BT_AUDIO_SYNC)) {
        return 0;
    }

    hdl->play_latency = 50 * 1000;//设置延时us

    hdl->reference = audio_reference_clock_select(hdl->bt_addr, 0);

    return 0;
}

static int ai_rx_file_stop(struct ai_rx_file_handle *hdl)
{
    if (!hdl->start) {
        return 0;
    }
    hdl->start = 0;
#if RCSP_MODE && RCSP_ADV_TRANSLATOR
    JL_rcsp_translator_set_decode_resume_handler(hdl->source, hdl, NULL);
#endif
    if (hdl->reference) {
        audio_reference_clock_exit(hdl->reference);
    }
    return 0;
}

static int ai_rx_file_ioctl(void *file, int cmd, int arg)
{
    int err = 0;
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)file;

    switch (cmd) {
    case NODE_IOC_SET_BTADDR:
        ai_rx_file_set_bt_addr(hdl, (void *)arg);
        break;
    case NODE_IOC_SET_FMT:
        memcpy(&hdl->param, (struct ai_rx_player_param *)arg, sizeof(struct ai_rx_player_param));
        break;
    case NODE_IOC_GET_FMT:
        ai_rx_file_get_fmt(hdl, (struct stream_fmt *)arg);
        break;
    case NODE_IOC_START:
        ai_rx_file_start(hdl);
        break;
    case NODE_IOC_SET_PARAM:
        hdl->source = (u8)arg;
        break;
    case NODE_IOC_SUSPEND:
    case NODE_IOC_STOP:
        ai_rx_file_stop(hdl);
        break;
    }

    return 0;
}

static void ai_rx_file_release(void *file)
{
    struct ai_rx_file_handle *hdl = (struct ai_rx_file_handle *)file;

    free(hdl);
}

REGISTER_SOURCE_NODE_PLUG(AI_RX_file_plug) = {
    .uuid       = NODE_UUID_AI_RX,
    .init       = ai_rx_file_init,
    .get_frame  = ai_rx_get_frame,
    .ioctl      = ai_rx_file_ioctl,
    .release    = ai_rx_file_release,
};


#endif /*TCFG_AI_RX_NODE_ENABLE*/
