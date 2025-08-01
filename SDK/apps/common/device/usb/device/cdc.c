#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "usb/device/usb_stack.h"
#include "usb/usb_config.h"
#include "usb/device/cdc.h"
#include "app_config.h"
#include "os/os_api.h"
#include "cdc_defs.h"  //need redefine __u8, __u16, __u32

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_SLAVE_CDC_ENABLE


struct usb_cdc_gadget {
    u8 *cdc_buffer;
    u8 *bulk_ep_out_buffer;
    u8 *bulk_ep_in_buffer;
    void *priv;
    int (*output)(void *priv, u8 *obuf, u32 olen);
    void (*wakeup_handler)(struct usb_device_t *usb_device);
    OS_MUTEX mutex_data;
#if CDC_INTR_EP_ENABLE
    OS_MUTEX mutex_intr;
    u8 *intr_ep_in_buffer;
#endif
    u8 bmTransceiver;
    u8 subtype_data[8];
    u8 comm_feature[2];
};

static struct usb_cdc_gadget *cdc_hdl;

#if USB_MALLOC_ENABLE

#else
static u8 _cdc_buffer[MAXP_SIZE_CDC_BULKOUT] SEC(.usb.data.bss.exchange) __attribute__((aligned(4)));
static struct usb_cdc_gadget _cdc_hdl SEC(.usb.data.bss.exchange);
#endif

static const u8 cdc_virtual_comport_desc[] = {
    //IAD Descriptor
    0x08,                       //bLength
    0x0b,                       //bDescriptorType
    0x00,                       //bFirstInterface
    0x02,                       //bInterfaceCount
    0x02,                       //bFunctionClass, Comunication and CDC control
    0x02,                       //bFunctionSubClass
    0x01,                       //bFunctionProtocol
    0x00,                       //iFunction
    //Interface 0, Alt 0
    0x09,                       //Length
    0x04,                       //DescriptorType:Interface
    0x00,                       //InterfaceNum
    0x00,                       //AlternateSetting
    0x01,                       //NumEndpoint
    0x02,                       //InterfaceClass, Communation and CDC control
    0x02,                       //InterfaceSubClass, Abstract Control Model
    0x01,                       //InterfaceProtocol, AT commands defined by ITU-T V.250 etc
    0x00,                       //Interface String
    //CDC Interface Descriptor
    0x05,                       //bLength
    0x24,                       //bDescriptorType
    0x00,                       //bDescriptorSubType, Header Functional Desc
    0x10, 0x01,                 //bcdCDC, version 1.10
    //CDC Interface Descriptor
    0x05,                       //bLength
    0x24,                       //bDescriptorType
    0x01,                       //bDescriptorSubType, Call Management Functional Descriptor
    0x03,                       //bmCapabilities, D7..D2 reversed
    //  D7..D2 reversed
    //  D1 sends/receives call management information only over a Data Class interface
    //  D0 handle call management itself
    0x01,                       //bDataInterface
    //CDC Interface Descriptor
    0x04,                       //bLength
    0x24,                       //bDescriptorType
    0x02,                       //bDescriptorSubType, Abstract Control Management Functional Descriptor
    0x03,                       //bmCapabilities, D7..D2 reversed
    //  D7..D4 reversed
    //  D3 supports the notification Network_Connection
    //  D2 not supports the request Send_Break
    //  D1 supports the request combination of Set_Line_Coding, Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State
    //  D0 supports the request combination of Set_Comm_Feature, Clear_Comm_Feature, and Get_Comm_Feature
    //CDC Interface Descriptor
    0x05,                       //bLength
    0x24,                       //bDescriptorType
    0x06,                       //bDescriptorSubType, Union Functional Descriptor
    0x00,                       //bControlInterface
    0x01,                       //bSubordinateInterface[0]
    //Endpoint In
    0x07,                       //bLength
    0x05,                       //bDescritorType
    0x82,                       //bEndpointAddr
    0x03,                       //bmAttributes, interrupt
    0x08, 0x00,                 //wMaxPacketSize
    0x01,                       //bInterval, 1ms
    //Interface 1, Alt 0
    0x09,                       //Length
    0x04,                       //DescriptorType:Interface
    0x01,                       //InterfaceNum
    0x00,                       //AlternateSetting
    0x02,                       //NumEndpoint
    0x0a,                       //InterfaceClass, CDC Data
    0x00,                       //InterfaceSubClass
    0x00,                       //InterfaceProtocol
    0x00,                       //Interface String
    //Endpoint Out
    0x07,                       //bLength
    0x05,                       //bDescriptor
    0x02,                       //bEndpointAddr
    0x02,                       //bmAttributes, bulk
    0x40, 0x00,                 //wMaxPacketSize
    0x00,                       //bInterval
    //Endpoint In
    0x07,                       //bLength
    0x05,                       //bDescritorType
    0x83,                       //bEndpointAddr
    0x02,                       //bmAttributes, bulk
    0x40, 0x00,                 //wMaxPacketSize
    0x00,                       //bInterval
};

