#include "dap_main.h"
#include "DAP_config.h"
#include "DAP.h"

#ifdef CONFIG_USE_HID_CONFIG
#include "usbd_hid.h"
#endif

#define DAP_IN_EP  0x81
#define DAP_OUT_EP 0x02

#define CDC_IN_EP  0x83
#define CDC_OUT_EP 0x04
#define CDC_INT_EP 0x85

#define MSC_IN_EP  0x86
#define MSC_OUT_EP 0x07

#ifdef CONFIG_USE_HID_CONFIG
#define HID_IN_EP  0x88
#define HID_OUT_EP 0x09

#ifdef CONFIG_USB_HS
#define HID_PACKET_SIZE 1024
#else
#define HID_PACKET_SIZE 64
#endif

#endif

#define USBD_VID           0x0D28
#define USBD_PID           0x0204
#define USBD_MAX_POWER     500
#define USBD_LANGID_STRING 1033

#define CMSIS_DAP_INTERFACE_SIZE (9 + 7 + 7)

#ifdef CONFIG_CHERRYDAP_USE_MSC
#define CONFIG_MSC_DESCRIPTOR_LEN CDC_ACM_DESCRIPTOR_LEN
#define CONFIG_MSC_INTF_NUM       1
#define MSC_INTF_NUM              (0x02 + 1)
#else
#define CONFIG_MSC_DESCRIPTOR_LEN 0
#define CONFIG_MSC_INTF_NUM       0
#define MSC_INTF_NUM              (0x02)
#endif

