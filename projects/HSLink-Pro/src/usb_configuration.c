#include "usb_configuration.h"
#include "chry_ringbuffer.h"
#include "usbd_core.h"
#include "dap_main.h"
#include "hid_comm.h"

#define USB_CONFIG_SIZE (9 + CMSIS_DAP_INTERFACE_SIZE + CDC_ACM_DESCRIPTOR_LEN + CONFIG_MSC_DESCRIPTOR_LEN + CONFIG_HID_DESCRIPTOR_LEN)
#define INTF_NUM        (2 + 1 + CONFIG_MSC_INTF_NUM + CONFIG_HID_INTF_NUM)

static const uint8_t config_descriptor[] = {
        USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, INTF_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
        /* Interface 0 */
        USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xFF, 0x00, 0x00, 0x02),
        /* Endpoint OUT 2 */
        USB_ENDPOINT_DESCRIPTOR_INIT(DAP_OUT_EP, USB_ENDPOINT_TYPE_BULK, DAP_PACKET_SIZE, 0x00),
        /* Endpoint IN 1 */
        USB_ENDPOINT_DESCRIPTOR_INIT(DAP_IN_EP, USB_ENDPOINT_TYPE_BULK, DAP_PACKET_SIZE, 0x00),
        CDC_ACM_DESCRIPTOR_INIT(0x01, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, DAP_PACKET_SIZE, 0x00),
#ifdef CONFIG_CHERRYDAP_USE_MSC
        MSC_DESCRIPTOR_INIT(MSC_INTF_NUM, MSC_OUT_EP, MSC_IN_EP, DAP_PACKET_SIZE, 0x00),
#endif
#ifdef CONFIG_USE_HID_CONFIG
        HID_DESC()
#endif
};

static const uint8_t other_speed_config_descriptor[] = {
        USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, INTF_NUM, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
        /* Interface 0 */
        USB_INTERFACE_DESCRIPTOR_INIT(0x00, 0x00, 0x02, 0xFF, 0x00, 0x00, 0x02),
        /* Endpoint OUT 2 */
        USB_ENDPOINT_DESCRIPTOR_INIT(DAP_OUT_EP, USB_ENDPOINT_TYPE_BULK, DAP_PACKET_SIZE, 0x00),
        /* Endpoint IN 1 */
        USB_ENDPOINT_DESCRIPTOR_INIT(DAP_IN_EP, USB_ENDPOINT_TYPE_BULK, DAP_PACKET_SIZE, 0x00),
        CDC_ACM_DESCRIPTOR_INIT(0x01, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, DAP_PACKET_SIZE, 0x00),
#ifdef CONFIG_CHERRYDAP_USE_MSC
        MSC_DESCRIPTOR_INIT(MSC_INTF_NUM, MSC_OUT_EP, MSC_IN_EP, DAP_PACKET_SIZE, 0x00),
#endif
#ifdef CONFIG_USE_HID_CONFIG
        HID_DESC()
#endif
};

const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void) speed;
    return config_descriptor;
}

const uint8_t *other_speed_config_descriptor_callback(uint8_t speed)
{
    (void) speed;
    return other_speed_config_descriptor;
}

void USB_Configuration(void)
{
    chry_ringbuffer_init(&g_uartrx, uartrx_ringbuffer, CONFIG_UARTRX_RINGBUF_SIZE);
    chry_ringbuffer_init(&g_usbrx, usbrx_ringbuffer, CONFIG_USBRX_RINGBUF_SIZE);

    DAP_Setup();

    usbd_desc_register(0, &cmsisdap_descriptor);

    /*!< winusb */
    usbd_add_interface(0, &dap_intf);
    usbd_add_endpoint(0, &dap_out_ep);
    usbd_add_endpoint(0, &dap_in_ep);

    /*!< cdc acm */
    usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &intf1));
    usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &intf2));
    usbd_add_endpoint(0, &cdc_out_ep);
    usbd_add_endpoint(0, &cdc_in_ep);

#ifdef CONFIG_USE_HID_CONFIG
    /*!< hid */
    usbd_add_interface(0, usbd_hid_init_intf(0, &hid_intf, hid_custom_report_desc, HID_CUSTOM_REPORT_DESC_SIZE));
    usbd_add_endpoint(0, &hid_custom_in_ep);
    usbd_add_endpoint(0, &hid_custom_out_ep);
#endif

#ifdef CONFIG_CHERRYDAP_USE_MSC
    usbd_add_interface(0, usbd_msc_init_intf(0, &intf3, MSC_OUT_EP, MSC_IN_EP));
#endif
    extern void usbd_event_handler(uint8_t busid, uint8_t event);
    usbd_initialize(0, HPM_USB0_BASE, usbd_event_handler);
}
