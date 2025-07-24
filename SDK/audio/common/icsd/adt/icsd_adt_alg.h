#ifndef _ICSD_ADT_ALG_H
#define _ICSD_ADT_ALG_H

//==========ADJDCC=========================
typedef struct {
    s16 *refmic;
    s16 *errmic;
    u16 len;
} __adt_adjdcc_run_parm;

typedef struct {
    u8 result;
} __adt_adjdcc_output;

int icsd_adt_adjdcc_get_libfmt();
int icsd_adt_adjdcc_set_infmt(int _ram_addr, u8 TOOL_FUNCTION);
u8  icsd_adt_adjdcc_run(__adt_adjdcc_run_parm *_run_parm, __adt_adjdcc_output *_output);
//==========HOWL=========================
typedef struct {
    s16 *ref;
    s16 *anco;
} __adt_howl_run_parm;

typedef struct {
    u8 howl_output;
} __adt_howl_output;

int icsd_adt_howl_get_libfmt();
int icsd_adt_howl_set_infmt(int _ram_addr);
void icsd_adt_howl_run(__adt_howl_run_parm *_run_parm, __adt_howl_output *_output);
//==========AVC=========================
typedef struct {
    s16 *refmic;
    s16 *dac_data;
    u8  type;
} __adt_avc_run_parm;

typedef struct {
    int ctl_lvl;
    float spldb_iir;
} __adt_avc_output;

int icsd_adt_avc_get_libfmt(u8 type);
int icsd_adt_avc_set_infmt(int _ram_addr, u8 type);
void icsd_adt_avc_run(__adt_avc_run_parm *_run_parm, __adt_avc_output *_output);
void icsd_adt_avc_config_update_run(void *_config);
void icsd_adt_tidy_avc_alg_run();
//==========RTANC========================
typedef struct {
    u8  dma_ch;
    s16 *inptr_h;
    s16 *inptr_l;
    float *out0_sum;
    float *out1_sum;
    float *out2_sum;
    int   *fft_ram;
    float *hpest_temp;
} __adt_anc_part1_parm;

typedef struct {
    u8  dma_ch;
    float *out0_sum;
    float *out1_sum;
    float *out2_sum;
    float *sz_out0_sum;
    float *sz_out1_sum;
    float *sz_out2_sum;
    float *szpz_out;
} __adt_rtanc_part2_parm;

int  icsd_adt_rtanc_get_libfmt(u8 rtanc_type);
int  icsd_adt_rtanc_set_infmt(int _ram_addr, void *rtanc_tool, u8 rtanc_type, u8 TOOL_FUNCTION);
void icsd_adt_alg_rtanc_run_part1(__adt_anc_part1_parm *_part1_parm);
void icsd_adt_alg_rtanc_part2_parm_init();
u8   icsd_adt_alg_rtanc_run_part2(__adt_rtanc_part2_parm *_part2_parm);
u8 	 icsd_adt_alg_rtanc_get_wind_lvl();
u8   icsd_adt_alg_rtanc_get_adjdcc_result();
void icsd_adt_alg_rtanc_part1_reset();
void icsd_adt_rtanc_alg_output(void *rt_param_l, void *rt_param_r);
void icsd_adt_rtanc_fadegain_update_run(void *_param);
//==========EIN========================
typedef struct {
    s16 *adc_ref_buf;
    s16 *adc_err_buf;
    s16 *inear_sz_spko_buf;
    s16 *inear_sz_emic_buf;
    s16 *inear_sz_dac_buf;
    u8 resume_mode;
    u8 anc_onoff;
    u8 run_mode;
} __adt_ein_run_parm;

typedef struct {
    u8 ein_output;
    void *ein_alg_bt_inf;
    u16 ein_alg_bt_len;
} __adt_ein_output;

int  icsd_adt_ein_get_libfmt();
int  icsd_adt_ein_set_infmt(int _ram_addr, u8 ein_state);
void icsd_adt_alg_ein_run(__adt_ein_run_parm *_run_parm, __adt_ein_output *_output);
void icsd_EIN_output(u8 ein_state);
//==========WAT========================
typedef struct {
    s16 *data_1_ptr;
    u8 resume_mode;
    u8 anc_onoff;
    float err_gain;
} __adt_wat_run_parm;