#ifdef CONFIG_USE_HID_CONFIG
#define CONFIG_HID_DESCRIPTOR_LEN   (9 + 9 + 7 + 7)
#define CONFIG_HID_INTF_NUM         1
#define HID_CUSTOM_REPORT_DESC_SIZE 38
#define HIDRAW_INTERVAL             4
#define HID_INTF_NUM                (MSC_INTF_NUM + 1)

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t read_buffer[HID_PACKET_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t write_buffer[HID_PACKET_SIZE];
#else
#define CONFIG_HID_DESCRIPTOR_LEN 0
#define CONFIG_HID_INTF_NUM       0
#define HID_INTF_NUM              (MSC_INTF_NUM)
#endif

#define USB_CONFIG_SIZE (9 + CMSIS_DAP_INTERFACE_SIZE + CDC_ACM_DESCRIPTOR_LEN + CONFIG_MSC_DESCRIPTOR_LEN + CONFIG_HID_DESCRIPTOR_LEN)
#define INTF_NUM        (2 + 1 + CONFIG_MSC_INTF_NUM + CONFIG_HID_INTF_NUM)

#ifdef CONFIG_USB_HS
#if DAP_PACKET_SIZE != 512
#error "DAP_PACKET_SIZE must be 512 in hs"
#endif
#else
#if DAP_PACKET_SIZE != 64
#error "DAP_PACKET_SIZE must be 64 in fs"
#endif
#endif

#define USBD_WINUSB_VENDOR_CODE 0x20

#define USBD_WEBUSB_ENABLE 0
#define USBD_BULK_ENABLE   1
#define USBD_WINUSB_ENABLE 1

/* WinUSB Microsoft OS 2.0 descriptor sizes */
#define WINUSB_DESCRIPTOR_SET_HEADER_SIZE  10
#define WINUSB_FUNCTION_SUBSET_HEADER_SIZE 8
#define WINUSB_FEATURE_COMPATIBLE_ID_SIZE  20

#define FUNCTION_SUBSET_LEN                160
#define DEVICE_INTERFACE_GUIDS_FEATURE_LEN 132

#define USBD_WINUSB_DESC_SET_LEN (WINUSB_DESCRIPTOR_SET_HEADER_SIZE + USBD_WEBUSB_ENABLE * FUNCTION_SUBSET_LEN + USBD_BULK_ENABLE * FUNCTION_SUBSET_LEN)

__ALIGN_BEGIN const uint8_t USBD_WinUSBDescriptorSetDescriptor[] = {
    WBVAL(WINUSB_DESCRIPTOR_SET_HEADER_SIZE), /* wLength */
    WBVAL(WINUSB_SET_HEADER_DESCRIPTOR_TYPE), /* wDescriptorType */
    0x00, 0x00, 0x03, 0x06, /* >= Win 8.1 */  /* dwWindowsVersion*/
    WBVAL(USBD_WINUSB_DESC_SET_LEN),          /* wDescriptorSetTotalLength */
#if (USBD_WEBUSB_ENABLE)
    WBVAL(WINUSB_FUNCTION_SUBSET_HEADER_SIZE), // wLength
    WBVAL(WINUSB_SUBSET_HEADER_FUNCTION_TYPE), // wDescriptorType
    0,                                         // bFirstInterface USBD_WINUSB_IF_NUM
    0,                                         // bReserved
    WBVAL(FUNCTION_SUBSET_LEN),                // wSubsetLength
    WBVAL(WINUSB_FEATURE_COMPATIBLE_ID_SIZE),  // wLength
    WBVAL(WINUSB_FEATURE_COMPATIBLE_ID_TYPE),  // wDescriptorType
    'W', 'I', 'N', 'U', 'S', 'B', 0, 0,        // CompatibleId
    0, 0, 0, 0, 0, 0, 0, 0,                    // SubCompatibleId
    WBVAL(DEVICE_INTERFACE_GUIDS_FEATURE_LEN), // wLength
    WBVAL(WINUSB_FEATURE_REG_PROPERTY_TYPE),   // wDescriptorType
    WBVAL(WINUSB_PROP_DATA_TYPE_REG_MULTI_SZ), // wPropertyDataType
    WBVAL(42),                                 // wPropertyNameLength
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0,
    'G', 0, 'U', 0, 'I', 0, 'D', 0, 's', 0, 0, 0,
    WBVAL(80), // wPropertyDataLength
    '{', 0,
    '9', 0, '2', 0, 'C', 0, 'E', 0, '6', 0, '4', 0, '6', 0, '2', 0, '-', 0,
    '9', 0, 'C', 0, '7', 0, '7', 0, '-', 0,
    '4', 0, '6', 0, 'F', 0, 'E', 0, '-', 0,
    '9', 0, '3', 0, '3', 0, 'B', 0, '-',
    0, '3', 0, '1', 0, 'C', 0, 'B', 0, '9', 0, 'C', 0, '5', 0, 'A', 0, 'A', 0, '3', 0, 'B', 0, '9', 0,
    '}', 0, 0, 0, 0, 0
#endif
#if USBD_BULK_ENABLE
    WBVAL(WINUSB_FUNCTION_SUBSET_HEADER_SIZE), /* wLength */
    WBVAL(WINUSB_SUBSET_HEADER_FUNCTION_TYPE), /* wDescriptorType */
    0,                                         /* bFirstInterface USBD_BULK_IF_NUM*/
    0,                                         /* bReserved */
    WBVAL(FUNCTION_SUBSET_LEN),                /* wSubsetLength */
    WBVAL(WINUSB_FEATURE_COMPATIBLE_ID_SIZE),  /* wLength */
    WBVAL(WINUSB_FEATURE_COMPATIBLE_ID_TYPE),  /* wDescriptorType */
    'W', 'I', 'N', 'U', 'S', 'B', 0, 0,        /* CompatibleId*/
    0, 0, 0, 0, 0, 0, 0, 0,                    /* SubCompatibleId*/
    WBVAL(DEVICE_INTERFACE_GUIDS_FEATURE_LEN), /* wLength */
    WBVAL(WINUSB_FEATURE_REG_PROPERTY_TYPE),   /* wDescriptorType */
    WBVAL(WINUSB_PROP_DATA_TYPE_REG_MULTI_SZ), /* wPropertyDataType */
    WBVAL(42),                                 /* wPropertyNameLength */
    'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0,
    'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0, 'a', 0, 'c', 0, 'e', 0,
    'G', 0, 'U', 0, 'I', 0, 'D', 0, 's', 0, 0, 0,
    WBVAL(80), /* wPropertyDataLength */
    '{', 0,
    'C', 0, 'D', 0, 'B', 0, '3', 0, 'B', 0, '5', 0, 'A', 0, 'D', 0, '-', 0,
    '2', 0, '9', 0, '3', 0, 'B', 0, '-', 0,
    '4', 0, '6', 0, '6', 0, '3', 0, '-', 0,
    'A', 0, 'A', 0, '3', 0, '6', 0, '-',
    0, '1', 0, 'A', 0, 'A', 0, 'E', 0, '4', 0, '6', 0, '4', 0, '6', 0, '3', 0, '7', 0, '7', 0, '6', 0,
    '}', 0, 0, 0, 0, 0
#endif
};

#define USBD_NUM_DEV_CAPABILITIES (USBD_WEBUSB_ENABLE + USBD_WINUSB_ENABLE)

#define USBD_WEBUSB_DESC_LEN 24
#define USBD_WINUSB_DESC_LEN 28

#define USBD_BOS_WTOTALLENGTH (0x05 +                                      \
                               USBD_WEBUSB_DESC_LEN * USBD_WEBUSB_ENABLE + \
                               USBD_WINUSB_DESC_LEN * USBD_WINUSB_ENABLE)