static void cdc_endpoint_init(struct usb_device_t *usb_device, u32 itf);
static u32 cdc_setup_rx(struct usb_device_t *usb_device, struct usb_ctrlrequest *ctrl_req);

static void usb_cdc_line_coding_init(struct usb_cdc_line_coding *lc)
{
    lc->dwDTERate = 460800;
    lc->bCharFormat = USB_CDC_1_STOP_BITS;
    lc->bParityType = USB_CDC_NO_PARITY;
    lc->bDataBits = 8;
}

static void dump_line_coding(struct usb_cdc_line_coding *lc)
{
    log_info("dtw rate      : %d", lc->dwDTERate);
    log_info("stop bits     : %d", lc->bCharFormat);
    log_info("verify bits   : %d", lc->bParityType);
    log_info("data bits     : %d", lc->bDataBits);
}

static u32 cdc_setup(struct usb_device_t *usb_device, struct usb_ctrlrequest *ctrl_req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    int recip_type;
    u32 len;

    recip_type = ctrl_req->bRequestType & USB_TYPE_MASK;

    switch (recip_type) {
    case USB_TYPE_CLASS:
        switch (ctrl_req->bRequest) {
        case USB_CDC_SEND_ENCAPSULATED_COMMAND:
            usb_set_setup_recv(usb_device, cdc_setup_rx);
            break;
        case USB_CDC_GET_ENCAPSULATED_RESPONSE:
            //Not need to transfer ITU-T V250 AT command,
            //just sample for this command
            memset(usb_get_setup_buffer(usb_device), 0, ctrl_req->wLength);
            usb_set_data_payload(usb_device, ctrl_req, usb_get_setup_buffer(usb_device), ctrl_req->wLength);
            break;
        case 0x02:  //Set_Comm_Feature
            usb_set_setup_recv(usb_device, cdc_setup_rx);
            break;
        case 0x03:  //Clear_Comm_Feature
            if (ctrl_req->wValue == 0x01) {  //ABSTRACT_STATE
                cdc_hdl->comm_feature[0] = 0;
                usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            }
            break;
        case 0x04:  //Get_Comm_Feature
            if (ctrl_req->wValue == 0x01) {  //ABSTRACT_STATE
                usb_set_data_payload(usb_device, ctrl_req, cdc_hdl->comm_feature, ctrl_req->wLength);
            }
            break;
        case USB_CDC_REQ_SET_LINE_CODING:
            log_info("set line coding");
            usb_set_setup_recv(usb_device, cdc_setup_rx);
            break;
        case USB_CDC_REQ_GET_LINE_CODING:
            log_info("get line codling");
            len = ctrl_req->wLength < sizeof(struct usb_cdc_line_coding) ?
                  ctrl_req->wLength : sizeof(struct usb_cdc_line_coding);
            if (cdc_hdl == NULL) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                break;
            }
            usb_set_data_payload(usb_device, ctrl_req, cdc_hdl->subtype_data, len);
            dump_line_coding((struct usb_cdc_line_coding *)cdc_hdl->subtype_data);
            break;
        case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
            log_info("set control line state - %d", ctrl_req->wValue);
            if (cdc_hdl) {
                /* if (ctrl_req->wValue & BIT(0)) { //DTR */
                cdc_hdl->bmTransceiver |= BIT(0);
                /* } else { */
                /* cdc_hdl->bmTransceiver &= ~BIT(0); */
                /* } */
                /* if (ctrl_req->wValue & BIT(1)) { //RTS */
                cdc_hdl->bmTransceiver |= BIT(1);
                /* } else { */
                /* usb_slave->cdc->bmTransceiver &= ~BIT(1); */
                /* } */
                cdc_hdl->bmTransceiver |= BIT(4);  //cfg done
            }
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            //cdc_endpoint_init(usb_device, (ctrl_req->wIndex & USB_RECIP_MASK));
            break;
        default:
            log_error("unsupported class req");
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            break;
        }
        break;
    default:
        log_error("unsupported req type");
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        break;
    }
    return 0;
}

