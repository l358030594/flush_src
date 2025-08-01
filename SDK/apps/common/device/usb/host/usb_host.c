#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "app_config.h"
#include "device_drive.h"
/* #include "os/os_compat.h" */
#include "system/event.h"
#include "clock_manager/clock_manager.h"
#if TCFG_USB_HOST_ENABLE
#include "usb_config.h"
#include "usb/host/usb_host.h"
#include "usb/usb_phy.h"
#include "usb_ctrl_transfer.h"
#include "usb_storage.h"
#include "adb.h"
#include "aoa.h"
#include "hid.h"
#include "audio.h"
#include "usb/usb_task.h"

#if TCFG_USB_APPLE_DOCK_EN
#include "apple_dock/iAP.h"
#include "apple_mfi.h"
#endif

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[mount]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static struct usb_host_device host_devices[USB_MAX_HW_NUM];// SEC(.usb_h_bss);

#define     device_to_usbdev(device)	((struct usb_host_device *)((device)->private_data))


int host_dev_status(const struct usb_host_device *host_dev)
{
    return ((host_dev)->private_data.status);
}

u32 host_device2id(const struct usb_host_device *host_dev)
{
#if USB_MAX_HW_NUM > 1
    return ((host_dev)->private_data.usb_id);
#else
    return 0;
#endif
}
const struct usb_host_device *host_id2device(const usb_dev id)
{
#if USB_MAX_HW_NUM > 1
    return &host_devices[id];
#else
    return &host_devices[0];
#endif
}
int usb_sem_init(struct usb_host_device  *host_dev)
{
    usb_dev usb_id = host_device2id(host_dev);

    usb_host_config(usb_id);

    OS_SEM *sem = zalloc(sizeof(OS_SEM));
    ASSERT(sem, "usb alloc sem error");
    host_dev->sem = sem;
    g_printf("%s %x %x ", __func__, host_dev, sem);
    os_sem_create(host_dev->sem, 0);
    return 0;
}
int usb_sem_pend(struct usb_host_device  *host_dev, u32 timeout)
{
    if (host_dev->sem == NULL) {
        return 1;
    }
    int ret = os_sem_pend(host_dev->sem, timeout);
    if (ret) {
        r_printf("%s %d ", __func__, ret);
    }
    return ret;
}
int usb_sem_post(struct usb_host_device  *host_dev)
{
    if (host_dev->sem == NULL) {
        return 1;
    }
    int ret = os_sem_post(host_dev->sem);
    if (ret) {
        r_printf("%s %d ", __func__, ret);
    }
    return 0;
}
int usb_sem_del(struct usb_host_device *host_dev)
{
    usb_dev usb_id = host_device2id(host_dev);

    r_printf("1");
    if (host_dev == NULL || host_dev->sem == NULL) {
        return 0;
    }
    r_printf("2");
    r_printf("3");
#if USB_HUB
    if (host_dev->father == NULL) {
        os_sem_del(host_dev->sem, 0);
    }
#else
    os_sem_del(host_dev->sem, 0);
#endif
    r_printf("4");
    g_printf("%s %x %x ", __func__, host_dev, host_dev->sem);
    free(host_dev->sem);
    r_printf("5");
    host_dev->sem = NULL;
    r_printf("6");
    usb_host_free(usb_id);
    r_printf("7");
    return 0;
}
int usb_mutex_init(struct usb_host_device *host_dev)
{
    OS_MUTEX *mutex = zalloc(sizeof(OS_MUTEX));
    ASSERT(mutex, "usb alloc mutex error");
    host_dev->mutex = mutex;
    os_mutex_create(host_dev->mutex);
    return 0;
}
int usb_mutex_pend(struct usb_host_device *host_dev, u32 timeout)
{
    if (host_dev->mutex == NULL) {
        return 1;
    }
    int ret = os_mutex_pend(host_dev->mutex, timeout);
    if (ret) {
        r_printf("%s %d ", __func__, ret);
    }
    return ret;
}
int usb_mutex_post(struct usb_host_device *host_dev)
{
    if (host_dev->mutex == NULL) {
        return 1;
    }
    int ret = os_mutex_post(host_dev->mutex);
    if (ret) {
        r_printf("%s %d ", __func__, ret);
    }
    return 0;
}
int usb_mutex_del(struct usb_host_device *host_dev)
{
    if (host_dev->mutex == NULL) {
        return 0;
    }
    os_mutex_del(host_dev->mutex, 1);
    free(host_dev->mutex);
    host_dev->mutex = NULL;
    return 0;
}

