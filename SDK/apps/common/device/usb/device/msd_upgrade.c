#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "usb/device/msd.h"
#include "usb/scsi.h"
#include "usb_config.h"
#include "app_config.h"
#include "cpu.h"
#include "asm/debug.h"

#define WRITE_FLASH                     0xFB
#define READ_FLASH                      0xFD
#define OTHER_CMD                       0xFC
typedef enum {
    UPGRADE_NULL = 0,
    UPGRADE_USB_HARD_KEY,
    UPGRADE_USB_SOFT_KEY,
    UPGRADE_UART_KEY,
} UPGRADE_STATE;

extern void nvram_set_boot_state(u32 state);
extern void hw_mmu_disable(void);
extern void ram_protect_close(void);

AT(.volatile_ram_code)
void go_mask_usb_updata()
{
#ifdef CONFIG_CPU_BR18
    gpio_set_die(IO_PORTD_02, 0);
    gpio_set_die(IO_PORTB_04, 0);
    cpu_reset();

#else
    usb_g_hold(0);
#if USB_MAX_HW_NUM > 1
    usb_g_hold(1);
#endif
    local_irq_disable();
    ram_protect_close();
    //TODO
    //hw_mmu_disable();
    nvram_set_boot_state(UPGRADE_USB_SOFT_KEY);
    cpu_reset();
#endif

    while (1);
}
#if TCFG_PC_UPDATE

u32 _usb_bulk_rw_test(const struct usb_device_t *usb_device, struct usb_scsi_cbw *cbw);

u32 private_scsi_cmd(const struct usb_device_t *usb_device, struct usb_scsi_cbw *cbw)
{
    /* if (_usb_bulk_rw_test(usb_device, cbw)) { */
    /*     return TRUE;                          */
    /* }                                         */

    switch (cbw->operationCode) {
//////////////////////Boot Loader Custom CMD
    case WRITE_FLASH:
    case READ_FLASH:
    case OTHER_CMD:
        log_d("goto mask pc mode\n");
        go_mask_usb_updata();
        break;

    default:

        return FALSE;
    }

    return TRUE;
}
#else
u32 private_scsi_cmd(const struct usb_device_t *usb_device, struct usb_scsi_cbw *cbw)
{
    return FALSE;
}
#endif