static u32 cdc_setup_rx(struct usb_device_t *usb_device, struct usb_ctrlrequest *ctrl_req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    int recip_type;
    struct usb_cdc_line_coding *lc = 0;
    u32 len;
    u8 *read_ep = usb_get_setup_buffer(usb_device);

    len = ctrl_req->wLength;
    usb_read_ep0(usb_id, read_ep, len);
    recip_type = ctrl_req->bRequestType & USB_TYPE_MASK;
    switch (recip_type) {
    case USB_TYPE_CLASS:
        switch (ctrl_req->bRequest) {
        case USB_CDC_SEND_ENCAPSULATED_COMMAND:
            log_info("USB_CDC_SEND_ENCAPSULATED_COMMAND");
            log_debug_hexdump(read_ep, len);
            break;
        case 0x02:  //Set_Comm_Feature
            log_info("USB_CDC_REQ_SET_COMM_FEATURE");
            if (cdc_hdl == NULL) {
                break;
            }
            memcpy(cdc_hdl->comm_feature, read_ep, len);
            log_debug_hexdump(read_ep, len);
            break;
        case USB_CDC_REQ_SET_LINE_CODING:
            log_info("USB_CDC_REQ_SET_LINE_CODING");
            if (cdc_hdl == NULL) {
                break;
            }
            if (len > sizeof(struct usb_cdc_line_coding)) {
                len = sizeof(struct usb_cdc_line_coding);
            }
            memcpy(cdc_hdl->subtype_data, read_ep, len);
            lc = (struct usb_cdc_line_coding *)cdc_hdl->subtype_data;
            dump_line_coding(lc);
            break;
        }
        break;
    }
    return USB_EP0_STAGE_SETUP;
}

static void cdc_reset(struct usb_device_t *usb_device, u32 itf)
{
    log_debug("%s", __func__);
    //cppcheck-suppress unreadVariable
    const usb_dev usb_id = usb_device2id(usb_device);
#if USB_ROOT2
    usb_disable_ep(usb_id, CDC_DATA_EP_IN);
#if CDC_INTR_EP_ENABLE
    usb_disable_ep(usb_id, CDC_INTR_EP_IN);
#endif
#else
    cdc_endpoint_init(usb_device, itf);
#endif
}

u32 cdc_desc_config(const usb_dev usb_id, u8 *ptr, u32 *itf)
{
    //cppcheck-suppress unreadVariable
    struct usb_device_t *usb_device = usb_id2device(usb_id);
    u8 *tptr;

    tptr = ptr;
    memcpy(tptr, cdc_virtual_comport_desc, sizeof(cdc_virtual_comport_desc));
    //iad interface number
    tptr[2] = *itf;
    //control interface number
    tptr[8 + 2] = *itf;
    tptr[8 + 9 + 5 + 4] = *itf + 1;
    tptr[8 + 9 + 5 + 5 + 4 + 3] = *itf;
    tptr[8 + 9 + 5 + 5 + 4 + 4] = *itf + 1;
    //interrupt in ep
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 2] = USB_DIR_IN | CDC_INTR_EP_IN;
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 4] = MAXP_SIZE_CDC_INTRIN & 0xff;
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 5] = (MAXP_SIZE_CDC_INTRIN >> 8) & 0xff;
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 6] = CDC_INTR_INTERVAL;
#if defined(FUSB_MODE) && FUSB_MODE == 0
    if (usb_device->bSpeed == USB_SPEED_FULL) {
        tptr[8 + 9 + 5 + 5 + 4 + 5 + 6] = CDC_INTR_INTERVAL_FS;  //high-speed mode, 125x2^(4-1)=1ms
    }
#endif
    //data interface number
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 2] = *itf + 1;
    //bulk out ep
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 2] = CDC_DATA_EP_OUT;
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 4] = MAXP_SIZE_CDC_BULKOUT & 0xff;
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 5] = (MAXP_SIZE_CDC_BULKOUT >> 8) & 0xff;
    //bulk in ep
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 2] = USB_DIR_IN | CDC_DATA_EP_IN;
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 4] = MAXP_SIZE_CDC_BULKIN & 0xff;
    tptr[8 + 9 + 5 + 5 + 4 + 5 + 7 + 9 + 7 + 5] = (MAXP_SIZE_CDC_BULKIN >> 8) & 0xff;
    tptr += sizeof(cdc_virtual_comport_desc);

    if (usb_set_interface_hander(usb_id, *itf, cdc_setup) != *itf) {
        ASSERT(0, "cdc set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *itf, cdc_reset) != *itf) {

    }
    *itf += 2;
    return (u32)(tptr - ptr);
}

