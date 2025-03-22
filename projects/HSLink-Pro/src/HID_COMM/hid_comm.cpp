#include "hid_comm.h"
#include "usbd_core.h"
#include "dap_main.h"
#include "setting.h"
#include "usb_configuration.h"
#include "HSLink_Pro_expansion.h"

#ifdef CONFIG_USE_HID_CONFIG

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace rapidjson;

#include <unordered_map>
#include <functional>

USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t HID_read_buffer[HID_PACKET_SIZE];
USB_NOCACHE_RAM_SECTION USB_MEM_ALIGNX uint8_t HID_write_buffer[HID_PACKET_SIZE];

typedef enum {
    HID_STATE_BUSY = 0,
    HID_STATE_DONE,
} HID_State_t;

typedef enum {
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

void usbd_hid_custom_in_callback(uint8_t busid, uint8_t ep, uint32_t nbytes) {
    (void) busid;
    (void) ep;
    USB_LOG_DBG("actual in len:%d\r\n", nbytes);
    // custom_state = HID_STATE_IDLE;
}

void usbd_hid_custom_out_callback(uint8_t busid, uint8_t ep, uint32_t nbytes) {
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

static void FillStatus(HID_Response_t status, char *res, const char *message) {
    StringBuffer buffer;
    Writer writer(buffer);
    writer.StartObject();
    writer.Key("status");
    writer.String(response_str[status]);
    writer.Key("message");
    writer.String(message);
    writer.EndObject();
    std::strcpy(res, buffer.GetString());
}

static void FillStatus(HID_Response_t status, char *res) {
    FillStatus(status, res, "");
}

static void Hello(Document &root, char *res) {
    (void) root;
    StringBuffer buffer;

    Writer writer(buffer);
    writer.StartObject();

    writer.Key("serial");
    writer.String(serial_number);

    writer.Key("model");
    writer.String("HSLink-Pro");

    writer.Key("version");
    char version[32];
    snprintf(version, 32, "%d.%d.%d",
             bl_setting.app_version.major,
             bl_setting.app_version.minor,
             bl_setting.app_version.patch
    );
    writer.String(version);

    writer.Key("bootloader");
    snprintf(version, 32, "%d.%d.%d",
             bl_setting.bl_version.major,
             bl_setting.bl_version.minor,
             bl_setting.bl_version.patch
    );
    writer.String(version);

    writer.Key("hardware");
    if ((HSLink_Hardware_Version.major == 0x00 &&
         HSLink_Hardware_Version.minor == 0x00 &&
         HSLink_Hardware_Version.patch == 0x00) ||
        (HSLink_Hardware_Version.major == 0xFF &&
         HSLink_Hardware_Version.minor == 0xFF &&
         HSLink_Hardware_Version.patch == 0xFF)
            ) {
        writer.String("unknown");
    } else {
        char version[32];
        std::sprintf(version, "%d.%d.%d",
                     HSLink_Hardware_Version.major,
                     HSLink_Hardware_Version.minor,
                     HSLink_Hardware_Version.patch);
        writer.String(version);
    }

    writer.Key("nickname");
    writer.String(HSLink_Setting.nickname);

    writer.EndObject();

    std::strcpy(res, buffer.GetString());
}

static void settings(Document &root, char *res) {
    if (!root.HasMember("data") || !root["data"].IsObject()) {
        const char *message = "data not found";
        USB_LOG_WRN("%s\n", message);
        FillStatus(HID_RESPONSE_FAILED, res, message);
        return;
    }

    const Value &data = root["data"].GetObject();

    if (!data.HasMember("boost")) {
        const char *message = "boost not found";
        USB_LOG_WRN("%s\n", message);
        FillStatus(HID_RESPONSE_FAILED, res, message);
        return;
    }
    HSLink_Setting.boost = data["boost"].GetBool();
    auto mode = [](const char *mode) {
        if (strcmp(mode, "spi") == 0) {
            return PORT_MODE_SPI;
        }
        return PORT_MODE_GPIO;
    };
    HSLink_Setting.swd_port_mode = mode(data["swd_port_mode"].GetString());
    HSLink_Setting.jtag_port_mode = mode(data["jtag_port_mode"].GetString());

    const Value &power = data["power"];
    HSLink_Setting.power.voltage = power["vref"].GetDouble();
    HSLink_Setting.power.power_on = power["power_on"].GetBool();
    HSLink_Setting.power.port_on = power["port_on"].GetBool();

    for (auto &reset: data["reset"].GetArray()) {
        if (!SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_NRST) && strcmp(reset.GetString(), "nrst") == 0) {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_NRST);
        } else if (!SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_POR) && strcmp(reset.GetString(), "por") == 0) {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_POR);
        } else if (!SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT) && strcmp(
                reset.GetString(), "arm_swd_soft") == 0) {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT);
        }
    }

    HSLink_Setting.led = data["led"].GetBool();
    HSLink_Setting.led_brightness = data["led_brightness"].GetUint();

    Setting_Save();

    FillStatus(HID_RESPONSE_SUCCESS, res);
}

static void set_nickname(Document &root, char *res) {
    if (!root.HasMember("nickname") || !root["nickname"].IsString()) {
        const char *message = "nickname not found";
        USB_LOG_WRN("%s\n", message);
        FillStatus(HID_RESPONSE_FAILED, res, message);
        return;
    }
    std::memset(HSLink_Setting.nickname, 0, sizeof(HSLink_Setting.nickname));
    std::strcpy(HSLink_Setting.nickname, root["nickname"].GetString());

    Setting_Save();

    FillStatus(HID_RESPONSE_SUCCESS, res);
}