static int _usb_msd_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find udisk @ interface %d", interface_num);
#if TCFG_UDISK_ENABLE
    return   usb_msd_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_apple_mfi_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find udisk @ interface %d", interface_num);
#if TCFG_USB_APPLE_DOCK_EN
    return   usb_apple_mfi_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_adb_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find adb @ interface %d", interface_num);
#if TCFG_ADB_ENABLE
    return usb_adb_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_aoa_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find aoa @ interface %d", interface_num);
#if TCFG_AOA_ENABLE
    return usb_aoa_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif

}
static int _usb_hid_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find hid @ interface %d", interface_num);
#if TCFG_HID_HOST_ENABLE
    return usb_hid_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_audio_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find audio @ interface %d", interface_num);
#if TCFG_HOST_AUDIO_ENABLE
    return usb_audio_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_adb_interface_ptp_mtp_parse(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find adbmtp @ interface %d", interface_num);
#if TCFG_ADB_ENABLE
    return usb_adb_interface_ptp_mtp_parse(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
/**
 * @brief usb_descriptor_parser
 *
 * @param device
 * @param pBuf
 * @param total_len
 *
 * @return
 */
static int usb_descriptor_parser(struct usb_host_device *host_dev, const u8 *pBuf, u32 total_len, struct usb_device_descriptor *device_desc)
{
    int len = 0;
    u8 interface_num = 0;
    //struct usb_private_data *private_data = &host_dev->private_data;

    struct usb_config_descriptor *cfg_desc = (struct usb_config_descriptor *)pBuf;

    if (cfg_desc->bDescriptorType != USB_DT_CONFIG ||
        cfg_desc->bLength < USB_DT_CONFIG_SIZE) {
        log_error("invalid descriptor for config bDescriptorType = %d bLength= %d",
                  cfg_desc->bDescriptorType, cfg_desc->bLength);
        return -USB_DT_CONFIG;
    }

    log_info("idVendor %x idProduct %x", device_desc->idVendor, device_desc->idProduct);

    len += USB_DT_CONFIG_SIZE;
    pBuf += USB_DT_CONFIG_SIZE;
    int i;
    u32 have_find_valid_class = 0;
    while (len < total_len) {
        if (interface_num >= MAX_HOST_INTERFACE) {
            log_error("interface_num too much");
            break;
        }

        struct usb_interface_descriptor *interface = (struct usb_interface_descriptor *)pBuf;
        if (interface->bDescriptorType == USB_DT_INTERFACE) {

            printf("inf class %x subclass %x ep %d",
                   interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bNumEndpoints);

            if (interface->bInterfaceClass == USB_CLASS_MASS_STORAGE) {
                i = _usb_msd_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((device_desc->idVendor == 0x05AC) &&
                       ((device_desc->idProduct & 0xff00) == 0x1200)) {
                i = _usb_apple_mfi_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if (interface->bInterfaceClass == USB_CLASS_AUDIO) {
                i = _usb_audio_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((interface->bInterfaceClass == 0xff)  &&
                       (interface->bInterfaceSubClass == USB_CLASS_ADB)) {
                i = _usb_adb_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((device_desc->idVendor == 0x18d1) &&
                       ((device_desc->idProduct & 0x2d00) == 0x2d00)) {
                i = _usb_aoa_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if (interface->bInterfaceClass == USB_CLASS_HID) {
                i = _usb_hid_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((interface->bNumEndpoints == 3) &&
                       (interface->bInterfaceClass == 0xff || interface->bInterfaceClass == 0x06)) {
                i = _usb_adb_interface_ptp_mtp_parse(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else if ((interface->bInterfaceClass == 0xff) &&
                       (interface->bInterfaceSubClass == 0xff)) {
                i = _usb_aoa_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;

            } else {
                log_info("find unsupport [class %x subClass %x] @ interface %d",
                         interface->bInterfaceClass,
                         interface->bInterfaceSubClass,
                         interface_num);

                len += USB_DT_INTERFACE_SIZE;
                pBuf += USB_DT_INTERFACE_SIZE;
            }
        } else {
            /* log_error("unknown section %d %d", len, pBuf[0]); */
            if (pBuf[0]) {
                len += pBuf[0];
                pBuf += pBuf[0];
            } else {
                len = total_len;
            }
        }
    }


    log_debug("len %d total_len %d", len, total_len);
    return !have_find_valid_class;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_suspend
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
void usb_host_suspend(const usb_dev usb_id)
{
    usb_h_entry_suspend(usb_id);
}

void usb_host_resume(const usb_dev usb_id)
{
    usb_h_resume(usb_id);
}

static int usb_event_notify(const struct usb_host_device *host_dev, u32 ev)
{
    const usb_dev id = host_device2id(host_dev);
    u32 event;
    typedef struct {
        char subdev[MAX_HOST_INTERFACE][8];
    } subdev_t;
    static subdev_t itf_set[USB_MAX_HW_NUM];
    static u32 bmUsbEvent[USB_MAX_HW_NUM];
    u8 have_post_event = 0;
    u8 no_send_event = 0;
    if (ev == 0) {
        event = DEVICE_EVENT_IN;
    } else if (ev == 1) {
        event = DEVICE_EVENT_CHANGE;
    } else {
        event = DEVICE_EVENT_OUT;
        goto __usb_event_out;
    }
    for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
        no_send_event = 0;
        memset(itf_set[id].subdev[i], 0, sizeof(itf_set[id].subdev[i]));
        if (host_dev->interface_info[i]) {
            switch (host_dev->interface_info[i]->ctrl->interface_class) {
#if TCFG_UDISK_ENABLE
            case USB_CLASS_MASS_STORAGE:
                if (have_post_event & BIT(0)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(0);
                }
                sprintf(itf_set[id].subdev[i], "udisk%d", id);
                bmUsbEvent[id] |= BIT(0);
                break;
#endif
#if TCFG_ADB_ENABLE
            case USB_CLASS_ADB:
                if (have_post_event & BIT(1)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(1);
                }
                sprintf(itf_set[id].subdev[i], "adb%d", id);
                bmUsbEvent[id] |= BIT(1);
                break;
#endif
#if TCFG_AOA_ENABLE
            case USB_CLASS_AOA:
                if (have_post_event & BIT(2)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(2);
                }
                sprintf(itf_set[id].subdev[i], "aoa%d", id);
                bmUsbEvent[id] |= BIT(2);
                break;
#endif
#if TCFG_HID_HOST_ENABLE
            case USB_CLASS_HID:
                if (have_post_event & BIT(3)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(3);
                }
                sprintf(itf_set[id].subdev[i], "hid%d", id);
                bmUsbEvent[id] |= BIT(3);
                break;
#endif
#if TCFG_HOST_AUDIO_ENABLE
            case USB_CLASS_AUDIO:
                if (have_post_event & BIT(4)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(4);
                }
                sprintf(itf_set[id].subdev[i], "audio%d", id);
                bmUsbEvent[id] |= BIT(4);
                break;
#endif
            }

            //cppcheck-suppress knownConditionTrueFalse
            if (!no_send_event && itf_set[id].subdev[i][0]) {
                log_info("event %x interface %x class %x %s",
                         event,
                         i,
                         host_dev->interface_info[i]->ctrl->interface_class,
                         itf_set[id].subdev[i]);

                /* printf("usb_host_mount notify >>>>>>>>>>>\n"); */
                /* usb_driver_event_to_user(DEVICE_EVENT_FROM_USB_HOST, event, itf_set[id].subdev[i]); */
                os_taskq_post_msg(USB_TASK_NAME, 3, USBSTACK_HOST_MSG, event, itf_set[id].subdev[i]);
            }
        }
    }

__usb_event_out:
    if (event == DEVICE_EVENT_OUT) {
        for (int i = 0; i < 32; i++) {
            if (bmUsbEvent[id] & BIT(i)) {
                bmUsbEvent[id] &= ~BIT(i);
                if (itf_set[id].subdev[i][0]) {
                    have_post_event = 1;
                    /* usb_driver_event_to_user(DEVICE_EVENT_FROM_USB_HOST, event, itf_set[id].subdev[i]); */
                    os_taskq_post_msg(USB_TASK_NAME, 3, USBSTACK_HOST_MSG, event, itf_set[id].subdev[i]);
                }
            }
        }
    }

    if (have_post_event) {
        return DEV_ERR_NONE;
    } else {
        return DEV_ERR_UNKNOW_CLASS;
    }

}

static u32 _usb_host_mount(const usb_dev usb_id, u32 retry, u32 reset_delay, u32 mount_timeout)
{
    u32 ret = DEV_ERR_NONE;
    struct usb_host_device *host_dev = &host_devices[usb_id];
    struct usb_private_data *private_data = &host_dev->private_data;
    u32 speed = USB_SPEED_FULL;

#if defined(FUSB_MODE) && FUSB_MODE
    speed = USB_SPEED_FULL;
#elif defined(FUSB_MODE) && (FUSB_MODE == 0)
    speed = USB_SPEED_HIGH;
#endif
    usb_host_clock_lock();
    for (int i = 0; i < retry; i++) {
        usb_h_sie_init(usb_id);
        ret = usb_host_init(usb_id, reset_delay, mount_timeout, speed);
        if (ret) {
            reset_delay += 10;
            continue;
        }

        void *const ep0_dma = usb_h_alloc_ep_buffer(usb_id, 0, 64);
        usb_set_dma_taddr(usb_id, 0, ep0_dma);

        usb_sie_enable(usb_id);//enable sie intr
        usb_mdelay(reset_delay);

        /**********get device descriptor*********/
        struct usb_device_descriptor device_desc;
        private_data->usb_id = usb_id;
        private_data->status = 0;
        private_data->devnum = 0;
        private_data->ep0_max_packet_size = 8;
        usb_get_device_descriptor(host_dev, &device_desc);

        /**********set address*********/
        usb_mdelay(20);
        u8 devnum = rand32() % 16 + 1;
        ret = set_address(host_dev, devnum);
        check_usb_mount(ret);
        private_data->devnum = devnum ;

        /**********get device descriptor*********/
        usb_mdelay(20);
        ret = usb_get_device_descriptor(host_dev, &device_desc);
        check_usb_mount(ret);
        private_data->ep0_max_packet_size = device_desc.bMaxPacketSize0;

        /**********get config descriptor*********/
        struct usb_config_descriptor cfg_desc;
        ret = get_config_descriptor(host_dev, &cfg_desc, USB_DT_CONFIG_SIZE);
        check_usb_mount(ret);

#if USB_H_MALLOC_ENABLE
        u8 *desc_buf = zalloc(cfg_desc.wTotalLength + 16);
        ASSERT(desc_buf, "desc_buf");
#else
        u8 desc_buf[128] = {0};
        cfg_desc.wTotalLength = min(sizeof(desc_buf), cfg_desc.wTotalLength);
#endif

        ret = get_config_descriptor(host_dev, desc_buf, cfg_desc.wTotalLength);
        check_usb_mount(ret);

        /**********set configuration*********/
        ret = set_configuration(host_dev);
        /* printf_buf(desc_buf, cfg_desc.wTotalLength); */
        ret |= usb_descriptor_parser(host_dev, desc_buf, cfg_desc.wTotalLength, &device_desc);
#if USB_H_MALLOC_ENABLE
        log_info("free:desc_buf= %x\n", desc_buf);
        free(desc_buf);
#endif
        check_usb_mount(ret);

        for (int itf = 0; itf < MAX_HOST_INTERFACE; itf++) {
            if (host_dev->interface_info[itf]) {
                if (host_dev->interface_info[itf]->ctrl->set_power) {
                    host_dev->interface_info[itf]->ctrl->set_power(host_dev, 1);
                }
            }
        }
        break;//succ
    }

    if (ret) {
        goto __exit_fail;
    }
    private_data->status = 1;
    return DEV_ERR_NONE;

__exit_fail:
    printf("usb_probe fail");
    private_data->status = 0;
    usb_sie_close(usb_id);
    usb_host_clock_unlock(NULL);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_mount
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
u32 usb_host_mount(const usb_dev id, u32 retry, u32 reset_delay, u32 mount_timeout)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif

    u32 ret;
    struct usb_host_device *host_dev = &host_devices[usb_id];
    memset(host_dev, 0, sizeof(*host_dev));

    host_dev->private_data.usb_id = id;

    usb_sem_init(host_dev);
    usb_mutex_init(host_dev);
    usb_h_isr_reg(usb_id, 1, 0);


    ret = _usb_host_mount(usb_id, retry, reset_delay, mount_timeout);
    usb_otg_resume(usb_id);  //打开usb host之后恢复otg检测
    if (ret) {
        goto __exit_fail;
    }
    return usb_event_notify(host_dev, 0);

__exit_fail:
    usb_sie_disable(usb_id);
    usb_mutex_del(host_dev);
    usb_sem_del(host_dev);
    return ret;
}

static u32 _usb_host_unmount(const usb_dev usb_id)
{
    struct usb_host_device *host_dev = &host_devices[usb_id];

    struct usb_private_data *private_data = &host_dev->private_data;
    private_data->status = 0;

    usb_sem_post(host_dev);//拔掉设备时，让读写线程快速释放

    for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
        if (host_dev->interface_info[i]) {
            if (host_dev->interface_info[i]->ctrl->set_power) {
                host_dev->interface_info[i]->ctrl->set_power(host_dev, 0);
            }
            if (host_dev->interface_info[i]->ctrl->release) {
                host_dev->interface_info[i]->ctrl->release(host_dev);
            }
            host_dev->interface_info[i] = NULL;
        }
    }

    usb_sie_close(usb_id);
    return DEV_ERR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_unmount
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
/* u32 usb_host_unmount(const usb_dev usb_id, char *device_name) */
u32 usb_host_unmount(const usb_dev id)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif
    u32 ret;
    struct usb_host_device *host_dev = &host_devices[usb_id];

#if (TCFG_UDISK_ENABLE && UDISK_READ_512_ASYNC_ENABLE)
    _usb_stor_async_wait_sem(host_dev);
#endif
    ret = _usb_host_unmount(usb_id);
    if (ret) {
        goto __exit_fail;
    }
    usb_mutex_del(host_dev);
    usb_sem_del(host_dev);

    /* printf("usb_host_unmount notify >>>>>>>>>>>\n"); */
    usb_event_notify(host_dev, 2);
    return DEV_ERR_NONE;

__exit_fail:
    return ret;
}

u32 usb_host_remount(const usb_dev id, u32 retry, u32 delay, u32 ot, u8 notify)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif
    u32 ret;

    ret = _usb_host_unmount(usb_id);
    if (ret) {
        goto __exit_fail;
    }

    struct usb_host_device *host_dev = &host_devices[usb_id];
    os_sem_set(host_dev->sem, 0);

    ret = _usb_host_mount(usb_id, retry, delay, ot);
    if (ret) {
        goto __exit_fail;
    }

    if (notify) {
        usb_event_notify(host_dev, 1);
    }
    return DEV_ERR_NONE;

__exit_fail:
    return ret;
}

int usb_host_force_reset(const usb_dev usb_id)
{
    //预留以后加上hub的处理
    usb_h_force_reset(usb_id);
    return 0;
}

static u16 clock_lock_timer;
void usb_host_clock_unlock(void *priv)
{
#ifdef CONFIG_CPU_BR27
    if (clock_lock_timer) {
        clock_unlock("usb_host");
        sys_timeout_del(clock_lock_timer);
        clock_lock_timer = 0;
    }
#endif
}

void usb_host_clock_lock(void)
{
#ifdef CONFIG_CPU_BR27
    if (clock_lock_timer) {
        sys_timer_re_run(clock_lock_timer);
        return;
    }
    clock_lock("usb_host", 320000000);
    clock_lock_timer = sys_timeout_add(NULL, usb_host_clock_unlock, 3000);
#endif
}
#endif
