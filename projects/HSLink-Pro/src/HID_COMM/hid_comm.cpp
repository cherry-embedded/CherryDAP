#include "hid_comm.h"
#include "dap_main.h"
#include "setting.h"
#include "HSLink_Pro_expansion.h"
#include "fal.h"
#include "fal_cfg.h"
#include <b64.h>
#include "crc32.h"

#define LOG_TAG "HID_COMM"

#include "elog.h"

#if CONFIG_CHERRYDAP_USE_CUSTOM_HID

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

using namespace rapidjson;

#include <functional>
#include <unordered_map>

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

std::string_view filed_miss_msg = "Some fields are missing";

static volatile HID_State_t HID_ReadState = HID_STATE_BUSY;

void hid_custom_notify_handler(uint8_t busid, uint8_t event, void *arg)
{
    (void)arg;

    switch (event) {
        case USBD_EVENT_CONFIGURED:
            usbd_ep_start_read(0, HID_OUT_EP, HID_read_buffer, HID_PACKET_SIZE);
            break;
        case USBD_EVENT_RESET:
            memset(HID_write_buffer, 0, HID_PACKET_SIZE);
            memset(HID_read_buffer, 0, HID_PACKET_SIZE);
            HID_write_buffer[0] = 0x02;
            break;
        default:
            break;
    }
}

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

static bool CheckField(const Value &object, std::pair<std::string_view, Type> field)
{
    auto &[field_name, field_type] = field;
    if (!object.HasMember(field_name.data())) {
        return false;
    }
    if (object[field_name.data()].GetType() != field_type) {
        return false;
    }
    return true;
}

static bool CheckFields(const Value &object, std::vector<std::pair<std::string_view, Type>> fields)
{
    for (auto &field: fields) {
        if (!CheckField(object, field)) {
            return false;
        }
    }
    return true;
}

static void FillStatus(HID_Response_t status, char *res, const char *message)
{
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

static void FillStatus(HID_Response_t status, char *res, std::string_view message)
{
    FillStatus(status, res, message.data());
}

static void FillStatus(HID_Response_t status, char *res)
{
    FillStatus(status, res, "");
}

static void Hello(Document &root, char *res)
{
    (void) root;
    StringBuffer buffer;

    Writer writer(buffer);
    writer.StartObject();

    writer.Key("serial");
    writer.String(serial_number_dynamic);

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

static void settings(Document &root, char *res)
{
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
    auto mode = [](const char *mode)
    {
        if (strcmp(mode, "spi") == 0) {
            return PORT_MODE_SPI;
        }
        return PORT_MODE_GPIO;
    };
    HSLink_Setting.swd_port_mode = mode(data["swd_port_mode"].GetString());
    HSLink_Setting.jtag_port_mode = mode(data["jtag_port_mode"].GetString());
    HSLink_Setting.jtag_single_bit_mode = get_json_value(data, "jtag_single_bit_mode", false);

    const Value &power = data["power"];
    HSLink_Setting.power.vref = power["vref"].GetDouble();
    HSLink_Setting.power.power_on = power["power_on"].GetBool();
    HSLink_Setting.power.port_on = power["port_on"].GetBool();

    HSLink_Setting.reset = 0;
    for (auto &reset: data["reset"].GetArray()) {
        if (strcmp(reset.GetString(), "nrst") == 0) {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_NRST);
        } else if (strcmp(reset.GetString(), "por") == 0) {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_POR);
        } else if (strcmp(reset.GetString(), "arm_swd_soft") == 0) {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT);
        }
    }

    HSLink_Setting.led = data["led"].GetBool();
    HSLink_Setting.led_brightness = data["led_brightness"].GetUint();

    HSLink_Setting.jtag_20pin_compatible = get_json_value(data, "jtag_20pin_compatible", false);

    Setting_Save();

    FillStatus(HID_RESPONSE_SUCCESS, res);
}