__ALIGN_BEGIN const uint8_t USBD_BinaryObjectStoreDescriptor[] = {
    0x05,                         /* bLength */
    0x0f,                         /* bDescriptorType */
    WBVAL(USBD_BOS_WTOTALLENGTH), /* wTotalLength */
    USBD_NUM_DEV_CAPABILITIES,    /* bNumDeviceCaps */
#if (USBD_WEBUSB_ENABLE)
    USBD_WEBUSB_DESC_LEN,           /* bLength */
    0x10,                           /* bDescriptorType */
    USB_DEVICE_CAPABILITY_PLATFORM, /* bDevCapabilityType */
    0x00,                           /* bReserved */
    0x38, 0xB6, 0x08, 0x34,         /* PlatformCapabilityUUID */
    0xA9, 0x09, 0xA0, 0x47,
    0x8B, 0xFD, 0xA0, 0x76,
    0x88, 0x15, 0xB6, 0x65,
    WBVAL(0x0100), /* 1.00 */ /* bcdVersion */
    USBD_WINUSB_VENDOR_CODE,  /* bVendorCode */
    0,                        /* iLandingPage */
#endif
#if (USBD_WINUSB_ENABLE)
    USBD_WINUSB_DESC_LEN,           /* bLength */
    0x10,                           /* bDescriptorType */
    USB_DEVICE_CAPABILITY_PLATFORM, /* bDevCapabilityType */
    0x00,                           /* bReserved */
    0xDF, 0x60, 0xDD, 0xD8,         /* PlatformCapabilityUUID */
    0x89, 0x45, 0xC7, 0x4C,
    0x9C, 0xD2, 0x65, 0x9D,
    0x9E, 0x64, 0x8A, 0x9F,
    0x00, 0x00, 0x03, 0x06, /* >= Win 8.1 */ /* dwWindowsVersion*/
    WBVAL(USBD_WINUSB_DESC_SET_LEN),         /* wDescriptorSetTotalLength */
    USBD_WINUSB_VENDOR_CODE,                 /* bVendorCode */
    0,                                       /* bAltEnumCode */
#endif
};

