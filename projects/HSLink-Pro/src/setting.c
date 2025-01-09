#include "setting.h"

HSLink_Setting_t HSLink_Setting = {
    .boost = false,
    .swd_port_mode = PORT_MODE_SPI,
    .jtag_port_mode = PORT_MODE_SPI,
    .power = {
        .enable = false,
        .voltage = 3.3,
        .power_on = false,
    },
    .reset = RESET_NRST,
    .led = false,
    .led_brightness = 0,
};

void Setting_Init(void)
{

}
