#include "hid_comm.h"
#include "usbd_core.h"
#include "dap_main.h"
#include "setting.h"

#ifdef CONFIG_USE_HID_CONFIG

#include "cJSON.h"

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t HID_read_buffer[HID_PACKET_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t HID_write_buffer[HID_PACKET_SIZE];

typedef enum
{
    HID_STATE_BUSY = 0,
    HID_STATE_DONE,
} HID_State_t;

typedef int8_t (*HID_Command_Handler_t)(char *res, const cJSON *root);

typedef struct
{
    const char *command;
    HID_Command_Handler_t handler;
} HID_Command_t;

typedef enum
{
    HID_RESPONSE_SUCCESS = 0,
    HID_RESPONSE_FAILED,
} HID_Response_t;

const char *response_str[] = {
    "success",
    "failed"
};

static volatile HID_State_t HID_ReadState = HID_STATE_BUSY;

/*!< custom hid report descriptor */
const uint8_t hid_custom_report_desc[HID_CUSTOM_REPORT_DESC_SIZE] = {
    /* USER CODE BEGIN 0 */
    0x06, 0x00, 0xff, /* USAGE_PAGE (Vendor Defined Page 1) */
    0x09, 0x01, /* USAGE (Vendor Usage 1) */
    0xa1, 0x01, /* COLLECTION (Application) */
    0x85, 0x02, /*   REPORT ID (0x02) */
    0x09, 0x02, /*   USAGE (Vendor Usage 1) */
    0x15, 0x00, /*   LOGICAL_MINIMUM (0) */
    0x25, 0xff, /*LOGICAL_MAXIMUM (255) */
    0x75, 0x08, /*   REPORT_SIZE (8) */
    0x96, 0xff, 0x03, /*   REPORT_COUNT (1023) */
    0x81, 0x02, /*   INPUT (Data,Var,Abs) */
    /* <___________________________________________________> */
    0x85, 0x01, /*   REPORT ID (0x01) */
    0x09, 0x03, /*   USAGE (Vendor Usage 1) */
    0x15, 0x00, /*   LOGICAL_MINIMUM (0) */
    0x25, 0xff, /*   LOGICAL_MAXIMUM (255) */
    0x75, 0x08, /*   REPORT_SIZE (8) */
    0x96, 0xff, 0x03, /*   REPORT_COUNT (1023) */
    0x91, 0x02, /*   OUTPUT (Data,Var,Abs) */

    /* <___________________________________________________> */
    0x85, 0x03, /*   REPORT ID (0x03) */
    0x09, 0x04, /*   USAGE (Vendor Usage 1) */
    0x15, 0x00, /*   LOGICAL_MINIMUM (0) */
    0x25, 0xff, /*   LOGICAL_MAXIMUM (255) */
    0x75, 0x08, /*   REPORT_SIZE (8) */
    0x96, 0xff, 0x03, /*   REPORT_COUNT (1023) */
    0xb1, 0x02, /*   FEATURE (Data,Var,Abs) */
    /* USER CODE END 0 */
    0xC0 /*     END_COLLECTION	             */
};

void usbd_hid_custom_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void) busid;
    (void) ep;
    USB_LOG_DBG("actual in len:%d\r\n", nbytes);
    // custom_state = HID_STATE_IDLE;
}

void usbd_hid_custom_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void) busid;
    HID_ReadState = HID_STATE_DONE;
    // usbd_ep_start_read(0, HID_OUT_EP, HID_read_buffer, HID_PACKET_SIZE);// 重新开启读取
}

struct usbd_endpoint hid_custom_in_ep = {
    .ep_cb = usbd_hid_custom_in_callback,
    .ep_addr = HID_IN_EP
};

struct usbd_endpoint hid_custom_out_ep = {
    .ep_cb = usbd_hid_custom_out_callback,
    .ep_addr = HID_OUT_EP
};

static void Add_ResponseState(cJSON *response, HID_Response_t state)
{
    cJSON_AddStringToObject(response, "state", response_str[state]);
}

static int8_t Hello(char *res, const cJSON *root)
{
    (void) root;
    int8_t ret = -1;
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "serial", string_descriptors[3]);
    cJSON_AddStringToObject(response, "model", "HSLink-Pro");
    cJSON_AddStringToObject(response, "version", CONFIG_BUILD_VERSION);
    cJSON_AddStringToObject(response, "bootloader", "1.0.0"); // 以后再改

    char *response_str = cJSON_PrintUnformatted(response);
    if (response_str == NULL)
    {
        USB_LOG_ERR("Hello command print json failed\n");
        goto exit;
    }

    strcpy(res, response_str);
    cJSON_free(response_str);
    ret = 0;
exit:
    cJSON_Delete(response);
    return ret;
}