static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_1, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
};

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
    /************** Descriptor of Custom interface *****************/
    0x09,                          /* bLength: Interface Descriptor size */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType: Interface descriptor type */
    HID_INTF_NUM,                  /* bInterfaceNumber: Number of Interface */
    0x00,                          /* bAlternateSetting: Alternate setting */
    0x02,                          /* bNumEndpoints */
    0x03,                          /* bInterfaceClass: HID */
    0x01,                          /* bInterfaceSubClass : 1=BOOT, 0=no boot */
    0x00,                          /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
    0,                             /* iInterface: Index of string descriptor */
    /******************** Descriptor of Custom HID ********************/
    0x09,                    /* bLength: HID Descriptor size */
    HID_DESCRIPTOR_TYPE_HID, /* bDescriptorType: HID */
    0x11,                    /* bcdHID: HID Class Spec release number */
    0x01,
    0x00,                        /* bCountryCode: Hardware target country */
    0x01,                        /* bNumDescriptors: Number of HID class descriptors to follow */
    0x22,                        /* bDescriptorType */
    HID_CUSTOM_REPORT_DESC_SIZE, /* wItemLength: Total length of Report descriptor */
    0x00,
    /******************** Descriptor of Custom in endpoint ********************/
    0x07,                         /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: */
    HID_IN_EP,                    /* bEndpointAddress: Endpoint Address (IN) */
    0x03,                         /* bmAttributes: Interrupt endpoint */
    WBVAL(HID_PACKET_SIZE),       /* wMaxPacketSize: 4 Byte max */
    HIDRAW_INTERVAL,              /* bInterval: Polling Interval */
    /******************** Descriptor of Custom out endpoint ********************/
    0x07,                         /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: */
    HID_OUT_EP,                   /* bEndpointAddress: Endpoint Address (IN) */
    0x03,                         /* bmAttributes: Interrupt endpoint */
    WBVAL(HID_PACKET_SIZE),       /* wMaxPacketSize: 4 Byte max */
    HIDRAW_INTERVAL,              /* bInterval: Polling Interval */
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
    /************** Descriptor of Custom interface *****************/
    0x09,                          /* bLength: Interface Descriptor size */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType: Interface descriptor type */
    HID_INTF_NUM,                  /* bInterfaceNumber: Number of Interface */
    0x00,                          /* bAlternateSetting: Alternate setting */
    0x02,                          /* bNumEndpoints */
    0x03,                          /* bInterfaceClass: HID */
    0x01,                          /* bInterfaceSubClass : 1=BOOT, 0=no boot */
    0x00,                          /* nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse */
    0,                             /* iInterface: Index of string descriptor */
    /******************** Descriptor of Custom HID ********************/
    0x09,                    /* bLength: HID Descriptor size */
    HID_DESCRIPTOR_TYPE_HID, /* bDescriptorType: HID */
    0x11,                    /* bcdHID: HID Class Spec release number */
    0x01,
    0x00,                        /* bCountryCode: Hardware target country */
    0x01,                        /* bNumDescriptors: Number of HID class descriptors to follow */
    0x22,                        /* bDescriptorType */
    HID_CUSTOM_REPORT_DESC_SIZE, /* wItemLength: Total length of Report descriptor */
    0x00,
    /******************** Descriptor of Custom in endpoint ********************/
    0x07,                         /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: */
    HID_IN_EP,                    /* bEndpointAddress: Endpoint Address (IN) */
    0x03,                         /* bmAttributes: Interrupt endpoint */
    WBVAL(HID_PACKET_SIZE),       /* wMaxPacketSize: 4 Byte max */
    HIDRAW_INTERVAL,              /* bInterval: Polling Interval */
    /******************** Descriptor of Custom out endpoint ********************/
    0x07,                         /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: */
    HID_OUT_EP,                   /* bEndpointAddress: Endpoint Address (IN) */
    0x03,                         /* bmAttributes: Interrupt endpoint */
    WBVAL(HID_PACKET_SIZE),       /* wMaxPacketSize: 4 Byte max */
    HIDRAW_INTERVAL,              /* bInterval: Polling Interval */
#endif
};

char *string_descriptors[] = {
    (char[]){ 0x09, 0x04 },             /* Langid */
    "CherryUSB",                        /* Manufacturer */
    "CherryUSB CMSIS-DAP",              /* Product */
    "00000000000000000123456789ABCDEF", /* Serial Number */
};

static const uint8_t device_quality_descriptor[] = {
    USB_DEVICE_QUALIFIER_DESCRIPTOR_INIT(USB_2_1, 0x00, 0x00, 0x00, 0x01),
};

static const uint8_t *device_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return config_descriptor;
}

static const uint8_t *device_quality_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

static const uint8_t *other_speed_config_descriptor_callback(uint8_t speed)
{
    (void)speed;
    return other_speed_config_descriptor;
}

static const char *string_descriptor_callback(uint8_t speed, uint8_t index)
{
    (void)speed;

    if (index >= (sizeof(string_descriptors) / sizeof(char *))) {
        return NULL;
    }
    return string_descriptors[index];
}