static void upgrade(Document &root, char *res) {
    (void) root;

    FillStatus(HID_RESPONSE_SUCCESS, res);
    usbd_ep_start_write(0, HID_IN_EP, HID_write_buffer, HID_PACKET_SIZE);
    clock_cpu_delay_ms(5); // 确保已经发送完毕
    HSP_EnterHSLinkBootloader();
}

static void entry_sys_bl(Document &root, char *res) {
    (void) root;

    FillStatus(HID_RESPONSE_SUCCESS, res);
    usbd_ep_start_write(0, HID_IN_EP, HID_write_buffer, HID_PACKET_SIZE);
    clock_cpu_delay_ms(5); // 确保已经发送完毕
    HSP_EntrySysBootloader();
}

static void entry_hslink_bl(Document &root, char *res) {
    (void) root;

    FillStatus(HID_RESPONSE_SUCCESS, res);
    usbd_ep_start_write(0, HID_IN_EP, HID_write_buffer, HID_PACKET_SIZE);
    clock_cpu_delay_ms(5); // 确保已经发送完毕
    HSP_EnterHSLinkBootloader();
}

static void set_hw_ver(Document &root, char *res) {
    if (!root.HasMember("hw_ver") || !root["hw_ver"].IsString()) {
        const char *message = "hw_ver not found";
        USB_LOG_WRN("%s\n", message);
        FillStatus(HID_RESPONSE_FAILED, res, message);
        return;
    }
    auto hw_ver_s = std::string{root["hw_ver"].GetString()};
    size_t pos1 = hw_ver_s.find('.');
    size_t pos2 = hw_ver_s.find('.', pos1 + 1);

    version_t ver{0};
    ver.major = std::stoi(hw_ver_s.substr(0, pos1));
    ver.minor = std::stoi(hw_ver_s.substr(pos1 + 1, pos2 - pos1 - 1));
    ver.patch = std::stoi(hw_ver_s.substr(pos2 + 1));

    Setting_SaveHardwareVersion(ver);

    FillStatus(HID_RESPONSE_SUCCESS, res);
}

static void get_setting(Document &root, char *res) {
    (void) root;
    StringBuffer buffer;
    Writer writer(buffer);
    writer.StartObject();

    writer.Key("boost");
    writer.Bool(HSLink_Setting.boost);

    writer.Key("swd_port_mode");
    writer.String(HSLink_Setting.swd_port_mode == PORT_MODE_SPI ? "spi" : "gpio");

    writer.Key("jtag_port_mode");
    writer.String(HSLink_Setting.jtag_port_mode == PORT_MODE_SPI ? "spi" : "gpio");

    writer.Key("power");
    writer.StartObject();
    writer.Key("vref");
    writer.Double(HSLink_Setting.power.voltage);
    writer.Key("power_on");
    writer.Bool(HSLink_Setting.power.power_on);
    writer.Key("port_on");
    writer.Bool(HSLink_Setting.power.port_on);
    writer.EndObject();

    writer.Key("reset");
    writer.StartArray();
    if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_NRST)) {
        writer.String("nrst");
    }
    if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_POR)) {
        writer.String("por");
    }
    if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT)) {
        writer.String("arm_swd_soft");
    }
    writer.EndArray();

    writer.Key("led");
    writer.Bool(HSLink_Setting.led);

    writer.Key("led_brightness");
    writer.Uint(HSLink_Setting.led_brightness);

    writer.EndObject();

    std::strcpy(res, buffer.GetString());
}

static void HID_Write(const std::string &res) {
    std::strcpy(reinterpret_cast<char *>(HID_write_buffer + 1), res.c_str());
}

static void HID_Write(const char *res) {
    std::strcpy(reinterpret_cast<char *>(HID_write_buffer + 1), res);
}

using HID_Command_t = std::function<void(Document &, char *res)>;

void HID_Handle() {
    if (HID_ReadState == HID_STATE_BUSY) {
        return; // 接收中，不处理
    }
    memset(HID_write_buffer + 1, 0, HID_PACKET_SIZE - 1);

    static std::unordered_map<std::string_view, HID_Command_t> hid_command = {
            {"Hello",           Hello},
            {"settings",        settings},
            {"set_nickname",    set_nickname},
            {"get_setting",     get_setting},
            {"upgrade",         upgrade},
            {"entry_sys_bl",    entry_sys_bl},
            {"entry_hslink_bl", entry_hslink_bl},
            {"set_hw_ver",      set_hw_ver}
    };

    Document root;
    const auto parse = reinterpret_cast<char *>(HID_read_buffer + 1);
    root.Parse(parse);
    [&]() {
        if (root.HasParseError()
            || root.HasMember("name") == false
                ) {
            HID_Write("parse error!\n");
            return;
        }

        auto name = root["name"].GetString();
        auto it = hid_command.find(name);
        if (it == hid_command.end()) {
            // 没有这个命令
            HID_Write("command " + std::string(name) + " not found!\n");
            return;
        }
        USB_LOG_DBG("command %s\n", name);
        it->second(root, reinterpret_cast<char *>(HID_write_buffer + 1));
    }();

    HID_ReadState = HID_STATE_BUSY;
    usbd_ep_start_write(0, HID_IN_EP, HID_write_buffer, HID_PACKET_SIZE);
    usbd_ep_start_read(0, HID_OUT_EP, HID_read_buffer, HID_PACKET_SIZE);
}

#endif