static int8_t settings(char *res, const cJSON *root)
{
    int8_t ret = -1;
    cJSON *response = cJSON_CreateObject();
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (data == NULL)
    {
        const char *message = "data object not found";
        USB_LOG_ERR("%s\n", message);
        cJSON_AddStringToObject(response, "message", message);
        goto fail;
    }

    HSLink_Setting.boost = (bool) cJSON_GetObjectItem(data, "boost")->valueint;
    char *swd = cJSON_GetObjectItem(data, "swd_port_mode")->valuestring;
    if (strcmp(swd, "spi") == 0)
    {
        HSLink_Setting.swd_port_mode = PORT_MODE_SPI;
    }
    else
    {
        // 其它任何字段都当作GPIO处理
        HSLink_Setting.swd_port_mode = PORT_MODE_GPIO;
    }

    char *jtag = cJSON_GetObjectItem(data, "jtag_port_mode")->valuestring;
    if (strcmp(jtag, "spi") == 0)
    {
        HSLink_Setting.jtag_port_mode = PORT_MODE_SPI;
    }
    else
    {
        // 其它任何字段都当作GPIO处理
        HSLink_Setting.jtag_port_mode = PORT_MODE_GPIO;
    }

    cJSON *power = cJSON_GetObjectItem(data, "power");
    HSLink_Setting.power.enable = (bool) cJSON_GetObjectItem(power, "enable")->valueint;
    HSLink_Setting.power.voltage = cJSON_GetObjectItem(power, "voltage")->valuedouble;
    HSLink_Setting.power.power_on = (bool) cJSON_GetObjectItem(power, "power_on")->valueint;
    HSLink_Setting.power.port_on = (bool) cJSON_GetObjectItem(power, "port_on")->valueint;

    int reset_size = cJSON_GetArraySize(cJSON_GetObjectItem(data, "reset"));
    cJSON *reset_array = cJSON_GetObjectItem(data, "reset");
    HSLink_Setting.reset = 0;
    for (int i = 0; i < reset_size; i++)
    {
        cJSON *reset = cJSON_GetArrayItem(reset_array, i);
        if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_NRST) != 1 && strcmp(reset->valuestring, "nrst") == 0)
        {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_NRST);
        }
        else if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_POR) != 1 && strcmp(reset->valuestring, "por") == 0)
        {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_POR);
        }
        else if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT) != 1 && strcmp(reset->valuestring,
                     "arm_swd_soft") == 0)
        {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT);
        }
    }

    HSLink_Setting.led = (bool) cJSON_GetObjectItem(data, "led")->valueint;
    HSLink_Setting.led_brightness = cJSON_GetObjectItem(data, "led_brightness")->valueint;


    Add_ResponseState(response, HID_RESPONSE_SUCCESS);
    cJSON_AddStringToObject(response, "message", "settings success");
    ret = 0;
    Setting_Save();
    goto exit;
fail:
    Add_ResponseState(response, HID_RESPONSE_FAILED);
    // 错误原因由错误的分支自行填充
    // cJSON_AddStringToObject(response, "message", "settings failed");
exit:
    char *response_str = cJSON_PrintUnformatted(response);
    strcpy(res, response_str);
    cJSON_free(response_str);
    cJSON_Delete(response);
    return ret;
}

static const HID_Command_t hid_command[] = {
    {"Hello", Hello},
    {"settings", settings}
};

void HID_Handle(void)
{
    if (HID_ReadState == HID_STATE_BUSY)
    {
        return; // 接收中，不处理
    }

    const char *parse = (const char *) (HID_read_buffer + 1);
    cJSON *root = cJSON_Parse(parse);
    if (root == NULL)
    {
        USB_LOG_ERR("parse json failed\n");
        goto fail;
    }

    // 取出key为"name"的值
    const cJSON *name = cJSON_GetObjectItem(root, "name");
    if (name == NULL)
    {
        USB_LOG_ERR("get name failed\n");
        goto fail;
    }

    // 先清空HID_write_buffer中其他数据
    memset(HID_write_buffer + 1, '\0', HID_PACKET_SIZE - 1);

    // 遍历hid_command，找到对应的处理函数
    for (size_t i = 0; i < sizeof(hid_command) / sizeof(hid_command[0]); i++)
    {
        if (strcmp(name->valuestring, hid_command[i].command) == 0)
        {
            USB_LOG_DBG("command %s found\n", name->valuestring);
            char *res = (char *) (HID_write_buffer + 1); // 第一个字节为Report ID
            if (hid_command[i].handler(res, root) < 0)
            {
                // 失败
                USB_LOG_ERR("command %s failed\n", hid_command[i].command);

                cJSON *fail = cJSON_CreateObject();
                Add_ResponseState(fail, HID_RESPONSE_FAILED);
                char *response_str = cJSON_PrintUnformatted(fail);
                strcpy(res, response_str);
                cJSON_free(response_str);
                cJSON_Delete(fail);
            }
            goto exit;
        }
    }

fail:
    // 进入此处说明未找到对应的处理函数
    USB_LOG_ERR("command %s not found\n", name->valuestring);
    cJSON *fail = cJSON_CreateObject();
    Add_ResponseState(fail, HID_RESPONSE_FAILED);
    cJSON_AddStringToObject(fail, "message", "command not found");
    char *response_str = cJSON_PrintUnformatted(fail);
    strcpy((char *) (HID_write_buffer + 1), response_str);
    cJSON_free(response_str);
    cJSON_Delete(fail);

exit:
    cJSON_Delete(root);
    HID_ReadState = HID_STATE_BUSY;
    usbd_ep_start_write(0,HID_IN_EP, HID_write_buffer, HID_PACKET_SIZE);
    usbd_ep_start_read(0, HID_OUT_EP, HID_read_buffer, HID_PACKET_SIZE);
}

#endif
