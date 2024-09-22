#include "SW_DP.h"
#include "DAP.h"

PORT_Mode_t SWD_Port_Mode = PORT_MODE_SPI;

void PORT_SWD_SETUP(void)
{
    if (SWD_Port_Mode == PORT_MODE_SPI) {
        SPI_PORT_SWD_SETUP();
    } else {
        IO_PORT_SWD_SETUP();
    }
}

void SWJ_Sequence(uint32_t count, const uint8_t *data)
{
    if (DAP_Data.debug_port == DAP_PORT_JTAG) {
        gpio_write_pin(PIN_SWDIO_DIR_GPIO, GPIO_GET_PORT_INDEX(SWDIO_DIR), GPIO_GET_PIN_INDEX(SWDIO_DIR), 1); // SWDIO 输出
        PIN_SWDIO_TMS_SET();
        // 切换为GPIO控制器
        HPM_IOC->PAD[PIN_TCK].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0) | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK; /* as gpio*/
        HPM_IOC->PAD[PIN_TMS].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0); /* as gpio*/
        gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(PIN_TCK), GPIO_GET_PIN_INDEX(PIN_TCK));
        gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(PIN_TMS), GPIO_GET_PIN_INDEX(PIN_TMS));
        IO_SWJ_Sequence(count, data);
        // 切换为JTAG控制器
        HPM_IOC->PAD[PIN_TCK].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(5) | IOC_PAD_FUNC_CTL_LOOP_BACK_SET(1); /* as spi sck*/
        HPM_IOC->PAD[PIN_TMS].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
        return;
    }

    if (SWD_Port_Mode == PORT_MODE_SPI) {
        SPI_SWJ_Sequence(count, data);
    } else {
        IO_SWJ_Sequence(count, data);
    }
}

void SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi)
{
    if (SWD_Port_Mode == PORT_MODE_SPI) {
        SPI_SWD_Sequence(info, swdo, swdi);
    } else {
        IO_SWD_Sequence(info, swdo, swdi);
    }
}

uint8_t SWD_Transfer(uint32_t request, uint32_t *data)
{
    if (SWD_Port_Mode == PORT_MODE_SPI) {
        return SPI_SWD_Transfer(request, data);
    } else {
        return IO_SWD_Transfer(request, data);
    }
}