#ifdef CONFIG_USE_HID_CONFIG
/*!< custom hid report descriptor */
static const uint8_t hid_custom_report_desc[HID_CUSTOM_REPORT_DESC_SIZE] = {
    /* USER CODE BEGIN 0 */
    0x06, 0x00, 0xff, /* USAGE_PAGE (Vendor Defined Page 1) */
    0x09, 0x01,       /* USAGE (Vendor Usage 1) */
    0xa1, 0x01,       /* COLLECTION (Application) */
    0x85, 0x02,       /*   REPORT ID (0x02) */
    0x09, 0x02,       /*   USAGE (Vendor Usage 1) */
    0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
    0x25, 0xff,       /*LOGICAL_MAXIMUM (255) */
    0x75, 0x08,       /*   REPORT_SIZE (8) */
    0x96, 0xff, 0x03, /*   REPORT_COUNT (63) */
    0x81, 0x02,       /*   INPUT (Data,Var,Abs) */
    /* <___________________________________________________> */
    0x85, 0x01,       /*   REPORT ID (0x01) */
    0x09, 0x03,       /*   USAGE (Vendor Usage 1) */
    0x15, 0x00,       /*   LOGICAL_MINIMUM (0) */
    0x25, 0xff,       /*   LOGICAL_MAXIMUM (255) */
    0x75, 0x08,       /*   REPORT_SIZE (8) */
    0x96, 0xff, 0x03, /*   REPORT_COUNT (63) */
    0x91, 0x02,       /*   OUTPUT (Data,Var,Abs) */
    /* USER CODE END 0 */
    0xC0 /*     END_COLLECTION	             */
};
#endif

static volatile uint16_t USB_RequestIndexI; // Request  Index In
static volatile uint16_t USB_RequestIndexO; // Request  Index Out
static volatile uint16_t USB_RequestCountI; // Request  Count In
static volatile uint16_t USB_RequestCountO; // Request  Count Out
static volatile uint8_t USB_RequestIdle;    // Request  Idle  Flag

static volatile uint16_t USB_ResponseIndexI; // Response Index In
static volatile uint16_t USB_ResponseIndexO; // Response Index Out
static volatile uint16_t USB_ResponseCountI; // Response Count In
static volatile uint16_t USB_ResponseCountO; // Response Count Out
static volatile uint8_t USB_ResponseIdle;    // Response Idle  Flag

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t USB_Request[DAP_PACKET_COUNT][DAP_PACKET_SIZE];  // Request  Buffer
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t USB_Response[DAP_PACKET_COUNT][DAP_PACKET_SIZE]; // Response Buffer
static uint16_t USB_RespSize[DAP_PACKET_COUNT];                                                        // Response Size

volatile struct cdc_line_coding g_cdc_lincoding;
volatile uint8_t config_uart = 0;
volatile uint8_t config_uart_transfer = 0;

#define CONFIG_UARTRX_RINGBUF_SIZE (8 * 1024)
#define CONFIG_USBRX_RINGBUF_SIZE  (8 * 1024)

static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t uartrx_ringbuffer[CONFIG_UARTRX_RINGBUF_SIZE];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t usbrx_ringbuffer[CONFIG_USBRX_RINGBUF_SIZE];
static USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t usb_tmpbuffer[DAP_PACKET_SIZE];

static volatile uint8_t usbrx_idle_flag = 0;
static volatile uint8_t usbtx_idle_flag = 0;
static volatile uint8_t uarttx_idle_flag = 0;

USB_NOCACHE_RAM_SECTION chry_ringbuffer_t g_uartrx;
USB_NOCACHE_RAM_SECTION chry_ringbuffer_t g_usbrx;

void usbd_event_handler(uint8_t busid, uint8_t event)
{
    (void)busid;
    switch (event) {
        case USBD_EVENT_RESET:
            usbrx_idle_flag = 0;
            usbtx_idle_flag = 0;
            uarttx_idle_flag = 0;
            config_uart_transfer = 0;
            break;
        case USBD_EVENT_CONNECTED:
            break;
        case USBD_EVENT_DISCONNECTED:
            break;
        case USBD_EVENT_RESUME:
            break;
        case USBD_EVENT_SUSPEND:
            break;
        case USBD_EVENT_CONFIGURED:
            /* setup first out ep read transfer */
            USB_RequestIdle = 0U;

            usbd_ep_start_read(0, DAP_OUT_EP, USB_Request[0], DAP_PACKET_SIZE);
            usbd_ep_start_read(0, CDC_OUT_EP, usb_tmpbuffer, DAP_PACKET_SIZE);

#ifdef CONFIG_USE_HID_CONFIG
            usbd_ep_start_read(0, HID_OUT_EP, read_buffer, HID_PACKET_SIZE);
#endif

            break;
        case USBD_EVENT_SET_REMOTE_WAKEUP:
            break;
        case USBD_EVENT_CLR_REMOTE_WAKEUP:
            break;

        default:
            break;
    }
}