typedef struct {
    u8 wat_output;
    u8 get_max_range;
    int max_range;
} __adt_wat_output;

int  icsd_adt_wat_get_libfmt();
int  icsd_adt_wat_set_infmt(int _ram_addr);
void icsd_adt_alg_wat_run(__adt_wat_run_parm *_run_parm, __adt_wat_output *_output);
void icsd_adt_alg_wat_ram_clean();
//==========WIND========================
#if ICSD_WIND_LIB
#else//没有风噪库时使用该配置
#define ICSD_WIND_HEADSET           1
#define ICSD_WIND_TWS		        2
#define ICSD_WIND_LFF_TALK          1
#define ICSD_WIND_LFF_RFF           2
#define ICSD_WIND_LFF_LFB           3
#define ICSD_WIND_LFF_LFB_TALK      4
#define ICSD_WIND_RFF_TALK      	5
#endif

typedef struct {
    u8  f2wind;
    float f2pwr;
    float f2curin[48];
    int idx;
    u8  wind_id;
} __win_part1_out;

typedef struct {
    u8  f2wind;
    float f2pwr;
    float f2in_cur3t[48];
    int idx;
    u8  wind_id;
} __win_part1_out_rx;

typedef struct {
    s16 *data_1_ptr;
    s16 *data_2_ptr;
    s16 *data_3_ptr;
    u8 anc_mode;
    u8 wind_ft;
    __win_part1_out *part1_out;
    __win_part1_out_rx *part1_out_rx;
} __adt_win_run_parm;

typedef struct {
    u8 wind_lvl;
    void *wind_alg_bt_inf;
    u16 wind_alg_bt_len;
} __adt_win_output;

int  icsd_adt_wind_get_libfmt();
int  icsd_adt_wind_set_infmt(int _ram_addr, u8 TOOL_FUNCTION);
void icsd_adt_alg_wind_run(__adt_win_run_parm *_run_parm, __adt_win_output *_output);
void icsd_adt_alg_wind_run_part1(__adt_win_run_parm *_run_parm, __adt_win_output *_output);
void icsd_adt_alg_wind_run_part2(__adt_win_run_parm *_run_parm, __adt_win_output *_output);
int  icsd_adt_alg_wind_ssync(__win_part1_out *_part1_out_tx);
void icsd_adt_wind_master_tx_data_sucess(void *_data);
void icsd_adt_wind_master_rx_data(void *_data);
void icsd_adt_wind_slave_tx_data_sucess(void *_data);
void icsd_adt_wind_slave_rx_data(void *_data);
u8   icsd_adt_wind_data_sync_en();
void icsd_wind_run_part2_cmd();
void icsd_wind_data_sync_master_cmd();
void *icsd_adt_wind_part1_rx();
u8 	 icsd_adt_win_get_tlkmic_en();
//==========VDT========================
typedef struct {
    s16 *refmic;
    s16 *errmic;
    s16 *tlkmic;
    s16 *dmah;
    s16 *dmal;
    s16 *dac_data;
    float fbgain;
    float tfbgain;
    u8 mic_num;
    u8 anc_mode_ind;
} __adt_vdt_run_parm;

typedef struct {
    u8 voice_state;
} __adt_vdt_output;

int  icsd_adt_vdt_get_libfmt();
int  icsd_adt_vdt_set_infmt(int _ram_addr);
void icsd_adt_alg_rtanc_de_run_l();
void icsd_adt_alg_vdt_run(__adt_vdt_run_parm *_run_parm, __adt_vdt_output *_output);
void icsd_adt_vdt_data_init(u8 _anc_mode_ind, float ref_mgain, float err_mgain, float tlk_mgain);
void icsd_adt_alg_rtanc_adjdcc_flag_set(u8 flag);
u8 icsd_adt_alg_adjdcc_trigger_update(u8 env_level, float *table);
u8 icsd_adt_alg_rtanc_adjdcc_flag_get();
#endif