static void set_nickname(Document &root, char *res)
{
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

static void upgrade(Document &root, char *res)
{
    (void) root;

    FillStatus(HID_RESPONSE_SUCCESS, res);
    usbd_ep_start_write(0, HID_IN_EP, HID_write_buffer, HID_PACKET_SIZE);
    clock_cpu_delay_ms(5); // 确保已经发送完毕
    HSP_EnterHSLinkBootloader();
}

static void entry_sys_bl(Document &root, char *res)
{
    (void) root;

    FillStatus(HID_RESPONSE_SUCCESS, res);
    usbd_ep_start_write(0, HID_IN_EP, HID_write_buffer, HID_PACKET_SIZE);
    clock_cpu_delay_ms(5); // 确保已经发送完毕
    HSP_EntrySysBootloader();
}

static void entry_hslink_bl(Document &root, char *res)
{
    (void) root;

    FillStatus(HID_RESPONSE_SUCCESS, res);
    usbd_ep_start_write(0, HID_IN_EP, HID_write_buffer, HID_PACKET_SIZE);
    clock_cpu_delay_ms(5); // 确保已经发送完毕
    HSP_EnterHSLinkBootloader();
}

static void set_hw_ver(Document &root, char *res)
{
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

static void get_setting(Document &root, char *res)
{
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

    writer.Key("jtag_single_bit_mode");
    writer.Bool(HSLink_Setting.jtag_single_bit_mode);

    writer.Key("power");
    writer.StartObject();
    writer.Key("vref");
    writer.Double(HSLink_Setting.power.vref);
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

    writer.Key("jtag_20pin_compatible");
    writer.Bool(HSLink_Setting.jtag_20pin_compatible);

    writer.EndObject();

    std::strcpy(res, buffer.GetString());
}

static void erase_bl_b(Document &root, char *res)
{
    fal_partition_erase(bl_b_part, 0, bl_b_part->len);
    FillStatus(HID_RESPONSE_SUCCESS, res);
}

static void write_bl_b(Document &root, char *res)
{
#define PACK_SIZE 512
    if (!CheckFields(root,
                     {{"addr", Type::kNumberType},
                      {"len",  Type::kNumberType},
                      {"data", Type::kStringType}})) {
        FillStatus(HID_RESPONSE_FAILED, res, filed_miss_msg);
        return;
    }
    auto addr = root["addr"].GetInt();
    auto len = root["len"].GetInt();
    auto data_b64 = root["data"].GetString();
    auto data_b64_len = root["data"].GetStringLength();
    log_d("addr: 0x%X, len: %d, data_len: %d", addr, len, data_b64_len);
    //    elog_hexdump("b64", 16, data_b64, data_b64_len);
    if (size_t(addr + len) > bl_b_part->len) {
        const char *message = "addr out of range";
        USB_LOG_WRN("%s\n", message);
        FillStatus(HID_RESPONSE_FAILED, res, message);
        return;
    }
    if (len % 4 != 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "len %d not multiple of 4", len);
        log_w(msg);
        FillStatus(HID_RESPONSE_FAILED, res, msg);
        return;
    }
    if (addr % 512 != 0) {
        log_w("addr %d not multiple of 512", addr);
        return;
    }
    log_i("addr: 0x%X, len: %d", addr, len);
    size_t data_len = 0;
    uint8_t *data = b64_decode_ex(data_b64, data_b64_len, &data_len);
    log_d("solve b64 data_len: %d", data_len);
    //    elog_hexdump(LOG_TAG, 16, data, data_len);
    if (data_len != (size_t) len) {
        log_w("data_len != len");
        FillStatus(HID_RESPONSE_FAILED, res, "data_len != len");
        return;
    }
    if (data_len > PACK_SIZE) {
        char msg[64];
        snprintf(msg, sizeof(msg), "data_len %d > %d", data_len, PACK_SIZE);
        log_w(msg);
        FillStatus(HID_RESPONSE_FAILED, res, msg);
        return;
    }
    fal_partition_write(bl_b_part, addr, data, len);
    log_i("write %d bytes to 0x%X done", len, addr + bl_b_part->offset);
    FillStatus(HID_RESPONSE_SUCCESS, res);
    free(data);
}

static void upgrade_bl(Document &root, char *res)
{
    if (!CheckFields(root,
                     {{"len", Type::kNumberType},
                      {"crc", Type::kStringType}})) {
        FillStatus(HID_RESPONSE_FAILED, res, filed_miss_msg);
        return;
    }
    auto len = root["len"].GetInt();
    auto crc_str = root["crc"].GetString();
    auto crc = strtoul(crc_str + 2, nullptr, 16);
    if ((size_t) len > bl_b_part->len) {
        char msg[64];
        snprintf(msg, sizeof(msg), "len %d > %d", len, bl_b_part->len);
        log_w(msg);
        FillStatus(HID_RESPONSE_FAILED, res, msg);
        return;
    }
    if (len % 4) {
        log_w("len %% 4 != 0");
        FillStatus(HID_RESPONSE_FAILED, res, "len %% 4 != 0");
        return;
    }

    {
        uint32_t crc_calc = 0xFFFFFFFF;
        const uint32_t CRC_CALC_LEN = 8 * 1024;
        auto buf = std::make_unique<uint8_t[]>(CRC_CALC_LEN);
        for (uint32_t i = 0; i < (size_t) len; i += CRC_CALC_LEN) {
            auto calc_len = std::min(CRC_CALC_LEN, len - i);
            fal_partition_read(bl_b_part, i, buf.get(), calc_len);
            crc_calc = CRC_CalcArray_Software(buf.get(), calc_len, crc_calc);
            //        log_d("crc calc 0x%x", crc_calc);
        }
        if (crc != crc_calc) {
            log_w("crc != crc_calc recv 0x%x calc 0x%x", crc, crc_calc);
            FillStatus(HID_RESPONSE_FAILED, res, "crc != crc_calc");
            return;
        }
    }

    log_d("crc check pass, start copy...");
    {
        constexpr uint32_t COPY_LEN = 4 * 1024;
        auto buf = std::make_unique<uint8_t[]>(COPY_LEN);
        fal_partition_erase(bl_part, 0, bl_part->len);
        for (size_t i = 0; i < bl_part->len; i += COPY_LEN) {
            auto copy_len = std::min(COPY_LEN, (uint32_t) (bl_part->len - i));
            log_d("copy from 0x%X to 0x%X, size 0x%X",
                  i + bl_b_part->offset,
                  i + bl_part->offset,
                  copy_len);
            fal_partition_read(bl_b_part, i, buf.get(), copy_len);
            fal_partition_write(bl_part, i, buf.get(), copy_len);
        }
    }
    log_d("copy done");
    {
        const uint32_t COMP_LEN = 1024;
        auto buf_1 = std::make_unique<uint8_t[]>(COMP_LEN);
        auto buf_2 = std::make_unique<uint8_t[]>(COMP_LEN);
        for (size_t i = 0; i < bl_part->len; i += COMP_LEN) {
            auto comp_len = std::min(COMP_LEN, (uint32_t) (bl_part->len - i));
            fal_partition_read(bl_b_part, i, buf_1.get(), comp_len);
            fal_partition_read(bl_part, i, buf_2.get(), comp_len);
            if (memcmp(buf_1.get(), buf_2.get(), comp_len) != 0) {
                log_w("copy fail at 0x%X", i);
                FillStatus(HID_RESPONSE_FAILED, res, "copy done check failed");
                log_d("in b slot");
                elog_hexdump(LOG_TAG, 16, buf_1.get(), comp_len);
                log_d("in bl");
                elog_hexdump(LOG_TAG, 16, buf_2.get(), comp_len);
                // TODO should we roll back bl? or copy it again?
            }
        }
    }

    log_d("check done");

    FillStatus(HID_RESPONSE_SUCCESS, res);

    board_delay_ms(1000);

    HSP_Reboot();
}

static void HID_Write(const std::string &res)
{
    std::strcpy(reinterpret_cast<char *>(HID_write_buffer + 1), res.c_str());
}

static void HID_Write(const char *res)
{
    std::strcpy(reinterpret_cast<char *>(HID_write_buffer + 1), res);
}

using HID_Command_t = std::function<void(Document &, char *res)>;

void HID_Handle()
{
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
            {"set_hw_ver",      set_hw_ver},
            {"erase_bl_b",      erase_bl_b},
            {"write_bl_b",      write_bl_b},
            {"upgrade_bl",      upgrade_bl}
    };

    Document root;
    const auto parse = reinterpret_cast<char *>(HID_read_buffer + 1);
    root.Parse(parse);
    [&]()
    {
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