void dap_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    if (USB_Request[USB_RequestIndexI][0] == ID_DAP_TransferAbort) {
        DAP_TransferAbort = 1U;
    } else {
        USB_RequestIndexI++;
        if (USB_RequestIndexI == DAP_PACKET_COUNT) {
            USB_RequestIndexI = 0U;
        }
        USB_RequestCountI++;
    }

    // Start reception of next request packet
    if ((uint16_t)(USB_RequestCountI - USB_RequestCountO) != DAP_PACKET_COUNT) {
        usbd_ep_start_read(0, DAP_OUT_EP, USB_Request[USB_RequestIndexI], DAP_PACKET_SIZE);
    } else {
        USB_RequestIdle = 1U;
    }
}

void dap_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    if (USB_ResponseCountI != USB_ResponseCountO) {
        // Load data from response buffer to be sent back
        usbd_ep_start_write(0, DAP_IN_EP, USB_Response[USB_ResponseIndexO], USB_RespSize[USB_ResponseIndexO]);
        USB_ResponseIndexO++;
        if (USB_ResponseIndexO == DAP_PACKET_COUNT) {
            USB_ResponseIndexO = 0U;
        }
        USB_ResponseCountO++;
    } else {
        USB_ResponseIdle = 1U;
    }
}

void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    chry_ringbuffer_write(&g_usbrx, usb_tmpbuffer, nbytes);
    if (chry_ringbuffer_get_free(&g_usbrx) >= DAP_PACKET_SIZE) {
        usbd_ep_start_read(0, CDC_OUT_EP, usb_tmpbuffer, DAP_PACKET_SIZE);
    } else {
        usbrx_idle_flag = 1;
    }
}

void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    uint32_t size;
    uint8_t *buffer;

    chry_ringbuffer_linear_read_done(&g_uartrx, nbytes);
    if ((nbytes % DAP_PACKET_SIZE) == 0 && nbytes) {
        /* send zlp */
        usbd_ep_start_write(0, CDC_IN_EP, NULL, 0);
    } else {
        if (chry_ringbuffer_get_used(&g_uartrx)) {
            buffer = chry_ringbuffer_linear_read_setup(&g_uartrx, &size);
            usbd_ep_start_write(0, CDC_IN_EP, buffer, size);
        } else {
            usbtx_idle_flag = 1;
        }
    }
}

static struct usbd_endpoint dap_out_ep = {
    .ep_addr = DAP_OUT_EP,
    .ep_cb = dap_out_callback
};

static struct usbd_endpoint dap_in_ep = {
    .ep_addr = DAP_IN_EP,
    .ep_cb = dap_in_callback
};

static struct usbd_endpoint cdc_out_ep = {
    .ep_addr = CDC_OUT_EP,
    .ep_cb = usbd_cdc_acm_bulk_out
};

static struct usbd_endpoint cdc_in_ep = {
    .ep_addr = CDC_IN_EP,
    .ep_cb = usbd_cdc_acm_bulk_in
};

#ifdef CONFIG_USE_HID_CONFIG

__WEAK void usbd_hid_custom_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    USB_LOG_RAW("actual in len:%d\r\n", nbytes);
    //    custom_state = HID_STATE_IDLE;
}

__WEAK void usbd_hid_custom_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    USB_LOG_RAW("actual out len:%d\r\n", nbytes);
    usbd_ep_start_read(0, ep, read_buffer, HID_PACKET_SIZE);
    read_buffer[0] = 0x02; /* IN: report id */
    usbd_ep_start_write(0, HID_IN_EP, read_buffer, nbytes);
}

static struct usbd_endpoint hid_custom_in_ep = {
    .ep_cb = usbd_hid_custom_in_callback,
    .ep_addr = HID_IN_EP
};

static struct usbd_endpoint hid_custom_out_ep = {
    .ep_cb = usbd_hid_custom_out_callback,
    .ep_addr = HID_OUT_EP
};
#endif

struct usbd_interface dap_intf;
struct usbd_interface intf1;
struct usbd_interface intf2;
struct usbd_interface intf3;
struct usbd_interface hid_intf;