void cdc_set_wakeup_handler(void (*handle)(struct usb_device_t *usb_device))
{
    if (cdc_hdl) {
        cdc_hdl->wakeup_handler = handle;
    }
}

static void cdc_wakeup_handler(struct usb_device_t *usb_device, u32 ep)
{
    if (cdc_hdl && cdc_hdl->wakeup_handler) {
        cdc_hdl->wakeup_handler(usb_device);
    }
}

void cdc_set_output_handle(void *priv, int (*output_handler)(void *priv, u8 *buf, u32 len))
{
    if (cdc_hdl) {
        cdc_hdl->output = output_handler;
        cdc_hdl->priv = priv;
    }
}

static void cdc_intrrx(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    if (cdc_hdl == NULL) {
        return;
    }
    u8 *cdc_rx_buf = cdc_hdl->cdc_buffer;
    //由于bulk传输使用双缓冲，无法用usb_get_ep_buffer()知道是哪一个buffer，需要外部buffer接收数据
    u32 len = usb_g_bulk_read(usb_id, CDC_DATA_EP_OUT, cdc_rx_buf, MAXP_SIZE_CDC_BULKOUT, 0);
    if (cdc_hdl->output) {
        cdc_hdl->output(cdc_hdl->priv, cdc_rx_buf, len);
    }
}

static void cdc_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    ASSERT(cdc_hdl, "cdc not register");

    const usb_dev usb_id = usb_device2id(usb_device);

    usb_g_ep_config(usb_id, CDC_DATA_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_BULK,
                    0, cdc_hdl->bulk_ep_in_buffer, MAXP_SIZE_CDC_BULKIN);

    usb_g_ep_config(usb_id, CDC_DATA_EP_OUT | USB_DIR_OUT, USB_ENDPOINT_XFER_BULK,
                    1, cdc_hdl->bulk_ep_out_buffer, MAXP_SIZE_CDC_BULKOUT);
    /* usb_g_set_intr_hander(usb_id, CDC_DATA_EP_OUT | USB_DIR_OUT, cdc_intrrx); */
    usb_g_set_intr_hander(usb_id, CDC_DATA_EP_OUT | USB_DIR_OUT, cdc_wakeup_handler);
    usb_enable_ep(usb_id, CDC_DATA_EP_IN);

#if CDC_INTR_EP_ENABLE
    usb_g_ep_config(usb_id, CDC_INTR_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_INT,
                    0, cdc_hdl->intr_ep_in_buffer, MAXP_SIZE_CDC_INTRIN);
    usb_enable_ep(usb_id, CDC_INTR_EP_IN);
#endif
}

u32 cdc_read_data(const usb_dev usb_id, u8 *buf, u32 len)
{
    u32 rxlen;
    if (cdc_hdl == NULL) {
        return 0;
    }
    u8 *cdc_rx_buf = cdc_hdl->cdc_buffer;
    os_mutex_pend(&cdc_hdl->mutex_data, 0);
    //由于bulk传输使用双缓冲，无法用usb_get_ep_buffer()知道是哪一个buffer，需要外部buffer接收数据
    rxlen = usb_g_bulk_read(usb_id, CDC_DATA_EP_OUT, cdc_rx_buf, MAXP_SIZE_CDC_BULKOUT, 0);
    rxlen = rxlen > len ? len : rxlen;
    if (rxlen > 0) {
        memcpy(buf, cdc_rx_buf, rxlen);
    }
    os_mutex_post(&cdc_hdl->mutex_data);
    return rxlen;
}

u32 cdc_write_data(const usb_dev usb_id, u8 *buf, u32 len)
{
    u32 txlen, offset;
    if (cdc_hdl == NULL) {
        return 0;
    }
    if ((cdc_hdl->bmTransceiver & (BIT(1) | BIT(4))) != (BIT(1) | BIT(4))) {
        return 0;
    }
    if (!(cdc_hdl->bmTransceiver & BIT(0))) {
        return 0;
    }
    offset = 0;
    os_mutex_pend(&cdc_hdl->mutex_data, 0);
    while (offset < len) {
        txlen = len - offset > MAXP_SIZE_CDC_BULKIN ?
                MAXP_SIZE_CDC_BULKIN : len - offset;
        txlen = usb_g_bulk_write(usb_id, CDC_DATA_EP_IN, buf + offset, txlen);
        if (txlen == 0) {
            break;
        }
        if ((cdc_hdl->bmTransceiver & (BIT(1) | BIT(4))) != (BIT(1) | BIT(4))) {
            break;
        }
        offset += txlen;
    }
    //当最后一包的包长等于maxpktsize，需要发一个0长包表示结束
    if ((offset % MAXP_SIZE_CDC_BULKIN) == 0) {
        usb_g_bulk_write(usb_id, CDC_DATA_EP_IN, NULL, 0);
    }
    os_mutex_post(&cdc_hdl->mutex_data);
    return offset;
}

