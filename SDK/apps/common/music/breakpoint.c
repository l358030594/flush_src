#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".breakpoint.data.bss")
#pragma data_seg(".breakpoint.data")
#pragma const_seg(".breakpoint.text.const")
#pragma code_seg(".breakpoint.text")
#endif
#include "breakpoint.h"
#include "app_config.h"
#include "vm.h"
#include "syscfg_id.h"

#if TCFG_MUSIC_PLAYER_ENABLE

#if (defined(TCFG_DEC_APE_ENABLE) && (TCFG_DEC_APE_ENABLE))
#define BREAKPOINT_DATA_LEN   (2036 + 4)
#elif (defined(TCFG_DEC_FLAC_ENABLE) && (TCFG_DEC_FLAC_ENABLE))
#define BREAKPOINT_DATA_LEN	688
#elif (TCFG_DEC_M4A_ENABLE || TCFG_DEC_ALAC_ENABLE)
#define BREAKPOINT_DATA_LEN	536
#endif

#ifdef CONFIG_CPU_BR18
#ifndef BREAKPOINT_DATA_LEN
#define BREAKPOINT_DATA_LEN	80
#endif
#else
#ifndef BREAKPOINT_DATA_LEN
#define BREAKPOINT_DATA_LEN	32
#endif
#endif

#define BREAKPOINT_VAILD_SIZE(p)		((p)->dbp.len + ((u32)(p)->dbp.data - (u32)(p)))

struct __breakpoint *breakpoint_handle_creat(void)
{
    struct __breakpoint *bp = (struct __breakpoint *)zalloc(sizeof(struct __breakpoint) + BREAKPOINT_DATA_LEN);
    bp->dbp.data_len = BREAKPOINT_DATA_LEN;
    return bp;
}

void breakpoint_handle_destroy(struct __breakpoint **bp)
{
    if (bp && *bp) {
        free(*bp);
        *bp = NULL;
    }
}

bool breakpoint_vm_read(struct __breakpoint *bp, char *logo)
{
    if (bp == NULL || logo == NULL) {
        return false;
    }
    u16 vm_id;
    if (!strcmp(logo, "sd0")) {
        vm_id = CFG_SD0_BREAKPOINT0;
    } else if (!strcmp(logo, "sd1")) {
        vm_id = CFG_SD1_BREAKPOINT0;
    } else if (!strcmp(logo, "udisk0")) {
        vm_id = CFG_USB_BREAKPOINT0;
    } else {
        return false;
    }
    int rlen = syscfg_read(vm_id, (void *)bp, sizeof(struct __breakpoint) + BREAKPOINT_DATA_LEN);
    if (rlen < 0) {
        memset(bp, 0, sizeof(struct __breakpoint) + BREAKPOINT_DATA_LEN);
        bp->dbp.data_len = BREAKPOINT_DATA_LEN;
        return false;
    } else {
        /* put_buf((u8*)bp,36); */
    }
    return true;
}

void breakpoint_vm_write(struct __breakpoint *bp, char *logo)
{
    if (bp == NULL || logo == NULL) {
        return ;
    }
    u16 vm_id;
    if (!strcmp(logo, "sd0")) {
        vm_id = CFG_SD0_BREAKPOINT0;
    } else if (!strcmp(logo, "sd1")) {
        vm_id = CFG_SD1_BREAKPOINT0;
    } else if (!strcmp(logo, "udisk0")) {
        vm_id = CFG_USB_BREAKPOINT0;
    } else {
        return ;
    }
    int ret = syscfg_write(vm_id, (void *)bp, BREAKPOINT_VAILD_SIZE(bp));
    printf("%s %s ok [%d]\n", logo, __FUNCTION__, ret);
}





#endif
