#include "setting.h"

#include "BL_Setting_Common.h"
#include "board.h"
#include "led_extern.h"
#include "string"
#include "string_view"
#include <elog.h>
#include <flashdb.h>
#include <hpm_romapi.h>

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "usb2uart.h"

using namespace rapidjson;

const HSLink_Setting_t default_setting = {
        .boost = false,
        .swd_port_mode = PORT_MODE_SPI,
        .jtag_port_mode = PORT_MODE_SPI,
        .power = {
                .vref = 3.3,
                .power_on = true,
                .port_on = true,
        },
        .reset = 1 << RESET_NRST,
        .led = true,
        .led_brightness = 10,
};

HSLink_Setting_t HSLink_Setting = {
        .boost = false,
        .swd_port_mode = PORT_MODE_SPI,
        .jtag_port_mode = PORT_MODE_SPI,
        .power = {
                .vref = 3.3,
                .power_on = false,
                .port_on = false,
        },
        .reset = 0,
        .led = false,
        .led_brightness = 0,
};

HSLink_Lazy_t HSLink_Global;

ATTR_PLACE_AT(".bl_setting")
BL_Setting_t bl_setting;

template<>
auto get_json_value<bool>(const rapidjson::Value &val, const char *key, bool value) -> bool {
    if (val.HasMember(key)) {
        return val[key].GetBool();
    }
    return value;
}

template<>
auto get_json_value<int>(const rapidjson::Value &val, const char *key, int value) -> int {
    if (val.HasMember(key)) {
        return val[key].GetInt();
    }
    return value;
}

template<>
auto get_json_value<unsigned int>(const rapidjson::Value &val, const char *key, unsigned int value) -> unsigned int {
    if (val.HasMember(key)) {
        return val[key].GetUint();
    }
    return value;
}

template<>
auto get_json_value<double>(const rapidjson::Value &val, const char *key, double value) -> double {
    if (val.HasMember(key)) {
        return val[key].GetDouble();
    }
    return value;
}

template<>
auto get_json_value<const char*>(const rapidjson::Value &val, const char *key, const char* value) -> const char* {
    if (val.HasMember(key)) {
        return val[key].GetString();
    }
    return value;
}

static void store_settings();

static void load_settings();

static void update_settings();

static std::string stringify_settings()
{
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
    {
        writer.StartObject();
        writer.Key("vref");
        writer.Double(HSLink_Setting.power.vref);
        writer.Key("power_on");
        writer.Bool(HSLink_Setting.power.power_on);
        writer.Key("port_on");
        writer.Bool(HSLink_Setting.power.port_on);
        writer.EndObject();
    }

    writer.Key("reset");
    {
        writer.StartArray();
        if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_NRST)) {
            writer.String("nrst");
        }
        if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT)) {
            writer.String("arm_swd_soft");
        }
        if (SETTING_GET_RESET_MODE(HSLink_Setting.reset, RESET_POR)) {
            writer.String("por");
        }
        writer.EndArray();
    }

    writer.Key("led");
    writer.Bool(HSLink_Setting.led);

    writer.Key("led_brightness");
    writer.Uint(HSLink_Setting.led_brightness);

    writer.Key("jtag_20pin_compatible");
    writer.Bool(HSLink_Setting.jtag_20pin_compatible);

    writer.Key("nickname");
    writer.String(HSLink_Setting.nickname);

    writer.EndObject();
    return std::string{buffer.GetString(), buffer.GetSize()};
}

static void parse_settings(std::string_view json)
{
    Document root;
    root.Parse(json.data());
    HSLink_Setting.boost = root["boost"].GetBool();
    auto mode = [](const char *mode)
    {
        if (strcmp(mode, "spi") == 0) {
            return PORT_MODE_SPI;
        }
        return PORT_MODE_GPIO;
    };

    HSLink_Setting.swd_port_mode = mode(root["swd_port_mode"].GetString());
    HSLink_Setting.jtag_port_mode = mode(root["jtag_port_mode"].GetString());
    HSLink_Setting.jtag_single_bit_mode = get_json_value(root, "jtag_single_bit_mode", false);

    const Value &power = root["power"];
    HSLink_Setting.power.vref = power["vref"].GetDouble();
    HSLink_Setting.power.power_on = power["power_on"].GetBool();
    HSLink_Setting.power.port_on = power["port_on"].GetBool();

    HSLink_Setting.reset = 0;
    for (auto &reset: root["reset"].GetArray()) {
        if (strcmp(reset.GetString(), "nrst") == 0) {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_NRST);
        } else if (strcmp(reset.GetString(), "por") == 0) {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_POR);
        } else if (strcmp(reset.GetString(), "arm_swd_soft") == 0) {
            SETTING_SET_RESET_MODE(HSLink_Setting.reset, RESET_ARM_SWD_SOFT);
        }
    }

    HSLink_Setting.led = root["led"].GetBool();
    HSLink_Setting.led_brightness = root["led_brightness"].GetUint();
    HSLink_Setting.jtag_20pin_compatible = get_json_value(root, "jtag_20pin_compatible", false);

    std::strncpy(HSLink_Setting.nickname, get_json_value(root, "nickname", ""), sizeof(HSLink_Setting.nickname) - 1);
    HSLink_Setting.nickname[sizeof(HSLink_Setting.nickname) - 1] = '\0';
}

static void load_settings()
{
    char *settings_json_c_str = fdb_kv_get(&env_db, "settings");
    if (settings_json_c_str == NULL) {
        log_w("settings not found, use default");
        memcpy(&HSLink_Setting, &default_setting, sizeof(HSLink_Setting_t));
        store_settings();
        return;
    }
    auto settings_json_str = std::string{settings_json_c_str};
    parse_settings(settings_json_str);
    log_d("settings loaded %s", settings_json_str.c_str());
}

static void store_settings()
{
    auto settings_json_str = stringify_settings();
    fdb_kv_set(&env_db, "settings", settings_json_str.c_str());
    log_d("settings stored %s", settings_json_str.c_str());
}

static void update_settings()
{
    LED_SetBrightness(HSLink_Setting.led_brightness);
    LED_SetBoost(HSLink_Setting.boost);
    LED_SetEnable(HSLink_Setting.led);

    uartx_io_init();
}

void Setting_Init(void)
{
    load_settings();

    update_settings();

    if (CheckHardwareVersion(0, 0, 0) ||
        CheckHardwareVersion(1, 0, 0xFF) ||
        CheckHardwareVersion(1, 1, 0xFF)) {
        HSLink_Global.reset_level = 1;
    } else {
        HSLink_Global.reset_level = 0;
    }
}

void Setting_Save(void)
{
    store_settings();

    update_settings();
}

void Setting_SaveHardwareVersion(version_t hw_ver)
{
    std::string hw_ver_str =
            std::to_string(hw_ver.major) + "." +
            std::to_string(hw_ver.minor) + "." +
            std::to_string(hw_ver.patch);
    fdb_kv_set(&env_db, "hw_ver", hw_ver_str.c_str());
}