static void chry_dap_state_init(void)
{
    // Initialize variables
    USB_RequestIndexI = 0U;
    USB_RequestIndexO = 0U;
    USB_RequestCountI = 0U;
    USB_RequestCountO = 0U;
    USB_RequestIdle = 1U;
    USB_ResponseIndexI = 0U;
    USB_ResponseIndexO = 0U;
    USB_ResponseCountI = 0U;
    USB_ResponseCountO = 0U;
    USB_ResponseIdle = 1U;
}

struct usb_msosv2_descriptor msosv2_desc = {
    .vendor_code = USBD_WINUSB_VENDOR_CODE,
    .compat_id = USBD_WinUSBDescriptorSetDescriptor,
    .compat_id_len = USBD_WINUSB_DESC_SET_LEN,
};

struct usb_bos_descriptor bos_desc = {
    .string = USBD_BinaryObjectStoreDescriptor,
    .string_len = USBD_BOS_WTOTALLENGTH
};

const struct usb_descriptor cmsisdap_descriptor = {
    .device_descriptor_callback = device_descriptor_callback,
    .config_descriptor_callback = config_descriptor_callback,
    .device_quality_descriptor_callback = device_quality_descriptor_callback,
    .other_speed_descriptor_callback = other_speed_config_descriptor_callback,
    .string_descriptor_callback = string_descriptor_callback,
    .bos_descriptor = &bos_desc,
    .msosv2_descriptor = &msosv2_desc,
};

void chry_dap_init(uint8_t busid, uint32_t reg_base)
{
    chry_ringbuffer_init(&g_uartrx, uartrx_ringbuffer, CONFIG_UARTRX_RINGBUF_SIZE);
    chry_ringbuffer_init(&g_usbrx, usbrx_ringbuffer, CONFIG_USBRX_RINGBUF_SIZE);

    DAP_Setup();

    chry_dap_state_init();

    usbd_desc_register(0, &cmsisdap_descriptor);
    //    usbd_bos_desc_register(0, &bos_desc);
    //    usbd_msosv2_desc_register(0, &msosv2_desc);

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
    usbd_initialize(busid, reg_base, usbd_event_handler);
}

void chry_dap_handle(void)
{
    uint32_t n;

    // Process pending requests
    while (USB_RequestCountI != USB_RequestCountO) {
        // Handle Queue Commands
        n = USB_RequestIndexO;
        while (USB_Request[n][0] == ID_DAP_QueueCommands) {
            USB_Request[n][0] = ID_DAP_ExecuteCommands;
            n++;
            if (n == DAP_PACKET_COUNT) {
                n = 0U;
            }
            if (n == USB_RequestIndexI) {
                // flags = osThreadFlagsWait(0x81U, osFlagsWaitAny, osWaitForever);
                // if (flags & 0x80U) {
                //     break;
                // }
            }
        }

        // Execute DAP Command (process request and prepare response)
        USB_RespSize[USB_ResponseIndexI] =
            (uint16_t)DAP_ExecuteCommand(USB_Request[USB_RequestIndexO], USB_Response[USB_ResponseIndexI]);

        // Update Request Index and Count
        USB_RequestIndexO++;
        if (USB_RequestIndexO == DAP_PACKET_COUNT) {
            USB_RequestIndexO = 0U;
        }
        USB_RequestCountO++;

        if (USB_RequestIdle) {
            if ((uint16_t)(USB_RequestCountI - USB_RequestCountO) != DAP_PACKET_COUNT) {
                USB_RequestIdle = 0U;
                usbd_ep_start_read(0, DAP_OUT_EP, USB_Request[USB_RequestIndexI], DAP_PACKET_SIZE);
            }
        }

        // Update Response Index and Count
        USB_ResponseIndexI++;
        if (USB_ResponseIndexI == DAP_PACKET_COUNT) {
            USB_ResponseIndexI = 0U;
        }
        USB_ResponseCountI++;

        if (USB_ResponseIdle) {
            if (USB_ResponseCountI != USB_ResponseCountO) {
                // Load data from response buffer to be sent back
                n = USB_ResponseIndexO++;
                if (USB_ResponseIndexO == DAP_PACKET_COUNT) {
                    USB_ResponseIndexO = 0U;
                }
                USB_ResponseCountO++;
                USB_ResponseIdle = 0U;
                usbd_ep_start_write(0, DAP_IN_EP, USB_Response[n], USB_RespSize[n]);
            }
        }
    }
}