u32 cdc_write_inir(const usb_dev usb_id, u8 *buf, u32 len)
{
#if CDC_INTR_EP_ENABLE
    u32 txlen, offset;
    if (cdc_hdl == NULL) {
        return 0;
    }
    if ((cdc_hdl->bmTransceiver & BIT(4)) == 0) {
        return 0;
    }
    offset = 0;
    os_mutex_pend(&cdc_hdl->mutex_intr, 0);
    while (offset < len) {
        txlen = len - offset > MAXP_SIZE_CDC_INTRIN ?
                MAXP_SIZE_CDC_INTRIN : len - offset;
        txlen = usb_g_intr_write(usb_id, CDC_INTR_EP_IN, buf + offset, txlen);
        if (txlen == 0) {
            break;
        }
        if ((cdc_hdl->bmTransceiver & BIT(4)) == 0) {
            break;
        }
        offset += txlen;
    }
    os_mutex_post(&cdc_hdl->mutex_intr);
    return offset;
#else
    return 0;
#endif
}

void cdc_register(const usb_dev usb_id)
{
    struct usb_cdc_line_coding *lc;
    /* log_info("%s() %d", __func__, __LINE__); */
    if (!cdc_hdl) {
#if USB_MALLOC_ENABLE
        cdc_hdl = zalloc(sizeof(struct usb_cdc_gadget));
        if (!cdc_hdl) {
            log_error("cdc_register err 1");
            return;
        }
        cdc_hdl->cdc_buffer = malloc(MAXP_SIZE_CDC_BULKOUT);
        if (!cdc_hdl->cdc_buffer) {
            log_error("cdc_register err 2");
            goto __exit_err;
        }
#else
        memset(&_cdc_hdl, 0, sizeof(struct usb_cdc_gadget));
        cdc_hdl = &_cdc_hdl;
        cdc_hdl->cdc_buffer = _cdc_buffer;
#endif
        lc = (struct usb_cdc_line_coding *)cdc_hdl->subtype_data;
        usb_cdc_line_coding_init(lc);
        os_mutex_create(&cdc_hdl->mutex_data);

        cdc_hdl->bulk_ep_in_buffer = usb_alloc_ep_dmabuffer(usb_id, CDC_DATA_EP_IN | USB_DIR_IN, MAXP_SIZE_CDC_BULKIN);
        cdc_hdl->bulk_ep_out_buffer = usb_alloc_ep_dmabuffer(usb_id, CDC_DATA_EP_OUT, MAXP_SIZE_CDC_BULKOUT * 2);

#if CDC_INTR_EP_ENABLE
        os_mutex_create(&cdc_hdl->mutex_intr);
        cdc_hdl->intr_ep_in_buffer = usb_alloc_ep_dmabuffer(usb_id, CDC_INTR_EP_IN | USB_DIR_IN, MAXP_SIZE_CDC_INTRIN);
#endif

    }
    return;
__exit_err:
#if USB_MALLOC_ENABLE
    if (cdc_hdl->cdc_buffer) {
        free(cdc_hdl->cdc_buffer);
    }
    if (cdc_hdl) {
        free(cdc_hdl);
    }
#endif
    cdc_hdl = NULL;
}

void cdc_release(const usb_dev usb_id)
{
    /* log_info("%s() %d", __func__, __LINE__); */
    if (cdc_hdl) {
        if (cdc_hdl->bulk_ep_in_buffer) {
            usb_free_ep_dmabuffer(usb_id, cdc_hdl->bulk_ep_in_buffer);
            cdc_hdl->bulk_ep_in_buffer = NULL;
        }
        if (cdc_hdl->bulk_ep_out_buffer) {
            usb_free_ep_dmabuffer(usb_id, cdc_hdl->bulk_ep_out_buffer);
            cdc_hdl->bulk_ep_out_buffer = NULL;
        }
#if CDC_INTR_EP_ENABLE
        if (cdc_hdl->intr_ep_in_buffer) {
            usb_free_ep_dmabuffer(usb_id, cdc_hdl->intr_ep_in_buffer);
            cdc_hdl->intr_ep_in_buffer = NULL;
        }
#endif
#if USB_MALLOC_ENABLE
        free(cdc_hdl->cdc_buffer);
        free(cdc_hdl);
#endif
        cdc_hdl = NULL;
    }
}

#endif
