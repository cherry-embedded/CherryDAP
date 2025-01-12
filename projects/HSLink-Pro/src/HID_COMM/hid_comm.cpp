#include "hid_comm.h"
#include "usbd_core.h"
#include "dap_main.h"
#include "setting.h"
#include "usb_configuration.h"

#ifdef CONFIG_USE_HID_CONFIG

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
using namespace rapidjson;

#include <unordered_map>
#include <functional>

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t HID_read_buffer[HID_PACKET_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t HID_write_buffer[HID_PACKET_SIZE];

typedef enum
{
    HID_STATE_BUSY = 0,
    HID_STATE_DONE,
} HID_State_t;

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
    .ep_addr = HID_IN_EP,
    .ep_cb = usbd_hid_custom_in_callback,
};

struct usbd_endpoint hid_custom_out_ep = {
    .ep_addr = HID_OUT_EP,
    .ep_cb = usbd_hid_custom_out_callback,
};

__attribute__((optimize("O1")))
static void Hello(Document &root, char* res)
{
    Document doc(kObjectType, &root.GetAllocator());
    Document::AllocatorType &allocator = doc.GetAllocator();
    doc.AddMember("serial", Value().SetString(StringRef(serial_number)).Move(),allocator);
    doc.AddMember("model", Value().SetString("HSLink-Pro").Move(), allocator);
    doc.AddMember("version", Value().SetString(CONFIG_BUILD_VERSION).Move(), allocator);
    doc.AddMember("bootloader", Value().SetString("1.0.0").Move(), allocator); // 以后再改
    if (HSLink_Setting.hardware.major == 0 && HSLink_Setting.hardware.minor == 0 && HSLink_Setting.hardware.patch == 0)
    {
        doc.AddMember("hardware", Value().SetString("unknown").Move(), allocator);
    }
    else
    {
        char version[32];
        sprintf(version, "%d.%d.%d", HSLink_Setting.hardware.major, HSLink_Setting.hardware.minor, HSLink_Setting.hardware.patch);
        doc.AddMember("hardware", Value().SetString(version, allocator).Move(), allocator);
    }
    doc.AddMember("nickname", Value().SetString(StringRef(HSLink_Setting.nickname)).Move(), allocator);

    StringBuffer buffer;
    Writer writer(buffer);
    doc.Accept(writer);

    std::strcpy(res, buffer.GetString());
}