void usbd_cdc_acm_set_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    if (memcmp(line_coding, (uint8_t *)&g_cdc_lincoding, sizeof(struct cdc_line_coding)) != 0) {
        memcpy((uint8_t *)&g_cdc_lincoding, line_coding, sizeof(struct cdc_line_coding));
        config_uart = 1;
        config_uart_transfer = 0;
    }
}

void usbd_cdc_acm_get_line_coding(uint8_t busid, uint8_t intf, struct cdc_line_coding *line_coding)
{
    (void)busid;
    memcpy(line_coding, (uint8_t *)&g_cdc_lincoding, sizeof(struct cdc_line_coding));
}

void chry_dap_usb2uart_handle(void)
{
    uint32_t size;
    uint8_t *buffer;

    if (config_uart) {
        /* disable irq here */
        config_uart = 0;
        /* config uart here */
        chry_dap_usb2uart_uart_config_callback((struct cdc_line_coding *)&g_cdc_lincoding);
        usbtx_idle_flag = 1;
        uarttx_idle_flag = 1;
        config_uart_transfer = 1;
        //chry_ringbuffer_reset_read(&g_uartrx);
        /* enable irq here */
    }

    if (config_uart_transfer == 0) {
        return;
    }

    /* why we use chry_ringbuffer_linear_read_setup?
     * becase we use dma and we do not want to use temp buffer to memcpy from ringbuffer
     * 
    */

    /* uartrx to usb tx */
    if (usbtx_idle_flag) {
        if (chry_ringbuffer_get_used(&g_uartrx)) {
            usbtx_idle_flag = 0;
            /* start first transfer */
            buffer = chry_ringbuffer_linear_read_setup(&g_uartrx, &size);
            usbd_ep_start_write(0, CDC_IN_EP, buffer, size);
        }
    }

    /* usbrx to uart tx */
    if (uarttx_idle_flag) {
        if (chry_ringbuffer_get_used(&g_usbrx)) {
            uarttx_idle_flag = 0;
            /* start first transfer */
            buffer = chry_ringbuffer_linear_read_setup(&g_usbrx, &size);
            chry_dap_usb2uart_uart_send_bydma(buffer, size);
        }
    }

    /* check whether usb rx ringbuffer have space to store */
    if (usbrx_idle_flag) {
        if (chry_ringbuffer_get_free(&g_usbrx) >= DAP_PACKET_SIZE) {
            usbrx_idle_flag = 0;
            usbd_ep_start_read(0, CDC_OUT_EP, usb_tmpbuffer, DAP_PACKET_SIZE);
        }
    }
}

/* implment by user */
__WEAK void chry_dap_usb2uart_uart_config_callback(struct cdc_line_coding *line_coding)
{
}

/* called by user */
void chry_dap_usb2uart_uart_send_complete(uint32_t size)
{
    uint8_t *buffer;

    chry_ringbuffer_linear_read_done(&g_usbrx, size);

    if (chry_ringbuffer_get_used(&g_usbrx)) {
        buffer = chry_ringbuffer_linear_read_setup(&g_usbrx, &size);
        chry_dap_usb2uart_uart_send_bydma(buffer, size);
    } else {
        uarttx_idle_flag = 1;
    }
}

/* implment by user */
__WEAK void chry_dap_usb2uart_uart_send_bydma(uint8_t *data, uint16_t len)
{
}

#ifdef CONFIG_CHERRYDAP_USE_MSC
#define BLOCK_SIZE  512
#define BLOCK_COUNT 10

typedef struct
{
    uint8_t BlockSpace[BLOCK_SIZE];
} BLOCK_TYPE;

BLOCK_TYPE mass_block[BLOCK_COUNT];

void usbd_msc_get_cap(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    *block_num = 1000; //Pretend having so many buffer,not has actually.
    *block_size = BLOCK_SIZE;
}
int usbd_msc_sector_read(uint32_t sector, uint8_t *buffer, uint32_t length)
{
    if (sector < 10)
        memcpy(buffer, mass_block[sector].BlockSpace, length);
    return 0;
}

int usbd_msc_sector_write(uint32_t sector, uint8_t *buffer, uint32_t length)
{
    if (sector < 10)
        memcpy(mass_block[sector].BlockSpace, buffer, length);
    return 0;
}
#endif
