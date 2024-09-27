//
// Created by HalfSweet on 24-9-25.
//

#include "hid_comm.h"
#include "usbd_core.h"

#include "dap_main.h"

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[HID_PACKET_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[HID_PACKET_SIZE];

#ifdef CONFIG_USE_HID_CONFIG
/*!< custom hid report descriptor */
const uint8_t hid_custom_report_desc[HID_CUSTOM_REPORT_DESC_SIZE] = {
        /* USER CODE BEGIN 0 */
        0x06, 0x00, 0xff, /* USAGE_PAGE (Vendor Defined Page 1) */
        0x09, 0x01,       /* USAGE (Vendor Usage 1) */
        0xa1, 0x01,       /* COLLECTION (Application) */
        0x85, 0x02,       /*   REPORT ID (0x02) */
        0x09, 0x02,       /*   USAGE (Vendor Usage 1) */
        0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
        0x25, 0xff,       /*LOGICAL_MAXIMUM (255) */
        0x75, 0x08,       /*   REPORT_SIZE (8) */
        0x96, 0xff, 0x03, /*   REPORT_COUNT (1023) */
        0x81, 0x02,       /*   INPUT (Data,Var,Abs) */
        /* <___________________________________________________> */
        0x85, 0x01,       /*   REPORT ID (0x01) */
        0x09, 0x03,       /*   USAGE (Vendor Usage 1) */
        0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
        0x25, 0xff,       /*   LOGICAL_MAXIMUM (255) */
        0x75, 0x08,       /*   REPORT_SIZE (8) */
        0x96, 0xff, 0x03, /*   REPORT_COUNT (1023) */
        0x91, 0x02,       /*   OUTPUT (Data,Var,Abs) */

        /* <___________________________________________________> */
        0x85, 0x03,       /*   REPORT ID (0x03) */
        0x09, 0x04,       /*   USAGE (Vendor Usage 1) */
        0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
        0x25, 0xff,       /*   LOGICAL_MAXIMUM (255) */
        0x75, 0x08,       /*   REPORT_SIZE (8) */
        0x96, 0xff, 0x03, /*   REPORT_COUNT (1023) */
        0xb1, 0x02,       /*   FEATURE (Data,Var,Abs) */
        /* USER CODE END 0 */
        0xC0 /*     END_COLLECTION	             */
};
#endif

#ifdef CONFIG_USE_HID_CONFIG

__WEAK void usbd_hid_custom_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void) busid;
    USB_LOG_RAW("actual in len:%d\r\n", nbytes);
    //    custom_state = HID_STATE_IDLE;
}

__WEAK void usbd_hid_custom_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void) busid;
    USB_LOG_RAW("actual out len:%d\r\n", nbytes);
    usbd_ep_start_read(0, ep, read_buffer, HID_PACKET_SIZE);
    read_buffer[0] = 0x02; /* IN: report id */
    usbd_ep_start_write(0, HID_IN_EP, read_buffer, nbytes);
}

struct usbd_endpoint hid_custom_in_ep = {
        .ep_cb = usbd_hid_custom_in_callback,
        .ep_addr = HID_IN_EP
};

struct usbd_endpoint hid_custom_out_ep = {
        .ep_cb = usbd_hid_custom_out_callback,
        .ep_addr = HID_OUT_EP
};
#endif