//
// static int8_t settings(char *res, const cJSON *root)
// {
//     int8_t ret = -1;
//     cJSON *response = cJSON_CreateObject();
//     cJSON *data = cJSON_GetObjectItem(root, "data");
//     if (data == NULL)
//     {
//         const char *message = "data object not found";
//         USB_LOG_ERR("%s\n", message);
//         cJSON_AddStringToObject(response, "message", message);
//         goto fail;
//     }
//
//     HSLink_Setting.boost = (bool) cJSON_GetObjectItem(data, "boost")->valueint;
//     char *swd = cJSON_GetObjectItem(data, "swd_port_mode")->valuestring;
//     if (strcmp(swd, "spi") == 0)
//     {
//         HSLink_Setting.swd_port_mode = PORT_MODE_SPI;
//     }
//     else
//     {
//         // 其它任何字段都当作GPIO处理
//         HSLink_Setting.swd_port_mode = PORT_MODE_GPIO;
//     }
//
//     char *jtag = cJSON_GetObjectItem(data, "jtag_port_mode")->valuestring;
//     if (strcmp(jtag, "spi") == 0)
//     {
//         HSLink_Setting.jtag_port_mode = PORT_MODE_SPI;
//     }
//     else
//     {
//         // 其它任何字段都当作GPIO处理
//         HSLink_Setting.jtag_port_mode = PORT_MODE_GPIO;
//     }
//
//     cJSON *power = cJSON_GetObjectItem(data, "power");
//     HSLink_Setting.power.voltage = cJSON_GetObjectItem(power, "voltage")->valuedouble;
//     HSLink_Setting.power.power_on = (bool) cJSON_GetObjectItem(power, "power_on")->valueint;
//     HSLink_Setting.power.port_on = (bool) cJSON_GetObjectItem(power, "port_on")->valueint;
//
//     int reset_size = cJSON_GetArraySize(cJSON_GetObjectItem(data, "reset"));
//     cJSON *reset_array = cJSON_GetObjectItem(data, "reset");
//     HSLink_Setting.reset = 0;
//     for (int i = 0; i < reset_size; i++)
//     {
//         cJSON *reset = cJSON_GetArrayItem(reset_array, i);
//         if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_NRST) != 1 && strcmp(reset->valuestring, "nrst") == 0)
//         {
//             SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_NRST);
//         }
//         else if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_POR) != 1 && strcmp(reset->valuestring, "por") == 0)
//         {
//             SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_POR);
//         }
//         else if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT) != 1 && strcmp(reset->valuestring,
//                      "arm_swd_soft") == 0)
//         {
//             SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT);
//         }
//     }
//
//     HSLink_Setting.led = (bool) cJSON_GetObjectItem(data, "led")->valueint;
//     HSLink_Setting.led_brightness = cJSON_GetObjectItem(data, "led_brightness")->valueint;
//
//     Add_ResponseState(response, HID_RESPONSE_SUCCESS);
//     cJSON_AddStringToObject(response, "message", "");
//     ret = 0;
//     Setting_Save();
//     goto exit;
// fail:
//     Add_ResponseState(response, HID_RESPONSE_FAILED);
//     // 错误原因由错误的分支自行填充
//     // cJSON_AddStringToObject(response, "message", "settings failed");
// exit:
//     char *response_str = cJSON_PrintUnformatted(response);
//     strcpy(res, response_str);
//     cJSON_free(response_str);
//     cJSON_Delete(response);
//     return ret;
// }
//
// static int8_t set_nickname(char *res, const cJSON *root)
// {
//     int8_t ret = -1;
//     cJSON *response = cJSON_CreateObject();
//
//     char *nickname = cJSON_GetObjectItem(root, "nickname")->valuestring;
//     if (strlen(nickname) > sizeof(HSLink_Setting.nickname))
//     {
//         const char *message = "nickname too long";
//         USB_LOG_ERR("%s\n", message);
//         cJSON_AddStringToObject(response, "message", message);
//         goto fail;
//     }
//
//     strcpy(HSLink_Setting.nickname, nickname);
//     Add_ResponseState(response, HID_RESPONSE_SUCCESS);
//     cJSON_AddStringToObject(response, "message", "");
//     ret = 0;
//     Setting_Save();
//     goto exit;
// fail:
//     Add_ResponseState(response, HID_RESPONSE_FAILED);
// exit:
//     char *response_str = cJSON_PrintUnformatted(response);
//     strcpy(res, response_str);
//     cJSON_free(response_str);
//     cJSON_Delete(response);
//     return ret;
// }
//
// static int8_t get_settings(char *res, const cJSON *root)
// {
//     (void) root;
//     int8_t ret = -1;
//     cJSON *response = cJSON_CreateObject();
//     cJSON_AddBoolToObject(response, "boost", HSLink_Setting.boost);
//     cJSON_AddStringToObject(response, "swd_port_mode", HSLink_Setting.swd_port_mode == PORT_MODE_SPI ? "spi" : "gpio");
//     cJSON_AddStringToObject(response, "jtag_port_mode", HSLink_Setting.jtag_port_mode == PORT_MODE_SPI ? "spi" : "gpio");
//
//     cJSON *power = cJSON_CreateObject();
//     cJSON_AddNumberToObject(power, "voltage", HSLink_Setting.power.voltage);
//     cJSON_AddBoolToObject(power, "power_on", HSLink_Setting.power.power_on);
//     cJSON_AddBoolToObject(power, "port_on", HSLink_Setting.power.port_on);
//     cJSON_AddItemToObject(response, "power", power);
//
//     cJSON *reset = cJSON_CreateArray();
//     if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_NRST) == 1)
//     {
//         cJSON_AddItemToArray(reset, cJSON_CreateString("nrst"));
//     }
//     if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_POR) == 1)
//     {
//         cJSON_AddItemToArray(reset, cJSON_CreateString("por"));
//     }
//     if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT) == 1)
//     {
//         cJSON_AddItemToArray(reset, cJSON_CreateString("arm_swd_soft"));
//     }
//     cJSON_AddItemToObject(response, "reset", reset);
//
//     cJSON_AddBoolToObject(response, "led", HSLink_Setting.led);
//     cJSON_AddNumberToObject(response, "led_brightness", HSLink_Setting.led_brightness);
//
//
// exit:
//     Add_ResponseState(response, HID_RESPONSE_SUCCESS);
//     cJSON_AddStringToObject(response, "message", "");
//     char *response_str = cJSON_PrintUnformatted(response);
//     strcpy(res, response_str);
//     cJSON_free(response_str);
//     cJSON_Delete(response);
//     ret = 0;
//     return ret;
// }

// static const HID_Command_t hid_command[] = {
//     {"Hello", Hello},
//     {"settings", settings},
//     {"set_nickname", set_nickname},
//     {"get_settings", get_settings},
// };

static void HID_Write(const std::string &res)
{
    std::strcpy(reinterpret_cast<char *>(HID_write_buffer + 1), res.c_str());
}

static void HID_Write(const char *res)
{
    std::strcpy(reinterpret_cast<char *>(HID_write_buffer + 1), res);
}

using HID_Command_t = std::function<void(Document &, char* res)>;

void HID_Handle(void)
{
    if (HID_ReadState == HID_STATE_BUSY)
    {
        return; // 接收中，不处理
    }

    static std::unordered_map<std::string_view, HID_Command_t> hid_command = {
        {"Hello", Hello},
    };

    Document root;
    const auto parse = reinterpret_cast<char *>(HID_read_buffer + 1);
    root.Parse(parse);
    [&]()
    {
        if (root.HasParseError()
            || root.HasMember("name") == false
        )
        {
            HID_Write("parse error!\n");
            return;
        }

        auto name = root["name"].GetString();
        auto it = hid_command.find(name);
        if (it == hid_command.end())
        {
            // 没有这个命令
            HID_Write("command " + std::string(name) + " not found!\n");
            return;
        }
        USB_LOG_DBG("command %s\n", name);
        it->second(root, reinterpret_cast<char *>(HID_write_buffer + 1));
    }();

    HID_ReadState = HID_STATE_BUSY;
    usbd_ep_start_write(0,HID_IN_EP, HID_write_buffer, HID_PACKET_SIZE);
    usbd_ep_start_read(0, HID_OUT_EP, HID_read_buffer, HID_PACKET_SIZE);
}

#endif
