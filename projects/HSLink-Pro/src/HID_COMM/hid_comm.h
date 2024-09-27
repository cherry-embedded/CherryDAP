//
// Created by HalfSweet on 24-9-25.
//

#ifndef HSLINK_PRO_HID_COMM_H
#define HSLINK_PRO_HID_COMM_H

#include <stdint.h>
#include "usbd_core.h"
#include "usbd_hid.h"

#ifdef CONFIG_USE_HID_CONFIG

#define HID_IN_EP  0x88
#define HID_OUT_EP 0x09

#ifdef CONFIG_USB_HS
#define HID_PACKET_SIZE 1024
#else
#define HID_PACKET_SIZE 64
#endif

#endif

#ifdef CONFIG_USE_HID_CONFIG
#define HID_IN_EP  0x88
#define HID_OUT_EP 0x09

#ifdef CONFIG_USB_HS
#define HID_PACKET_SIZE 1024
#else
#define HID_PACKET_SIZE 64
#endif

#endif

#ifdef CONFIG_USE_HID_CONFIG
#define CONFIG_HID_DESCRIPTOR_LEN   (9 + 9 + 7 + 7)
#define CONFIG_HID_INTF_NUM         1
#define HID_CUSTOM_REPORT_DESC_SIZE 53
#define HIDRAW_INTERVAL             4
#define HID_INTF_NUM                (MSC_INTF_NUM + 1)

#else
#define CONFIG_HID_DESCRIPTOR_LEN 0
#define CONFIG_HID_INTF_NUM       0
#define HID_INTF_NUM              (MSC_INTF_NUM)
#endif

#ifdef CONFIG_USE_HID_CONFIG
#define HID_DESC() \
    /************** Descriptor of Custom interface *****************/ \
    0x09,                          /* bLength: Interface Descriptor size */ \
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType: Interface descriptor type */ \
    HID_INTF_NUM,                  /* bInterfaceNumber: Number of Interface */ \
    0x00,                          /* bAlternateSetting: Alternate setting */ \
    0x02,                          /* bNumEndpoints */ \
    0x03,                          /* bInterfaceClass: HID */ \
    0x01,                          /* bInterfaceSubClass : 1=BOOT, 0=no boot */ \
    0x00,                          /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */ \
    0,                             /* iInterface: Index of string descriptor */ \
    /******************** Descriptor of Custom HID ********************/ \
    0x09,                    /* bLength: HID Descriptor size */ \
    HID_DESCRIPTOR_TYPE_HID, /* bDescriptorType: HID */ \
    0x11,                    /* bcdHID: HID Class Spec release number */ \
    0x01, \
    0x00,                        /* bCountryCode: Hardware target country */ \
    0x01,                        /* bNumDescriptors: Number of HID class descriptors to follow */ \
    0x22,                        /* bDescriptorType */ \
    HID_CUSTOM_REPORT_DESC_SIZE, /* wItemLength: Total length of Report descriptor */ \
    0x00, \
    /******************** Descriptor of Custom in endpoint ********************/ \
    0x07,                         /* bLength: Endpoint Descriptor size */ \
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: */ \
    HID_IN_EP,                    /* bEndpointAddress: Endpoint Address (IN) */ \
    0x03,                         /* bmAttributes: Interrupt endpoint */ \
    WBVAL(HID_PACKET_SIZE),       /* wMaxPacketSize: 4 Byte max */ \
    HIDRAW_INTERVAL,              /* bInterval: Polling Interval */ \
    /******************** Descriptor of Custom out endpoint ********************/ \
    0x07,                         /* bLength: Endpoint Descriptor size */ \
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: */ \
    HID_OUT_EP,                   /* bEndpointAddress: Endpoint Address (IN) */ \
    0x03,                         /* bmAttributes: Interrupt endpoint */ \
    WBVAL(HID_PACKET_SIZE),       /* wMaxPacketSize: 4 Byte max */ \
    HIDRAW_INTERVAL,              /* bInterval: Polling Interval */
#endif

#ifdef CONFIG_USE_HID_CONFIG
extern struct usbd_endpoint hid_custom_in_ep;
extern struct usbd_endpoint hid_custom_out_ep;

extern const uint8_t hid_custom_report_desc[HID_CUSTOM_REPORT_DESC_SIZE];
#endif

#endif //HSLINK_PRO_HID_COMM_H
